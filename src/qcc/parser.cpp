#include "parser.hpp"
#include "ast.hpp"
#include "escape_sequence.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <cctype>
#include <charconv>
#include <fmt/ranges.h>

namespace qcc
{

Parser::Parser(Ast &ast, Token *source, bool verbose) : ast(ast), source(source), verbose(verbose) {}

Statement *Parser::parse()
{
    ast.main_statement = new Scope_Statement{};
    context_push(ast.main_statement);
    parse_scope_statement(ast.main_statement, (Statement_Define | Statement_Function), Token_Eof);
    context_pop();
    return ast.main_statement;
}

void Parser::parse_type_cvr(Type *type, Token token, uint32 cvr_allowed)
{
    switch (token.type) {
    case Token_Const:
        type->cvr |= Token_Const;
        break;
    case Token_Volatile:
        type->cvr |= Token_Volatile;
        break;
    case Token_Restrict:
        type->cvr |= Token_Restrict;
        break;
    default:
        qcc_assert(0, "cannot cvr token");
    }

    if (cvr_allowed & ~type->cvr) {
        std::string_view cvr_disallowed = type_cvr_name(Type_Cvr{cvr_allowed & ~type->cvr});
        throw errorf("cannot use '{}' on type '{}'", token, cvr_disallowed, type->name());
    }
}

Type Parser::parse_type()
{
    Token token = {};
    Type type = {};
    type.size = -1;
    bool type_mods_allowed = true;
    int128 mask = Token_Mask_Type | Token_Mask_Record | Token_Id;

    while ((token = scan(mask)).ok) {
        if (token.type & Token_Mask_Type_Cvr) {
            parse_type_cvr(&type, token, Type_Const | Type_Volatile);
        }

        else if (token.type & (Token_Short | Token_Long | Token_Signed | Token_Unsigned)) {
            Type_Mod mod;

            if (token.type & Token_Long)
                mod = Type_Long;
            if (token.type & Token_Short)
                mod = Type_Short;
            if (token.type & Token_Signed)
                mod = Type_Signed;
            if (token.type & Token_Unsigned)
                mod = Type_Unsigned;

            if (type.mods & mod)
                throw errorf("type has duplicate modifiers '{}'", token, type_mod_name(mod));
            type.mods |= mod;

            if (type.mods & Type_Signed and type.mods & Type_Unsigned)
                throw errorf("type has discordant signedness modifiers", token);
            if (type.mods & Type_Short and type.mods & Type_Long)
                throw errorf("type has discordant size modifiers", token);
        }

        else if (token.type & (Token_Extern | Token_Register | Token_Static | Token_Auto)) {
            if (type.storage != Type_Local) {
                throw errorf("type has extraneous storage modifier with '{}'", token, token.str,
                             type_storage_name(type.storage));
            }
            if (token.type & Token_Extern)
                type.storage = Type_Extern;
            if (token.type & Token_Register)
                type.storage = Type_Register;
            if (token.type & Token_Static)
                type.storage = Type_Static;
            if (token.type & Token_Auto)
                type.storage = Type_Auto;
        }

        else if (token.type & Token_Mask_Fundamental) {
            if (type.kind != Type_Undefined)
                throw errorf("cannot combine type with '{}'", token, type.token.str);
            type.kind = token_to_type_kind(token.type);
            mask &= ~Token_Id;
        }

        else if (token.type & Token_Id) {
            Typedef *type_def = (Typedef *)context_scope()->object(token.str);
            if (!type_def or type_def->kind() != Object_Typedef)
                throw errorf("undefined type", token);
            type_system.merge_type(&type, type_def->type());
            type_mods_allowed = false;
            mask &= ~Token_Id;
        }

        else if (token.type & (Token_Struct | Token_Union)) {
            type_system.merge_type(&type, parse_struct_type(token));
            type_mods_allowed = false;
            mask &= ~Token_Id;
        }

        if (type.token.type & Token_Mask_Skip)
            type.token |= token;
    }

    if (!type_mods_allowed and type.mods != 0)
        throw errorf("type modifiers are not allowed in this type declaration", type.token);
    if (!type.kind and !type.mods)
        throw errorf("no type specified in declaration", peek(Token_Mask_Each));

    if (!type.kind and type.mods != 0)
        type.kind = Type_Int;
    if (type.kind & Type_Scalar and type.size == -1)
        type.size = type_system.scalar_size(type.kind, type.mods);
    if (type.kind & Type_Void)
        type.size = 0;
    if (type.kind & ~(Type_Char | Type_Int) and type.mods != 0)
        throw errorf("cannot declare type '{}' with modifiers", token, token.str);
    if (type.kind & Type_Int and !(type.mods & Type_Unsigned))
        type.mods |= Type_Signed;

    return type;
}

Declarator Parser::parse_declarator(Type type_base, Define_Env env)
{
    Declarator declarator = {};
    declarator.type = type_base;
    declarator.type = parse_pointer_declarator(declarator.type);
    declarator.name = scan(Token_Id);
    declarator.type = parse_array_declarator(declarator.type);

    if (!declarator.name.ok and env != Define_Parameter) {
        throw errorf("cannot define an anonymous {}", declarator.name, define_env_str(env));
    }
    if (peek(Token_Paren_Begin).ok) {
        declarator.has_matched_function = true;
        if (env != Define_Unknown)
            throw errorf("cannot define a function in this environment", declarator.name);
    }

    return declarator;
}

Type Parser::parse_pointer_declarator(Type pointed_type)
{
    Token star = scan(Token_Star);
    if (!star.ok) {
        return pointed_type;
    }
    Type type = {};
    type.token = pointed_type.token;
    type.kind = Type_Pointer;
    type.size = 8;
    type.pointed_type = type_system.orphan_type_push(new Type{pointed_type});

    Token token_cvr = {};
    while ((token_cvr = scan(Token_Mask_Type_Cvr)).ok) {
        parse_type_cvr(&type, token_cvr, Type_Const | Type_Volatile | Type_Restrict);
    }

    return parse_pointer_declarator(type);
}

Type Parser::parse_array_declarator(Type array_type)
{
    Token crochet_begin = scan(Token_Crochet_Begin);
    if (!crochet_begin.ok) {
        return array_type;
    }

    Type type = {};
    type.kind = Type_Array;
    type.array_type = type_system.orphan_type_push(new Type{array_type});

    Expression *size_expression = parse_expression();
    if (!size_expression) {
        qcc_todo("array type size inferrence, depends on array literals");
    } else {
        type.size = parse_constant(crochet_begin, size_expression) * array_type.size;
    }

    Token crochet_end = expect(Token_Crochet_End, "in array type declaration");
    type.token = array_type.token | crochet_end;
    return parse_array_declarator(type);
}

Type *Parser::parse_struct_type(Token keyword)
{
    Token name = scan(Token_Id);
    Token scope_begin = scan(Token_Scope_Begin);
    Type_Kind type_kind = token_to_type_kind(keyword.type);
    Record *record = NULL;

    if (name.ok) {
        record = context_scope()->record(type_kind, name.str);
    }
    if (record != NULL and record->type()->kind != type_kind) {
        throw errorf("was previously defined as '{}'", keyword | name, type_kind_name(record->type()->kind));
    }
    if (record != NULL and record->type()->struct_statement != NULL and scope_begin.ok) {
        throw errorf("redefinition of {} '{}'", keyword | name | scope_begin, keyword.str, name.str);
    }
    if (scope_begin.ok) {
        record = ast.push(new Record{});
        record->name = name;
        record->type()->kind = token_to_type_kind(keyword.type);
        record->type()->token = name;
        record->type()->struct_statement = parse_struct_statement(keyword);
        record->type()->size = type_system.struct_size(record->type());

        int64 alignment = record->type()->alignment();
        int64 offset = 0;
        for (auto [_, member] : record->type()->struct_statement->members) {
            member->struct_offset = Round_Up(offset, alignment);
            offset += member->type()->size;
        }

        context_scope()->records[name.str] = record;
    }

    return record->type();
}

Struct_Statement *Parser::parse_struct_statement(Token keyword)
{
    Struct_Statement *struct_statement = ast.push(new Struct_Statement{});
    struct_statement->keyword = keyword;
    struct_statement->hash = (uint64)struct_statement;
    context_push(struct_statement);

    Define_Env define_env = {};
    if (keyword.type & Token_Struct)
        define_env = Define_Struct;
    if (keyword.type & Token_Union)
        define_env = Define_Union;

    while (!peek_until(Token_Scope_End).ok) {
        parse_define_statement(parse_type(), define_env, NULL, Token_Semicolon);
    }

    expect(Token_Scope_End, "after struct scope statement");
    context_pop();
    return struct_statement;
}

Statement *Parser::parse_statement()
{
    Token token = peek(Token_Mask_Each);
    if (token.type & Token_If)
        return parse_condition_statement();
    if (token.type & Token_While)
        return parse_while_statement();
    if (token.type & Token_For)
        return parse_for_statement();
    if (token.type & Token_Return)
        return parse_return_statement();
    if (token.type & (Token_Mask_Type) or (token_is_typedef(token)))
        return parse_define_statement(parse_type(), Define_Unknown, NULL, Token_Semicolon);
    if (token.type & Token_Mask_Expression)
        return parse_expression_statement();

    qcc_todo("parse statement kind");
    return NULL;
}

Statement *Parser::parse_define_statement(Type type_base, Define_Env env, Define_Statement *previous,
                                          int128 end_mask)
{
    if (scan(Token_Semicolon).ok)
        return NULL;

    Declarator declarator = parse_declarator(type_base, env);
    auto &[type, name, _] = declarator;

    if (declarator.has_matched_function)
        return parse_function_statement(declarator);
    if (env & Define_Unknown)
        env = Define_Var;

    if (type.kind & Type_Void) {
        if (env & Define_Parameter) {
            expect(Token_Paren_End, "after void function parameter");
            return NULL;
        }
        throw errorf("cannot define {} as void", type.token | name, define_env_str(env));
    }

    Define_Statement *define_statement = ast.push(new Define_Statement{});

    Variable *variable = ast.push(new Variable{});
    define_statement->variable = variable;
    variable->env = env;
    variable->name = name;
    variable->var_type = type;
    context_push(define_statement);

    if (env & Define_Parameter and !name.ok) {
        variable->name.str = "(anonymous)";
        define_statement->next = parse_comma_define_statement(define_statement, type_base, env, end_mask);
        context_pop();
        return define_statement;
    }

    if (env & (Define_Var | Define_Enum) and scan(Token_Assign).ok) {
        define_statement->expression = parse_expression();
    }
    if (env & (Define_Var | Define_Enum)) {
        // Scope-wise check of duplicate
        if (context_scope()->object(name.str) != NULL)
            throw errorf("redefinition of '{}'", name, name.str);
    }
    if (env & (Define_Struct | Define_Union | Define_Parameter)) {
        // Local-wise check of duplicate
        if (context_scope()->objects.contains(name.str))
            throw errorf("redefinition of '{}'", name, name.str);
    }

    Function_Statement *function_statement = (Function_Statement *)context_of(Statement_Function);
    if (function_statement != NULL and env != Define_Parameter) {
        function_statement->function->locals.push_back(variable);
    }

    if (env & (Define_Struct | Define_Union)) {
        Struct_Statement *struct_statement = (Struct_Statement *)context_of(Statement_Struct);
        struct_statement->members[name.str] = define_statement->variable;
    } else {
        context_scope()->objects.emplace(name.str, define_statement->variable);
    }

    define_statement->next = parse_comma_define_statement(define_statement, type, env, end_mask);
    context_pop();
    return define_statement;
}

Define_Statement *Parser::parse_comma_define_statement(Define_Statement *define_statement, Type type_base,
                                                       Define_Env env, int128 end_mask)
{
    Token comma_or_end = expect(Token_Comma | end_mask, "after define statement");
    if (comma_or_end.type & end_mask)
        return NULL;
    if (env & Define_Parameter)
        type_base = parse_type();
    return (Define_Statement *)parse_define_statement(type_base, env, define_statement, end_mask);
}

Function_Statement *Parser::parse_function_statement(Declarator declarator)
{
    Token paren_begin = expect(Token_Paren_Begin, "after function name");
    auto &[return_type, name, _] = declarator;

    if (context_scope()->owner != NULL) {
        throw errorf("cannot define function inside scope", name);
    }

    Function *function = (Function *)context_scope()->object(name.str);
    if (!function) {
        function = ast.push(new Function{});
        function->name = name;
        function->return_type = return_type;
        function->is_main = (function->name.str == "main");
        context_scope()->objects.emplace(name.str, function);
    }

    Scope_Statement *scope_statement = ast.push(new Scope_Statement{});
    scope_statement->owner = context_scope();
    context_push(scope_statement);
    function->parameters =
        (Define_Statement *)parse_define_statement(parse_type(), Define_Parameter, NULL, Token_Paren_End);

    Function_Statement *function_statement = NULL;
    Token token = expect(Token_Scope_Begin | Token_Semicolon, "after function signature");
    if (token.type & Token_Scope_Begin) {
        function_statement = ast.push(new Function_Statement{});
        context_push(function_statement);

        function_statement->function = function;
        function_statement->scope =
            parse_scope_statement(scope_statement, Statement_Kind_Each, Token_Scope_End);
        context_pop();
    }

    context_pop();
    return function_statement;
}

Scope_Statement *Parser::parse_scope_statement(Scope_Statement *scope_statement, uint32 statement_mask,
                                               int128 end_mask)
{
    Token token = {};
    while (!(token = peek_until(end_mask)).ok) {
        Statement *statement = parse_statement();

        if (statement != NULL) {
            if (statement->kind() & ~statement_mask) {
                token = token | peek(Token_Mask_Each);
                std::string_view statement_name = statement_kind_str(statement->kind());
                throw errorf("unexpected {}-statement", token, statement_name);
            }
            scope_statement->body.push_back(statement);
        }
    }

    expect(end_mask, "at the end of scope statement");
    return scope_statement;
}

Scope_Statement *Parser::parse_enum_scope_statement(Token keyword, Type *enum_type)
{
    Type type = type_system.int_type;

    Define_Statement *define_statement =
        (Define_Statement *)parse_define_statement(type, Define_Enum, NULL, Token_Scope_End);
    context_scope()->body.push_back(define_statement);

    int64 enum_max = 0;
    Define_Statement *it = define_statement, *it_previous = NULL;
    for (; it != NULL; it_previous = it, it = it->next) {
        if (define_statement->expression != NULL) {
            it->variable->enum_constant = parse_constant(it->variable->name, define_statement->expression);
        } else {
            it->variable->enum_constant = it_previous != NULL ? it_previous->variable->enum_constant + 1 : 0;
        }

        enum_max = Max(enum_max, it->variable->enum_constant);
    }

    if (enum_max > INT32_MAX) {
        it = define_statement;
        for (; it != NULL; it = it->next) {
            it->variable->type()->size = 8;
        }
    }

    // Todo! set the enum_type, don't rembember why it's there
    return context_scope();
}

Scope_Statement *Parser::parse_maybe_inlined_scope_statement()
{
    Scope_Statement *scope_statement = ast.push(new Scope_Statement{});
    bool is_inlined = !scan(Token_Scope_Begin).ok;
    scope_statement->owner = context_scope();
    context_push(scope_statement);

    if (is_inlined) {
        scope_statement->body.push_back(parse_statement());
    } else {
        parse_scope_statement(scope_statement, Statement_Kind_Each, Token_Scope_End);
    }
    context_pop();
    return scope_statement;
}

Expression *Parser::parse_boolean_expression()
{
    Token paren_begin = expect(Token_Paren_Begin, "in boolean expression");
    Expression *expression = parse_expression();
    Token paren_end = expect(Token_Paren_End, "in boolean expression");
    return cast_if_needed(paren_begin | paren_end, expression, type_system.bool_type);
}

Condition_Statement *Parser::parse_condition_statement()
{
    qcc_assert(scan(Token_If | Token_Else).ok, "expected 'if' or 'else' token");

    Condition_Statement *condition_statement = ast.push(new Condition_Statement{});
    context_push(condition_statement);

    condition_statement->boolean = parse_boolean_expression();
    condition_statement->statement_if = parse_maybe_inlined_scope_statement();
    if (scan(Token_Else).ok)
        condition_statement->statement_else = parse_maybe_inlined_scope_statement();
    context_pop();
    return condition_statement;
}

While_Statement *Parser::parse_while_statement()
{
    qcc_assert(scan(Token_While).ok, "expected 'while' token");

    While_Statement *while_statement = ast.push(new While_Statement{});
    context_push(while_statement);
    while_statement->boolean = parse_boolean_expression();
    while_statement->statement = parse_maybe_inlined_scope_statement();
    context_pop();
    return while_statement;
}

For_Statement *Parser::parse_for_statement()
{
    qcc_assert(scan(Token_For).ok, "expected 'for' token");

    For_Statement *for_statement = ast.push(new For_Statement{});
    context_push(for_statement);

    Token paren_begin = expect(Token_Paren_Begin, "before init expression");
    for_statement->init = parse_expression();
    Token semicolon_init = expect(Token_Paren_Begin, "after init expression");
    for_statement->boolean = parse_expression();
    for_statement->boolean = cast_if_needed(semicolon_init, for_statement->boolean, type_system.bool_type);
    Token semicolon_boolean = expect(Token_Paren_Begin, "after boolean expression");
    for_statement->loop = parse_expression();
    Token paren_end = expect(Token_Paren_Begin, "after loop expression");
    for_statement->statement = parse_maybe_inlined_scope_statement();

    context_pop();
    return for_statement;
}

// Todo! void return statement
Return_Statement *Parser::parse_return_statement()
{
    Return_Statement *return_statement = ast.push(new Return_Statement{});
    Function_Statement *function_statement = (Function_Statement *)context_of(Statement_Function);
    context_push(return_statement);

    Token keyword = expect(Token_Return, "in return statement");
    if (!function_statement) {
        throw errorf("unexpected return outside of function", keyword);
    }

    return_statement->function = function_statement->function;
    return_statement->expression = parse_expression();

    Type *return_type = &return_statement->function->return_type;
    return_statement->expression = cast_if_needed(keyword, return_statement->expression, *return_type);

    expect(Token_Semicolon, "after return expression");
    context_pop();
    return return_statement;
}

Expression_Statement *Parser::parse_expression_statement()
{
    Expression_Statement *statement = ast.push(new Expression_Statement{});
    context_push(statement);
    statement->expression = parse_expression(statement->expression);
    expect(Token_Semicolon, "after expression statement");
    context_pop();
    return statement;
}

Expression *Parser::parse_expression(Expression *previous, int32 precedence)
{
    Expression *expression = NULL;
    Token token = scan(Token_Mask_Expression);

    if (!token.ok) {
        throw errorf("unexpected token", token);
    }
    switch (token.type) {
    case Token_Id:
        expression = parse_id_expression(token);
        break;

    case Token_Dot:
        expression = parse_dot_expression(token, previous);
        break;

    case Token_Arrow:
        expression = parse_arrow_expression(token, previous);
        break;

    case Token_Char:
    case Token_Int:
    case Token_Int_Bin:
    case Token_Int_Hex:
        expression = parse_int_expression(token);
        break;

    case Token_String:
        expression = parse_string_expression(token);
        break;

    case Token_Float:
        expression = parse_float_expression(token);
        break;

    case Token_Increment:
    case Token_Decrement:
        expression = parse_increment_expression(token, previous, precedence);
        break;

    case Token_Not:
    case Token_Bitwise_Not:
        expression = parse_unary_expression(token, Expression_Rhs, parse_expression(), precedence);
        break;

    case Token_Comma:
        expression = parse_comma_expression(token, previous);
        break;

    case Token_Assign:
        expression = parse_assign_expression(token, previous, parse_expression());
        break;

    case Token_Add:
    case Token_Sub:
        expression = previous != NULL ? (Expression *)parse_binary_expression(token, previous, precedence)
                                      : (Expression *)parse_unary_expression(token, Expression_Rhs,
                                                                             parse_expression(), precedence);
        break;

    case Token_Star:
        expression = previous != NULL ? (Expression *)parse_binary_expression(token, previous, precedence)
                                      : (Expression *)parse_deref_expression(token, parse_expression());
        break;

    case Token_Ampersand:
        expression = previous != NULL ? (Expression *)parse_binary_expression(token, previous, precedence)
                                      : (Expression *)parse_address_expression(token, parse_expression());
        break;

    case Token_Div:
    case Token_Mod:
    case Token_Bitwise_Or:
    case Token_Bitwise_Xor:
    case Token_Shift_L:
    case Token_Shift_R:
    case Token_Eq:
    case Token_Not_Eq:
    case Token_Less:
    case Token_Less_Eq:
    case Token_Greater:
    case Token_Greater_Eq:
    case Token_And:
    case Token_Or:
        expression = parse_binary_expression(token, previous, precedence);
        break;

    case Token_Paren_Begin:
        expression = previous != NULL ? (Expression *)parse_invoke_expression(token, previous)
                                      : (Expression *)parse_nested_expression(token);
        break;

    case Token_Crochet_Begin:
        expression = parse_subscript_expression(token, previous);
        break;

    default:
        expression = NULL;
    }

    if (!expression) {
        throw errorf("expression expected", token);
    }
    if (!expression->endpoint and peek(Token_Mask_Expression).ok) {
        return parse_expression(expression, precedence);
    }

    return expression;
}

Comma_Expression *Parser::parse_comma_expression(Token token, Expression *expression)
{
    Comma_Expression *comma_expression = ast.push(new Comma_Expression{});
    comma_expression->expression = expression;
    comma_expression->next = parse_expression();
    if (!comma_expression->expression)
        throw errorf("missing expression before ','", token);
    if (!comma_expression->next)
        throw errorf("missing expression after ','", token);

    return comma_expression;
}

Id_Expression *Parser::parse_id_expression(Token token)
{
    Object *object = context_scope()->object(token.str);
    if (!object)
        throw errorf("use of unknown identifier", token);

    Id_Expression *id_expression = ast.push(new Id_Expression{});
    id_expression->object = object;
    id_expression->token = token;
    return id_expression;
}

Int_Expression *Parser::parse_int_expression(Token token)
{
    Int_Expression *int_expression = ast.push(new Int_Expression{});
    int_expression->type = type_system.int_type;

    if (token.type & Token_Char) {
        std::string_view sequence = token.str.substr(1, token.str.size() - 2);
        std::string content = escape_string(sequence, 1);
        if (content.empty())
            throw errorf("empty character constant", token);
        if (content.size() > 1)
            throw errorf("multibyte character constant", token);
        int_expression->value = content[0];
    }
    if (token.type & (Token_Int | Token_Int_Bin | Token_Int_Hex)) {
        const char *number_begin = token.str.begin();
        const char *number_end = token.str.end();

        if (token.type & (Token_Int_Bin | Token_Int_Hex))
            number_begin = number_begin + 2;

        if (std::isalpha(number_end[-1])) {
            switch (number_end[-1]) {
            case 'l':
            case 'L':
                int_expression->type.mods |= Type_Long;
                break;
            case 'u':
            case 'U':
                int_expression->type.mods |= Type_Unsigned;
                break;
            }
        }

        std::from_chars_result status;
        if (token.type & Token_Int)
            status = std::from_chars<uint64>(number_begin, number_end, int_expression->value, 10);
        if (token.type & Token_Int_Hex)
            status = std::from_chars<uint64>(number_begin, number_end, int_expression->value, 16);
        if (token.type & Token_Int_Bin)
            status = std::from_chars<uint64>(number_begin, number_end, int_expression->value, 2);

        if (status.ec != std::errc{}) {
            std::string why = std::error_condition{status.ec}.message();
            throw errorf("cannot parse integer constant ({})", token, why);
        }
    }

    int_expression->type.size = type_system.scalar_size(Type_Int, int_expression->type.mods);
    return int_expression;
}

Float_Expression *Parser::parse_float_expression(Token token)
{
    Float_Expression *float_expression = ast.push(new Float_Expression{});

    const char *number_begin = token.str.begin();
    const char *number_end = token.str.end();

    if (std::isalpha(number_end[-1])) {
        switch (number_end[-1]) {
        case 'f':
        case 'F':
            float_expression->type = type_system.float_type;
            break;
        case 'l':
        case 'L':
            float_expression->type = type_system.double_type;
        }
    } else {
        float_expression->type = type_system.double_type;
    }

    std::from_chars_result status = {0};
    if (token.type & Token_Float) {
        status = std::from_chars(number_begin, number_end, float_expression->value);
    }
    if (status.ec != std::errc{}) {
        std::string why = std::error_condition{status.ec}.message();
        throw errorf("cannot parse float constant ({})", token, why);
    }

    return float_expression;
}

String_Expression *Parser::parse_string_expression(Token token)
{
    String_Expression *string_expression = ast.push(new String_Expression{});

    // Chop string quotes
    std::string_view raw_string = token.str.substr(1, token.str.size() - 2);
    string_expression->string = un_escape_string(raw_string, 1);
    return string_expression;
}

// Todo! operator precedence for ++/-- expressions
// (do not fetch another expression without checking the precedence)
Expression *Parser::parse_increment_expression(Token operation, Expression *operand, int32 precedence)
{
    Expression_Order order = operand != NULL ? Expression_Lhs : Expression_Rhs;
    int32 precedence_now = unary_operator_precedence(operation.type, order);

    if (order == Expression_Rhs) {
        operand = parse_expression(NULL, precedence_now);
    }

    if (precedence_now >= precedence) {
        operand->endpoint = true;
        source--;
        return operand;
    }

    Unary_Expression *unary_expression = ast.push(new Unary_Expression{});
    unary_expression->operand = typecheck_unary_operand(operand, operation);
    unary_expression->operation = operation;
    unary_expression->order = order;
    unary_expression->type = *type_system.expression_type(operand);

    if (categorize_expression(operand) != Expression_L) {
        throw errorf("operand is not assignable in '{}' operation", operation, operation.str);
    }
    return unary_expression;
}

Expression *Parser::parse_unary_expression(Token operation, Expression_Order order, Expression *operand,
                                           int32 precedence)
{
    int32 precedence_now = unary_operator_precedence(operation.type, order);
    if (precedence_now >= precedence) {
        operand->endpoint = true;
        source--;
        return operand;
    }

    Unary_Expression *unary_expression = ast.push(new Unary_Expression{});
    unary_expression->order = order;
    unary_expression->operand = typecheck_unary_operand(operand, operation);
    unary_expression->operation = operation;

    if (operation.type & Token_Mask_Boolean) {
        unary_expression->type = type_system.bool_type;
    } else {
        unary_expression->type = *type_system.expression_type(operand);
    }
    return unary_expression;
}

Expression *Parser::typecheck_unary_operand(Expression *operand, Token operation)
{
    Type *type = type_system.expression_type(operand);

    if (type->kind & Type_Array) {
        operand = cast_array_decay(operation, operand);
        return typecheck_unary_operand(operand, operation);
    }
    if (!(type->kind & Type_Scalar)) {
        throw errorf("cannot perform '{}' on type '{}'", operation, operation.str, type->name());
    }
    return operand;
}

Expression *Parser::parse_binary_expression(Token operation, Expression *lhs, int32 precedence)
{
    if (Token assign = scan(Token_Assign); assign.ok) {
        return parse_binary_assign_expression(operation, lhs);
    }

    int32 precedence_now = binary_operator_precedence(operation.type);
    if (precedence_now >= precedence) {
        source--;
        lhs->endpoint = true;
        return lhs;
    }

    Binary_Expression *binary_expression = ast.push(new Binary_Expression{});
    binary_expression->operation = operation;
    binary_expression->lhs = lhs;
    binary_expression->rhs = parse_expression(NULL, precedence_now);
    return typecheck_binary_expression(binary_expression);
}

Expression *Parser::parse_binary_assign_expression(Token operation, Expression *lhs)
{
    // binary assignments transform (a += b) into (a = a + b)
    Binary_Expression *binary_expression = ast.push(new Binary_Expression{});
    binary_expression->operation = operation;
    binary_expression->lhs = lhs;
    binary_expression->rhs = parse_expression(NULL, Lowest_Precedence);
    typecheck_binary_expression(binary_expression);

    return parse_assign_expression(operation, lhs, binary_expression);
}

Expression *Parser::typecheck_binary_operand(Expression *operand, Token operation)
{
    Type *type = type_system.expression_type(operand);

    if (type->kind & Type_Array) {
        operand = cast_array_decay(operation, operand);
        return typecheck_binary_operand(operand, operation);
    }
    if (!(type->kind & Type_Scalar)) {
        throw errorf("cannot perform '{}' on type '{}'", operation, operation.str, type->name());
    }
    if (type->kind & Type_Real and operation.type & (Token_Mod | Token_Mask_Bitwise)) {
        throw errorf("cannot perform '{}' on real type '{}'", operation, operation.str, type->name());
    }
    if (type->kind & Type_Pointer and operation.type & ~(Token_Add | Token_Sub | Token_Mask_Compare)) {
        throw errorf("cannot perform '{}' on pointer type '{}'", operation, operation.str, type->name());
    }
    return operand;
}

Expression *Parser::typecheck_binary_expression(Binary_Expression *binary_expression)
{
    binary_expression->lhs = typecheck_binary_operand(binary_expression->lhs, binary_expression->operation);
    binary_expression->rhs = typecheck_binary_operand(binary_expression->rhs, binary_expression->operation);

    if (binary_expression->operation.type & Token_Mask_Boolean) {
        binary_expression->type = type_system.bool_type;
    } else {
        binary_expression->type = *type_system.expression_type(binary_expression->lhs);
    }
    return binary_expression;
}

Nested_Expression *Parser::parse_nested_expression(Token token)
{
    Nested_Expression *nested_expression = ast.push(new Nested_Expression{});
    nested_expression->operand = parse_expression();
    expect(Token_Paren_End, "closing nested expression");
    return nested_expression;
}

Argument_Expression *Parser::parse_argument_expression(Token token, Function *function,
                                                       Define_Statement *parameter)
{
    Argument_Expression *argument_expression = ast.push(new Argument_Expression{});
    Ref_Expression *ref_expression = parse_ref_expression(parameter->variable, parameter->variable->type());
    argument_expression->assign_expression =
        parse_assign_expression(token, ref_expression, parse_expression());

    Token comma_or_end = expect(Token_Comma | Token_Paren_End, "in function invoke argument list");
    if (comma_or_end.type & Token_Comma and !parameter->next) {
        throw errorf("excessive function argument", comma_or_end);
    }
    if (comma_or_end.type & Token_Paren_End and parameter->next) {
        std::string_view name = parameter->variable->name.str;
        throw errorf("expected function argument for '{}'", comma_or_end, name);
    }
    if (comma_or_end.type & Token_Comma) {
        Argument_Expression *next = parse_argument_expression(token, function, parameter->next);
        next->previous = argument_expression;
        argument_expression->next = next;
    }
    return argument_expression;
}

Invoke_Expression *Parser::parse_invoke_expression(Token token, Expression *previous)
{
    Invoke_Expression *invoke_expression = ast.push(new Invoke_Expression{});
    invoke_expression->function = (Function *)ast.decode_designated_expression(previous);

    if (!invoke_expression->function or invoke_expression->function->kind() != Object_Function) {
        throw errorf("cannot invoke expression", token);
    }

    Function *function = invoke_expression->function;
    Define_Statement *parameters = invoke_expression->function->parameters;
    if (parameters != NULL) {
        invoke_expression->arguments = parse_argument_expression(token, function, parameters);
    } else {
        expect(Token_Paren_End, "in function invoke argument list");
    }
    return invoke_expression;
}

Expression *Parser::parse_subscript_expression(Token token, Expression *operand)
{
    // Convert (x[y]) into *((x) + z)), where z = y * sizeof(x[0])
    // z is implicit due to the compiler producing the pointer arithmetic
    Binary_Expression *binary_expression = ast.push(new Binary_Expression{});
    binary_expression->lhs = operand;
    binary_expression->rhs = parse_expression(NULL, Lowest_Precedence);
    binary_expression->operation = token | expect(Token_Crochet_End, "in subscript expression");
    binary_expression->operation.type = Token_Add;
    // Todo! does the typecheck got the subscription type right ?
    // binary_expression->type = type_system.expression_type(operand);
    typecheck_binary_expression(binary_expression);

    return parse_deref_expression(token, binary_expression);
}

Assign_Expression *Parser::parse_assign_expression(Token token, Expression *lhs, Expression *rhs)
{
    Assign_Expression *assign_expression = ast.push(new Assign_Expression{});
    assign_expression->lhs = lhs;
    assign_expression->type = type_system.expression_type(lhs);
    assign_expression->rhs = cast_if_needed(token, rhs, *assign_expression->type);

    Object *object = ast.decode_designated_expression(lhs);
    if (object != NULL and !object->has_assign()) {
        throw errorf("expression is not assignable", token);
    }

    return assign_expression;
}

Cast_Expression *Parser::parse_cast_expression(Token token, Expression *expression, Type type)
{
    Cast_Expression *cast_expression = ast.push(new Cast_Expression{});
    cast_expression->operand = expression;
    cast_expression->from = type_system.expression_type(expression);
    cast_expression->into = type;

    if (!cast_expression->operand)
        throw errorf("missing expression for type cast expression", token);
    return cast_expression;
}

Dot_Expression *Parser::parse_dot_expression(Token token, Expression *previous)
{
    Dot_Expression *dot_expression = ast.push(new Dot_Expression{});

    dot_expression->operand = previous;
    if (!dot_expression->operand) {
        throw errorf("variable expected for member access", token);
    }
    Type *type = type_system.expression_type(dot_expression->operand);
    if (type->kind & ~(Type_Struct | Type_Union)) {
        throw errorf("struct or union expected for member access ", token, token.str);
    }

    Token name = expect(Token_Id, "after member access operator");
    dot_expression->struct_statement = type->struct_statement;

    if (!type->struct_statement->members.contains(name.str)) {
        throw errorf("member '{}.{}' is undeclared", name, type->name(), name.str);
    }

    dot_expression->member = type->struct_statement->members[name.str];
    if (!dot_expression->member) {
        throw errorf("member '{}.{}' is not a variable member", name, type->name(), name.str);
    }

    return dot_expression;
}

Dot_Expression *Parser::parse_arrow_expression(Token token, Expression *operand)
{
    Deref_Expression *deref_expression = parse_deref_expression(token, operand);
    Dot_Expression *dot_expression = parse_dot_expression(token, deref_expression);
    return dot_expression;
}

Deref_Expression *Parser::parse_deref_expression(Token token, Expression *operand)
{
    Deref_Expression *deref_expression = ast.push(new Deref_Expression);
    deref_expression->operand = operand;
    deref_expression->type = type_system.expression_type(operand);

    if (!(deref_expression->type->kind & (Type_Pointer | Type_Array))) {
        throw errorf("'*' operand cannot be dereferenced", token);
    }

    deref_expression->type = deref_expression->type->pointed_type;
    return deref_expression;
}

Address_Expression *Parser::parse_address_expression(Token token, Expression *operand)
{
    Address_Expression *address_expression = ast.push(new Address_Expression);
    Type *operand_type = type_system.expression_type(operand);
    Expression_Category category = categorize_expression(operand);
    Object *object = ast.decode_designated_expression(operand);

    if (category != Expression_L) {
        throw errorf("'&' operand cannot be addressed", token);
    }
    if (object and object->kind() & Object_Variable) {
        Variable *variable = (Variable *)object;
        variable->location = Source_Stack;
    }
    address_expression->operand = operand;
    address_expression->type = Type{};
    address_expression->type.kind = Type_Pointer;
    address_expression->type.size = 8;
    address_expression->type.pointed_type = operand_type;

    return address_expression;
}

Ref_Expression *Parser::parse_ref_expression(Object *object, Type *type)
{
    Ref_Expression *ref_expression = ast.push(new Ref_Expression);

    ref_expression->object = object;
    ref_expression->type = type;
    return ref_expression;
}

Cast_Expression *Parser::cast_array_decay(Token token, Expression *expression)
{
    Type *array_type = type_system.expression_type(expression);
    qcc_assert(array_type->kind & Type_Array, "cannot decay a non-array type");

    Type decay_type = {};
    type_system.merge_type(&decay_type, array_type);
    decay_type.kind = Type_Pointer;
    decay_type.size = 8;
    return parse_cast_expression(token, expression, decay_type);
}

Expression *Parser::cast_if_needed(Token token, Expression *expression, Type type)
{
    Type *expression_type = type_system.expression_type(expression);
    uint32 type_cast = type_system.cast(expression_type, &type);

    if (type_cast & Type_Cast_Error) {
        throw errorf("cannot cast type '{}' into '{}'", token, expression_type->name(), type.name());
    }
    if (type_cast > Type_Cast_Same) {
        return parse_cast_expression(token, expression, type);
    }
    return expression;
}

bool Parser::token_is_typedef(Token token)
{
    if (token.type != Token_Id)
        return false;

    Typedef *type_def = (Typedef *)context_scope()->object(token.str);
    return type_def != NULL and type_def->kind() & Object_Typedef;
}

Expression_Category Parser::categorize_expression(Expression *expression)
{
    switch (expression->kind()) {
    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)expression;
        return Expression_L;
    }

    case Expression_Ref: {
        Ref_Expression *ref_expression = (Ref_Expression *)expression;
        return Expression_L;
    }

    case Expression_Address: {
        Address_Expression *address_expression = (Address_Expression *)expression;
        return Expression_L;
    }

    case Expression_Deref: {
        Deref_Expression *deref_expression = (Deref_Expression *)expression;
        return categorize_expression(deref_expression->operand);
    }

    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        return Expression_L;
    }

    case Expression_Unary: {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;
        if (unary_expression->operation.type & (Token_Increment | Token_Decrement))
            return Expression_L;
        return Expression_R;
    }

    case Expression_Binary: {
        Binary_Expression *binary_expression = (Binary_Expression *)expression;
        if (binary_expression->type.kind & (Type_Pointer | Type_Array))
            return Expression_L;
        return Expression_R;
    }

    case Expression_Assign: {
        Assign_Expression *assign_expression = (Assign_Expression *)expression;
        return categorize_expression(assign_expression->lhs);
    }

    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        return categorize_expression(nested_expression->operand);
    }

    default:
        return Expression_R;
    }
}

Scope_Statement *Parser::context_scope()
{
    return (Scope_Statement *)context_of(Statement_Scope);
}

Statement *Parser::context_of(uint32 kind)
{
    for (auto it = context.rbegin(); it != context.rend(); it++) {
        if ((*it)->kind() & kind)
            return (*it);
    }
    return NULL;
}

Statement *Parser::context_push(Statement *statement)
{
    if (verbose) {
        std::string_view kind = statement_kind_str(statement->kind());
        fmt::println(stderr, "{:{}}Context_Push! {}", "", context.size() * 4, kind);
    }

    context.push_back(statement);
    return statement;
}

Statement *Parser::context_pop()
{
    qcc_assert(!context.empty(), "cannot pop context, stack is empty");

    Statement *statement = context.back();
    context.pop_back();

    if (verbose) {
        std::string_view kind = statement_kind_str(statement->kind());
        fmt::println(stderr, "{:{}}Context_Pop!  {}", "", (context.size()) * 4, kind);
    }
    return statement;
}

Token Parser::peek_until(int128 mask)
{
    Token token = peek(mask);

    if (!token.ok and !(source->type & Token_Eof))
        return token;
    if (!token.ok)
        throw errorf("unexpected end of file", token);
    return token;
}

int64 Parser::parse_constant(Token token, Expression *expression)
{
    if (expression->kind() & Expression_Unary) {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;

#define Unary(type, op) \
    case type:          \
        return op(parse_constant(token, unary_expression->operand))

        switch (unary_expression->operation.type) {
            Unary(Token_Add, +);
            Unary(Token_Sub, +);
            Unary(Token_Not, !);
            Unary(Token_Bitwise_Not, ~);
        default:
            break;
        }
#undef Unary
    }

    if (expression->kind() & Expression_Binary) {
        Binary_Expression *binary_expression = (Binary_Expression *)expression;

#define Binary(type, op)                                      \
    case type:                                                \
        return (parse_constant(token, binary_expression->lhs) \
                    op parse_constant(token, binary_expression->rhs))

        switch (binary_expression->operation.type) {
            Binary(Token_Add, +);
            Binary(Token_Sub, -);
            Binary(Token_Mul, *);
            Binary(Token_Div, /);
            Binary(Token_Mod, %);
            Binary(Token_Less, <);
            Binary(Token_Less_Eq, <=);
            Binary(Token_Greater, >=);
            Binary(Token_Greater_Eq, >=);
            Binary(Token_And, &&);
            Binary(Token_Or, ||);
            Binary(Token_Bitwise_And, &);
            Binary(Token_Bitwise_Or, |);
            Binary(Token_Shift_L, <<);
            Binary(Token_Shift_R, >>);
            Binary(Token_Bitwise_Xor, ^);
        default:
            break;
        }
#undef Unary
    }

    if (expression->kind() & Expression_Int) {
        Int_Expression *int_expression = (Int_Expression *)expression;
        return int_expression->value;
    }

    if (expression->kind() & Expression_Nested) {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        return parse_constant(token, nested_expression->operand);
    }

    if (expression->kind() & Expression_Ternary) {
        Ternary_Expression *ternary_expression = (Ternary_Expression *)expression;
        bool boolean = parse_constant(token, ternary_expression->boolean);
        return boolean ? parse_constant(token, ternary_expression->expression_if)
                       : parse_constant(token, ternary_expression->expression_else);
    }

    throw errorf("expression is not an integer constant", token);
}

Token Parser::peek(int128 mask)
{
    Token token = *source;
    token.ok = token.type & mask;
    return token;
}

Token Parser::scan(int128 mask)
{
    if (source->type & Token_Eof)
        return *source;
    Token token = *source;
    token.ok = token.type & mask;
    if (token.ok)
        source++;
    return token;
}

Token Parser::expect(int128 mask, std::string_view context)
{
    Token token = scan(mask);

    if (!token.ok) {
        std::vector<std::string> expected = {};
        for (int128 type = 1; type != Token_Type_End; type <<= 1) {
            if (type & mask) {
                std::string_view type_str = token_type_str((Token_Type)type);
                expected.push_back(fmt::format("'{}'", type_str));
            }
        }
        throw errorf("expected token {} {}", token, fmt::join(expected, ", "), context);
    }

    return token;
}

} // namespace qcc
