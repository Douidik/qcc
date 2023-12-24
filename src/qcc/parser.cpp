#include "parser.hpp"
#include "ast.hpp"
#include "escape_sequence.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "scan/scanner.hpp"
#include "statement.hpp"
#include <cctype>
#include <charconv>

namespace qcc
{

Parser::Parser(Ast &ast, Scanner &scanner) : ast(ast), scanner(scanner) {}

Statement *Parser::parse()
{
    ast.main_statement = new Scope_Statement;
    context.push_back(ast.main_statement);
    parse_scope_statement(ast.main_statement, (Statement_Define | Statement_Function), Token_Eof);
    context.pop_back();
    return ast.main_statement;
}

void Parser::parse_type_cvr(Type *type, Token token, bool is_pointer_type)
{

    switch (token.type) {
    case Token_Const:
        type->cvr |= Token_Const;
        break;
    case Token_Volatile:
        type->cvr |= Token_Volatile;
        break;
    case Token_Restrict:
        if (!is_pointer_type) {
            throw errorf("invalid use of 'restrict' keyword", token);
        }
        type->cvr |= Token_Restrict;
        break;
    default:
        qcc_assert("parse_type_cvr() token is not cvr", 0);
    }
}

Type Parser::parse_type()
{
    Token token = {};
    Type type = {};
    type.size = -1;
    bool type_mods_allowed = true;
    bool inside_pointer = false;
    int128 mask = Token_Mask_Type | Token_Mask_Record | Token_Id;

    while ((token = scan(mask)).ok) {
        if (token.type & Token_Mask_Type_Cvr) {
            parse_type_cvr(&type, token, false);
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
                throw errorf("type has discordant length modifiers", token);
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
            type_system.merge_type(&type, &type_def->type);
            type_mods_allowed = false;
            mask &= ~Token_Id;
        }

        else if (token.type & (Token_Struct | Token_Union)) {
            Type struct_type = parse_struct_type(token);
            type_system.merge_type(&type, &struct_type);
            type_mods_allowed = false;
            mask &= ~Token_Id;
        }

        // else if (token.type & Token_Mask_Record) {
        //     Record_Statement *record_statement = parse_record_statement(token);
        //     type_system.merge_type(&type, record_statement->type);
        //     type_mods_allowed = false;
        //     mask &= ~Token_Id;
        // }

        else if (token.type & Token_Pointer) {
            inside_pointer = true;
            mask = 0;
        }

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
    if (type.kind & Type_Record and type.mods != 0)
        throw errorf("type modifiers are not allowed in record type declaration", type.token);
    if (type.kind & Type_Void)
        type.size = 0;
    if (type.kind & ~(Type_Char | Type_Int) and type.mods != 0)
        throw errorf("cannot declare type '{}' with modifiers", token, token.str);

    return inside_pointer ? parse_pointer_type(type) : type;
}

Type Parser::parse_pointer_type(Type pointed_type)
{
    Type type = {};
    type.token = pointed_type.token;
    type.kind = Type_Pointer;
    type.size = 8;
    type.pointed_type = type_system.orphan_type_push(new Type{pointed_type});

    Token token_cvr = {};
    while ((token_cvr = scan(Token_Mask_Type_Cvr)).ok) {
        parse_type_cvr(&type, token_cvr, false);
    }

    return scan(Token_Pointer).ok ? parse_pointer_type(type) : type;
}

Type Parser::parse_struct_type(Token keyword)
{
    Token name = scan(Token_Id);
    Token scope_begin = scan(Token_Scope_Begin);
    Type_Kind type_kind = token_to_type_kind(keyword.type);
    Record *record = NULL;

    if (name.ok) {
        record = context_scope()->record(type_kind, name.str);
    }
    if (record != NULL and record->type.kind != type_kind) {
        throw errorf("was previously defined as '{}'", keyword | name, type_kind_name(record->type.kind));
    }
    if (record != NULL and record->type.struct_statement != NULL and scope_begin.ok) {
        throw errorf("redefinition of {} '{}'", keyword | name | scope_begin, keyword.str, name.str);
    }
    if (scope_begin.ok) {
        record = ast.push(new Record{});
        record->name = name;
	record->type.kind = token_to_type_kind(keyword.type);
        record->type.token = name;
        record->type.struct_statement = parse_struct_statement(keyword);
        record->type.size = type_system.struct_size(&record->type);

        context_scope()->records[name.str] = record;
    }

    return type_system.clone_type(ast, &record->type);
}

Struct_Statement *Parser::parse_struct_statement(Token keyword)
{
    Struct_Statement *struct_statement = ast.push(new Struct_Statement{});
    struct_statement->keyword = keyword;
    struct_statement->hash = (uint64)struct_statement;
    context.push_back(struct_statement);

    Define_Mode define_mode = {};
    if (keyword.type & Token_Struct)
        define_mode = Define_Struct;
    if (keyword.type & Token_Union)
        define_mode = Define_Union;

    while (!peek_until(Token_Scope_End).ok) {
        Type type = parse_type();
        Token name = scan(Token_Id);
        parse_define_statement(type, name, define_mode, NULL, Token_Semicolon);
    }

    expect(Token_Scope_End, "after struct scope statement");
    context.pop_back();
    return struct_statement;
}

Statement *Parser::parse_statement()
{
    Token token = peek(Token_Mask_Each);

    // TODO! nested scope statement
    // if (token.type & Token_Scope_Begin)
    //     return parse_scope_statement(Token_Scope_End);
    if (token.type & Token_If)
        return parse_condition_statement();
    if (token.type & Token_While)
        return parse_while_statement();
    if (token.type & Token_For)
        return parse_for_statement();
    if (token.type & Token_Return)
        return parse_return_statement();
    if (token.type & Token_Mask_Type)
        return parse_define_or_function_statement();
    if (token.type & Token_Id) {
        if (Typedef *type_def = (Typedef *)context_scope()->object(token.str);
            type_def != NULL and type_def->kind() & Object_Typedef) {
            return parse_define_or_function_statement();
        }
    }
    if (token.type & Token_Mask_Expression)
        return parse_expression_statement();

    qcc_assert("non-reachable", false);
    return NULL;
}

Statement *Parser::parse_define_or_function_statement()
{
    Type type = parse_type();
    Token name = expect(Token_Id | Token_Semicolon, "after type in define statement");
	
    if (name.ok and peek(Token_Paren_Begin).ok)
        return parse_function_statement(type, name);
    else
        return parse_define_statement(type, name, Define_Variable, NULL, Token_Semicolon);
}

Define_Statement *Parser::parse_define_statement(Type type, Token name, Define_Mode mode,
                                                 Define_Statement *previous, int128 end_mask)
{
    if (name.type & Token_Semicolon)
        return NULL;

    Define_Statement *define_statement = ast.push(new Define_Statement{});
    Variable *variable = ast.push(new Variable{});
    define_statement->variable = variable;
    variable->mode = mode;
    variable->name = name;
    variable->type = type;

    if (type.kind & Type_Void) {
        if (mode & Define_Parameter) {
            expect(Token_Paren_End, "after void function parameter");
            return NULL;
        }
        throw errorf("cannot define variable as void", type.token | name);
    }

    context.push_back(define_statement);

    if (!variable->name.ok) {
        variable->name.str = "(anonymous)";
        define_statement->next = parse_comma_define_statement(define_statement, mode, end_mask);
        context.pop_back();
        return define_statement;
    }

    if (mode & (Define_Variable | Define_Enum) and scan(Token_Assign).ok) {
        define_statement->expression = parse_expression(NULL);
    }
    if (mode & (Define_Variable | Define_Enum)) {
        // Scope-wise check of duplicate
        if (context_scope()->object(name.str) != NULL)
            throw errorf("redefinition of '{}'", name, name.str);
    }
    if (mode & (Define_Struct | Define_Union | Define_Parameter)) {
        // Parameters-wise check of duplicate
        if (context_scope()->objects.contains(name.str))
            throw errorf("redefinition of '{}'", name, name.str);
    }

    if (mode & Define_Enum) {
        int64 &constant = variable->constant;
        if (define_statement->expression != NULL) {
            constant = parse_constant(name, define_statement->expression);
        } else {
            constant = previous != NULL ? previous->variable->constant + 1 : 0;
        }
    }

    Function_Statement *function_statement = (Function_Statement *)context_of(Statement_Function);
    if (function_statement != NULL and mode != Define_Parameter) {
        function_statement->function->locals.push_back(variable);
    }

    if (mode & (Define_Struct | Define_Union)) {
        Struct_Statement *struct_statement = (Struct_Statement *)context_of(Statement_Struct);
        struct_statement->members[name.str] = define_statement->variable;
    } else {
        context_scope()->objects.emplace(name.str, define_statement->variable);
    }

    define_statement->next = parse_comma_define_statement(define_statement, mode, end_mask);
    context.pop_back();
    return define_statement;
}

Define_Statement *Parser::parse_comma_define_statement(Define_Statement *define_statement, Define_Mode mode,
                                                       int128 end_mask)
{
    Token sep = expect(Token_Comma | end_mask, "after define statement");
    if (sep.type & end_mask) {
        return NULL;
    }

    Type type = {};
    if (mode & Define_Parameter)
        type = parse_type();
    else
        type = define_statement->variable->type;

    Token comma_name = scan(Token_Id);
    if (!comma_name.ok and mode & Define_Enum)
        return NULL;
    return parse_define_statement(type, comma_name, mode, define_statement, end_mask);
}

Function_Statement *Parser::parse_function_statement(Type return_type, Token name)
{
    expect(Token_Paren_Begin, "after function name");

    if (!name.ok) {
        throw errorf("function cannot be anonymous", return_type.token);
    }
    if (context_scope()->owner != NULL) {
        throw errorf("cannot define function inside scope", name);
    }

    Function *function = (Function *)context_scope()->object(name.str);
    if (!function) {
        function = ast.push(new Function{});
        function->name = name;
        function->return_type = return_type;
        if (function->name.str == "main") {
            function->is_main = true;
        }
        context_scope()->objects.emplace(name.str, function);
    }

    Scope_Statement *scope_statement = ast.push(new Scope_Statement{});
    scope_statement->owner = context_scope();
    context.push_back(scope_statement);

    Type parameter_type = parse_type();
    Token parameter_id = scan(Token_Id);
    function->parameters =
        parse_define_statement(parameter_type, parameter_id, Define_Parameter, NULL, Token_Paren_End);

    Function_Statement *function_statement = NULL;
    Token token = expect(Token_Scope_Begin | Token_Semicolon, "after function signature");
    if (token.type & Token_Scope_Begin) {
        function_statement = ast.push(new Function_Statement{});
        context.push_back(function_statement);

        function_statement->function = function;
        function_statement->scope =
            parse_scope_statement(scope_statement, Statement_Kind_Each, Token_Scope_End);
        context.pop_back();
    }

    context.pop_back();
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

// Record_Statement *Parser::parse_record_statement(Token keyword)
// {
//     Record_Statement *record_statement = ast.push(new Record_Statement{});
//     context.push_back(record_statement);

//     Token name = scan(Token_Id);
//     Type_Kind type_kind = token_to_type_kind(keyword.type);
//     Record *record = context_scope()->record(type_kind, name.str);
//     Token scope_begin = scan(Token_Scope_Begin);

//     if (record != NULL and record->type.kind != type_kind) {
//         throw errorf("was previously defined as '{}'", keyword | name, keyword.str,
//                      type_kind_name(record->type.kind));
//     }
//     if (record != NULL and record->type.struct_statement != NULL and scope_begin.ok) {
//         throw errorf("redefinition of {} '{}'", keyword | name | scope_begin, keyword.str, name.str);
//     }
//     if (scope_begin.ok) {
//         record = ast.push(new Record{});
//         Type *type = &record->type;
//         type->kind = token_to_type_kind(keyword.type);

//         if (keyword.type & (Token_Struct | Token_Union)) {
//             type->scope = parse_struct_scope_statement(keyword);
//             type->size = type_system.struct_size(type);
//         }
//         if (keyword.type & (Token_Enum)) {
//             Type *enum_type = NULL;
//             type->scope = parse_enum_scope_statement(keyword, enum_type);
//             type->enum_type = enum_type;
//             type->size = type->enum_type->size;
//         }

//         if (name.ok) {
//             context_scope()->records.emplace(name.str, record);
//         }
//     }

//     record_statement->type = &record->type;
//     context.pop_back();
//     return record_statement;
// }

Scope_Statement *Parser::parse_enum_scope_statement(Token keyword, Type *enum_type)
{
    Type type = type_system.int_type;

    Define_Statement *define_statement =
        parse_define_statement(type, scan(Token_Id), Define_Enum, NULL, Token_Scope_End);
    context_scope()->body.push_back(define_statement);

    uint64 max = 0;
    for (Define_Statement *define = define_statement; define != NULL; define = define->next) {
        max = std::max(max, (uint64)(define->variable)->constant);
    }
    if (max > std::numeric_limits<int32>::max()) {
        for (Define_Statement *define = define_statement; define != NULL; define = define->next) {
            define->variable->type.size = 8;
        }
    }

    enum_type = &type;
    return context_scope();
}

Scope_Statement *Parser::parse_maybe_inlined_scope_statement()
{
    Scope_Statement *scope_statement = ast.push(new Scope_Statement{});
    bool is_inlined = !scan(Token_Scope_Begin).ok;
    scope_statement->owner = context_scope();
    context.push_back(scope_statement);

    if (is_inlined) {
        scope_statement->body.push_back(parse_statement());
    } else {
        parse_scope_statement(scope_statement, Statement_Kind_Each, Token_Scope_End);
    }
    context.pop_back();
    return scope_statement;
}

Expression *Parser::parse_boolean_expression()
{
    Token paren_begin = expect(Token_Paren_Begin, "before nested boolean expression");
    Expression *boolean_expression = parse_expression(NULL);
    Token paren_end = expect(Token_Paren_End, "after nested boolean expression");

    Type *type = type_system.expression_type(boolean_expression);
    if (type->kind & ~Type_Scalar) {
        throw errorf("expression must reduce to a scalar", paren_begin | paren_end);
    }
    return boolean_expression;
}

Condition_Statement *Parser::parse_condition_statement()
{
    qcc_assert("expected 'if' or 'else' token", scan(Token_If | Token_Else).ok);

    Condition_Statement *condition_statement = ast.push(new Condition_Statement{});
    context.push_back(condition_statement);

    condition_statement->boolean = parse_boolean_expression();
    condition_statement->statement_if = parse_maybe_inlined_scope_statement();
    if (scan(Token_Else).ok)
        condition_statement->statement_else = parse_maybe_inlined_scope_statement();
    context.pop_back();
    return condition_statement;
}

While_Statement *Parser::parse_while_statement()
{
    qcc_assert("expected 'while' token", scan(Token_While).ok);

    While_Statement *while_statement = ast.push(new While_Statement{});
    context.push_back(while_statement);
    while_statement->boolean = parse_boolean_expression();
    while_statement->statement = parse_maybe_inlined_scope_statement();
    context.pop_back();
    return while_statement;
}

For_Statement *Parser::parse_for_statement()
{
    qcc_assert("expected 'for' token", scan(Token_For).ok);

    For_Statement *for_statement = ast.push(new For_Statement{});
    context.push_back(for_statement);

    Token paren_begin = expect(Token_Paren_Begin, "before init expression");
    for_statement->init = parse_expression(NULL);
    Token semicolon_init = expect(Token_Paren_Begin, "after init expression");
    for_statement->boolean = parse_expression(NULL);
    Token semicolon_boolean = expect(Token_Paren_Begin, "after boolean expression");
    for_statement->loop = parse_expression(NULL);
    Token paren_end = expect(Token_Paren_Begin, "after loop expression");
    for_statement->statement = parse_maybe_inlined_scope_statement();

    context.pop_back();
    return for_statement;
}

// TODO! void return statement
Return_Statement *Parser::parse_return_statement()
{
    Return_Statement *return_statement = ast.push(new Return_Statement{});
    Function_Statement *function_statement = (Function_Statement *)context_of(Statement_Function);
    context.push_back(return_statement);

    Token keyword = expect(Token_Return, "in return statement");
    if (!function_statement) {
        throw errorf("unexpected return outside of function", keyword);
    }

    return_statement->function = function_statement->function;
    return_statement->expression = parse_expression(NULL);

    Type *return_type = &return_statement->function->return_type;
    return_statement->expression = cast_if_needed(keyword, return_statement->expression, return_type);

    expect(Token_Semicolon, "after return expression");
    context.pop_back();
    return return_statement;
}

Expression_Statement *Parser::parse_expression_statement()
{
    Expression_Statement *statement = ast.push(new Expression_Statement{});
    context.push_back(statement);

    while (!peek_until(Token_Semicolon).ok) {
        statement->expression = parse_expression(statement->expression);
    }
    expect(Token_Semicolon, "after expression statement");
    context.pop_back();
    return statement;
}

Expression *Parser::parse_expression(Expression *previous)
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
        expression = parse_increment_expression(token, previous);
        break;

    case Token_Not:
    case Token_Bin_Not:
        expression = parse_unary_expression(token, Expression_Rhs, parse_expression(NULL));
        break;

    case Token_Comma:
        expression = parse_comma_expression(token, previous);
        break;

    case Token_Assign:
        expression = parse_assign_expression(token, previous, parse_expression(NULL));
        break;

    case Token_Add:
    case Token_Sub:
        expression =
            previous != NULL
                ? (Expression *)parse_binary_expression(token, previous, parse_expression(NULL))
                : (Expression *)parse_unary_expression(token, Expression_Rhs, parse_expression(NULL));
        break;

    case Token_Star:
        expression = previous != NULL
                         ? (Expression *)parse_binary_expression(token, previous, parse_expression(NULL))
                         : (Expression *)parse_deref_expression(token, parse_expression(NULL));
        break;

    case Token_Ampersand:
        expression = previous != NULL
                         ? (Expression *)parse_binary_expression(token, previous, parse_expression(NULL))
                         : (Expression *)parse_address_expression(token, parse_expression(NULL));
        break;

    case Token_Div:
    case Token_Mod:
    case Token_Bin_Or:
    case Token_Bin_Xor:
    case Token_Shift_L:
    case Token_Shift_R:
    case Token_Eq:
    case Token_Not_Eq:
    case Token_Less:
    case Token_Less_Eq:
    case Token_Greater:
    case Token_Greater_Eq:
    case Token_Add_Assign:
    case Token_Sub_Assign:
    case Token_Mul_Assign:
    case Token_Div_Assign:
    case Token_Mod_Assign:
    case Token_Shift_L_Assign:
    case Token_Shift_R_Assign:
    case Token_Bin_And_Assign:
    case Token_Bin_Xor_Assign:
    case Token_Bin_Or_Assign:
        expression = parse_binary_expression(token, previous, parse_expression(NULL));
        break;

    case Token_Paren_Begin:
        expression = previous != NULL ? (Expression *)parse_invoke_expression(token, previous)
                                      : (Expression *)parse_nested_expression(token);
        break;

    default:
        expression = NULL;
    }

    if (!expression)
        throw errorf("expected expression", token);
    if (peek(Token_Mask_Expression).ok)
        return parse_expression(expression);

    return expression;
}

Comma_Expression *Parser::parse_comma_expression(Token token, Expression *expression)
{
    Comma_Expression *comma_expression = ast.push(new Comma_Expression{});
    comma_expression->expression = expression;
    comma_expression->next = parse_expression(NULL);
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

Unary_Expression *Parser::parse_increment_expression(Token operation, Expression *previous)
{
    Expression_Order order;
    Expression *operand;

    if (previous != NULL) {
        order = Expression_Lhs;
        operand = previous;
    } else {
        order = Expression_Rhs;
        operand = parse_expression(NULL);
    }

    Object *object = ast.decode_designated_expression(operand);
    if (!object or !object->has_assign()) {
        throw errorf("operand should be assignable", operation);
    }
    return parse_unary_expression(operation, order, operand);
}

Unary_Expression *Parser::parse_unary_expression(Token operation, Expression_Order order, Expression *operand)
{
    Unary_Expression *unary_expression = ast.push(new Unary_Expression{});
    unary_expression->order = order;
    unary_expression->operand = operand;
    unary_expression->operation = operation;
    unary_expression->type = type_system.expression_type(operand);

    if (unary_expression->type->kind & ~Type_Scalar) {
        throw errorf("cannot use operand on expression of type '{}'", operation,
                     type_system.name(unary_expression->type));
    }

    return unary_expression;
}

Expression *Parser::parse_binary_expression(Token operation, Expression *lhs, Expression *rhs)
{
    Binary_Expression *binary_expression = ast.push(new Binary_Expression{});
    binary_expression->operation = operation;
    binary_expression->lhs = lhs;
    binary_expression->rhs = rhs;

    if (!binary_expression->lhs)
        throw errorf("missing left-hand side expression in binary expression", operation);

    Type *lhs_type = type_system.expression_type(binary_expression->lhs);
    Type *rhs_type = type_system.expression_type(binary_expression->rhs);
    binary_expression->type = lhs_type;

    if (lhs_type->kind & ~Type_Scalar)
        throw errorf("left-hand side expression is not a scalar", operation);
    if (rhs_type->kind & ~Type_Scalar)
        throw errorf("right-hand side expression is not a scalar", operation);

    binary_expression->rhs = cast_if_needed(operation, binary_expression->rhs, lhs_type);

    if (operation.type & Token_Mask_Binary_Assign) {
        Object *object = ast.decode_designated_expression(binary_expression->lhs);
        if (!object or !object->has_assign()) {
            throw errorf("left-hand side expression is not assignable", operation);
        }
    }
    if (operation.type & (Token_Mod | Token_Mod_Assign | Token_Mask_Bin) and
        binary_expression->type->kind & (Type_Float | Type_Double)) {
        throw errorf("cannot perform binary operation '{}' on floating type '{}'", operation, operation.str,
                     type_system.name(binary_expression->type));
    }

#if 0
    int32 lhs_precedence = type_system.expression_precedence(binary_expression->lhs);
    int32 rhs_precedence = type_system.expression_precedence(binary_expression->rhs);
    if (rhs_precedence < lhs_precedence) {
        std::swap(binary_expression->rhs, binary_expression->lhs);
    }
#endif

    return binary_expression;
}

Nested_Expression *Parser::parse_nested_expression(Token token)
{
    Nested_Expression *nested_expression = ast.push(new Nested_Expression{});
    nested_expression->operand = parse_expression(NULL);
    expect(Token_Paren_End, "closing nested expression");
    return nested_expression;
}

Argument_Expression *Parser::parse_argument_expression(Token token, Function *function,
                                                       Define_Statement *parameter)
{
    Argument_Expression *argument_expression = ast.push(new Argument_Expression{});
    argument_expression->assign_expression =
        parse_designated_assign_expression(token, parameter->variable, parse_expression(NULL));

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
    }
    return invoke_expression;
}

Assign_Expression *Parser::parse_assign_expression(Token token, Expression *lhs, Expression *rhs)
{
    Object *object = ast.decode_designated_expression(lhs);
    if (!object or object->kind() != Object_Variable or !object->has_assign()) {
        throw errorf("cannot assign expression", token);
    }

    return parse_designated_assign_expression(token, (Variable *)object, rhs);
}

Assign_Expression *Parser::parse_designated_assign_expression(Token token, Variable *variable,
                                                              Expression *expression)
{
    if (variable->type.kind & (Type_Struct | Type_Union)) {
        return parse_struct_copy_expression(token, variable, expression);
    }
    if (variable->type.kind & (Type_Array)) {
        return parse_array_copy_expression(token, variable, expression);
    }
    if (variable->type.kind & (Type_Scalar)) {
        return parse_scalar_copy_expression(token, variable, expression);
    }

    qcc_assert("parse_designated_assign_expression() not implemented for this type", 0);
    return NULL;
}

Assign_Expression *Parser::parse_scalar_copy_expression(Token token, Variable *variable,
                                                        Expression *expression)
{
    Assign_Expression *assign_expression = ast.push(new Assign_Expression{});
    assign_expression->variable = variable;
    assign_expression->type = &assign_expression->variable->type;
    assign_expression->expression = cast_if_needed(token, expression, assign_expression->type);

    return assign_expression;
}

Assign_Expression *Parser::parse_struct_copy_expression(Token token, Variable *variable,
                                                        Expression *expression)
{
    Variable *expression_variable = (Variable *)ast.decode_designated_expression(expression);
    if (!expression_variable or expression_variable->kind() != Object_Variable) {
        throw errorf("cannot assign expression", token);
    }

    Struct_Statement *destination = variable->type.struct_statement;
    Struct_Statement *source = expression_variable->type.struct_statement;

    if (destination->hash != source->hash) {
        throw errorf("cannot assign expression", token);
    }

    Assign_Expression *assign_expression_head = NULL;
    Assign_Expression *assign_expression_previous = NULL;

    for (std::string_view name : views::keys(source->members)) {
        Assign_Expression *assign_expression = parse_designated_assign_expression(
            token, destination->members[name],
            parse_designated_dot_expression(token, expression_variable, source->members[name]));

        if (!assign_expression_head)
            assign_expression_head = assign_expression;
        if (assign_expression_previous)
            assign_expression_previous->next = assign_expression;
        assign_expression_previous = assign_expression;
    }

    return assign_expression_head;
}

Assign_Expression *Parser::parse_array_copy_expression(Token token, Variable *variable,
                                                       Expression *expression)
{
    qcc_assert("TODO! parse_assign_expression_with_array()", 0);
    return NULL;
}

Cast_Expression *Parser::parse_cast_expression(Token token, Expression *expression, Type *type)
{
    Cast_Expression *cast_expression = ast.push(new Cast_Expression{});
    cast_expression->expression = expression;
    cast_expression->from = type_system.expression_type(expression);
    cast_expression->into = type;

    if (!cast_expression->expression)
        throw errorf("missing expression for type cast expression", token);
    if (!cast_expression->into)
        throw errorf("missing type for type cast expression", token);

    return cast_expression;
}

Dot_Expression *Parser::parse_dot_expression(Token token, Expression *previous)
{
    Dot_Expression *dot_expression = ast.push(new Dot_Expression{});

    dot_expression->record = (Variable *)ast.decode_designated_expression(previous);
    if (!dot_expression->record or dot_expression->record->kind() & ~Object_Variable) {
        throw errorf("variable expected before '.'", token);
    }

    dot_expression->type = dot_expression->record->type.base();
    if (dot_expression->type->kind & ~(Type_Struct | Type_Union)) {
        throw errorf("struct or union expected before '.'", token);
    }

    Token name = expect(Token_Id, "after '.' member access operator");
    Struct_Statement *struct_statement = dot_expression->type->struct_statement;

    if (!struct_statement->members.contains(name.str)) {
        std::string type_name = type_system.name(&dot_expression->record->type);
        throw errorf("member '{}.{}' is undeclared", name, type_name, name.str);
    }

    dot_expression->member = struct_statement->members[name.str];
    if (!dot_expression->member) {
        std::string type_name = type_system.name(&dot_expression->record->type);
        throw errorf("member '{}.{}' is not a variable member", name, type_name, name.str);
    }

    return dot_expression;
}

Dot_Expression *Parser::parse_designated_dot_expression(Token token, Variable *record, Variable *member)
{
    Dot_Expression *dot_expression = ast.push(new Dot_Expression{});

    dot_expression->record = record;
    dot_expression->member = member;
    dot_expression->type = record->type.base();
    return dot_expression;
}

Deref_Expression *Parser::parse_deref_expression(Token token, Expression *operand)
{
    Deref_Expression *deref_expression = ast.push(new Deref_Expression);
    deref_expression->object = ast.decode_designated_expression(operand);

    switch (deref_expression->object->kind()) {
    case Object_Variable: {
        Variable *variable = (Variable *)deref_expression->object;
        if (variable->type.kind != Type_Pointer) {
            throw errorf("cannot dereference a non-pointer expression", token);
        }
        deref_expression->type = variable->type.pointed_type;
        break;
    }

    default:
        qcc_assert("TODO! parse_address_expression() does not support object kind", 0);
    }

    return deref_expression;
}

Address_Expression *Parser::parse_address_expression(Token token, Expression *operand)
{
    Address_Expression *address_expression = ast.push(new Address_Expression);
    address_expression->object = ast.decode_designated_expression(operand);

    if (!address_expression->object->has_address()) {
        throw errorf("object is not addressable", token);
    }

    switch (address_expression->object->kind()) {
    case Object_Variable: {
        Variable *variable = (Variable *)address_expression->object;
        address_expression->type = Type{};
        address_expression->type.kind = Type_Pointer;
        address_expression->type.size = 8;
        address_expression->type.pointed_type = &variable->type;
        break;
    }

    default:
        qcc_assert("TODO! parse_address_expression() does not support object kind", 0);
    }

    return address_expression;
}

Expression *Parser::cast_if_needed(Token token, Expression *expression, Type *type)
{
    Type *expression_type = type_system.expression_type(expression);
    uint32 type_cast = type_system.cast(expression_type, type);

    if (type_cast & Type_Cast_Error) {
        throw errorf("cannot cast type '{}' into '{}'", token, type_system.name(expression_type),
                     type_system.name(type));
    }
    if (type_cast > Type_Cast_Same) {
        return parse_cast_expression(token, expression, type);
    }
    return expression;
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
            Unary(Token_Bin_Not, ~);
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
            Binary(Token_Bin_And, &);
            Binary(Token_Bin_Or, |);
            Binary(Token_Shift_L, <<);
            Binary(Token_Shift_R, >>);
            Binary(Token_Bin_Xor, ^);
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

bool Parser::is_eof() const
{
    return scanner.next.empty() and token_queue.empty();
}

Token Parser::peek_until(int128 mask)
{
    Token token = peek(mask);

    if (!token.ok and !is_eof())
        return token;
    if (!token.ok)
        throw errorf("unexpected end of file", token);
    return token;
}

Token Parser::peek(int128 mask)
{
    Token token = {};

    if (!token_queue.empty()) {
        token = token_queue.front();
    } else {
        token = token_queue.emplace_back(scanner.tokenize(Token_Comment | Token_Blank));
    }

    token.ok = token.type & mask;
    return token;
}

Token Parser::scan(int128 mask)
{
    Token token = {};

    if (token_queue.size() != 0) {
        token = token_queue.front();
        token_queue.pop_front();
    } else {
        token = scanner.tokenize(Token_Comment | Token_Blank);
    }

    token.ok = token.type & mask;
    if (!token.ok and token.type != Token_Eof) {
        token_queue.emplace_back(token);
    }
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
