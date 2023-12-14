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

const int128 Token_Mask_Expression = Token_Id | Token_Char | Token_String | Token_Int | Token_Int_Bin | Token_Int_Hex |
                                     Token_Float | Token_Increment | Token_Decrement | Token_Div | Token_Mod |
                                     Token_Not | Token_Bin_Not | Token_Bin_And | Token_Bin_Or | Token_Bin_Xor |
                                     Token_Shift_L | Token_Shift_R | Token_Eq | Token_Not_Eq | Token_Less |
                                     Token_Less_Eq | Token_Greater | Token_Greater_Eq;

const int128 Token_Mask_Statement = Token_Scope_Begin | Token_If | Token_While | Token_For;

const int128 Token_Mask_Each = ~((int128)0);

const int128 Token_Mask_Type = Token_Auto | Token_Long | Token_Short | Token_Volatile | Token_Const | Token_Extern |
                               Token_Register | Token_Static | Token_Signed | Token_Unsigned | Token_Int_Type |
                               Token_Char_Type | Token_Float_Type | Token_Double_Type;

const int128 Token_Mask_Type_Storage = Token_Const | Token_Volatile | Token_Register | Token_Static;
const int128 Token_Mask_Fundamental = Token_Int_Type | Token_Char_Type | Token_Float_Type | Token_Double_Type;
const int128 Token_Mask_Record = Token_Struct | Token_Union | Token_Enum;

Parser::Parser(Ast &ast, Scanner &scanner) : ast(ast), scanner(scanner), type_system(ast.type_system)
{
    type_system.init();
}

Statement *Parser::parse()
{
    return (ast.main_statement = parse_scope_statement(Token_Eof));
}

Type Parser::parse_type()
{
    Token token = {};
    Type type = {};
    type.size = -1;
    bool type_mods_allowed = true;

    while ((token = scan(Token_Mask_Type | Token_Mask_Record | Token_Id)).ok) {
        if (token.type & Token_Id and type.kind != Type_Undefined)
            break;

        if (token.type & Token_Const)
            type.cvr |= Type_Const;
        if (token.type & Token_Volatile)
            type.cvr |= Type_Volatile;
        if (token.type & Token_Restrict)
            type.cvr |= Type_Restrict;

        if (token.type & (Token_Short | Token_Long | Token_Signed | Token_Unsigned)) {
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

        if (token.type & (Token_Extern | Token_Register | Token_Static | Token_Auto)) {
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

        if (token.type & (Token_Mask_Fundamental)) {
            if (type.kind != Type_Undefined)
                throw errorf("cannot combine type with '{}'", token, type.token.str);
            type.kind = token_to_type_kind(token.type);
        }

        if (token.type & Token_Id) {
            Typedef *type_def = (Typedef *)scope_in()->object(token.str);
            if (!type_def or type_def->kind() != Object_Typedef)
                throw errorf("undefined type", token);
            type_system.copy(&type, &type_def->type);
            type_mods_allowed = false;
        }

        if (token.type & (Token_Mask_Record)) {
            Record_Statement *record_statement = parse_record_statement(token);
            if (record_statement->kind() & Statement_Struct) {
                Struct_Statement *struct_statement = (Struct_Statement *)record_statement;
                type_system.copy(&type, struct_statement->type);
            }
            if (record_statement->kind() & Statement_Enum) {
                Enum_Statement *enum_statement = (Enum_Statement *)record_statement;
                type_system.copy(&type, enum_statement->type);
            }
            type_mods_allowed = false;
        }
        type.token |= token;
    }

    if (!type_mods_allowed and type.mods != 0)
        throw errorf("type modifiers are not allowed in this type declaration", type.token);
    if (!type.kind and !type.mods)
        throw errorf("no type specified in declaration", type.token);
    if (!type.kind and type.mods != 0)
        type.kind = Type_Int;
    if (type.kind & (Type_Scalar) and type.size < 0)
        type.size = type_system.scalar_size(type.kind, type.mods);
    if (type.kind & (Type_Record) and type.mods != 0)
        throw errorf("type modifiers are not allowed in record type declaration", type.token);
    if (type.mods != 0)
        throw errorf("cannot declare type '{}' with modifiers", token, token.str);

    qcc_assert("cannot parse sizeof type", type.size != -1);
    return type;
}

Statement *Parser::parse_statement()
{
    Token token = peek(Token_Mask_Each);

    if (token.type & Token_Scope_Begin)
        return parse_scope_statement(Token_Scope_End);
    if (token.type & Token_If)
        return parse_condition_statement();
    if (token.type & Token_While)
        return parse_while_statement();
    if (token.type & Token_For)
        return parse_for_statement();
    if (token.type & (Token_Mask_Expression))
        return parse_expression_statement();
    if (token.type & (Token_Mask_Type))
        return parse_define_or_function_statement();
    if (token.type & Token_Id) {
        if (Typedef *type_def = (Typedef *)scope_in()->object(token.str);
            type_def != NULL and type_def->kind() & Object_Typedef) {
            return parse_define_or_function_statement();
        }
    }
    qcc_assert("non-reachable", false);
}

Scope_Statement *Parser::parse_scope_statement(int128 end_mask)
{
    Scope_Statement *scope = ast.push(new Scope_Statement);
    scope->owner = scope_in();
    stack.push_back(scope->owner);

    while (peek_until(end_mask)) {
        Statement *statement = parse_statement();
        scope->body.push_back(statement);
    }

    stack.pop_back();
    return scope;
}

Statement *Parser::parse_define_or_function_statement()
{
    Type type = parse_type();
    Token name = expect(Token_Id | Token_Semicolon, "after type in definition");

    if (name.type & Token_Semicolon)
	return NULL;
    if (peek(Token_Paren_Begin).ok)
        return parse_function_statement(type, name);
    else
        return parse_define_statement(type, name, Parse_Define_Variable, NULL);
}

Define_Statement *Parser::parse_define_statement(Type type, Token name, Parse_Define_Type define_type,
                                                 Define_Statement *previous)
{
    Define_Statement *define_statement = ast.push(new Define_Statement);
    define_statement->name = name;
    define_statement->type = type;

    if (define_type & (Parse_Define_Variable | Parse_Define_Enum) and scan(Token_Assign).ok) {
        define_statement->expression = parse_expression(NULL);
    }

    if (define_type & (Parse_Define_Variable | Parse_Define_Enum)) {
        if (scope_in()->object(name.str) != NULL)
            throw errorf("redefinition of '{}'", name, name.str);
    }
    if (define_type & (Parse_Define_Struct | Parse_Define_Union)) {
        if (scope_in()->objects.contains(name.str))
            throw errorf("redefinition of member '{}'", name, name.str);
    }

    define_statement->variable = new Variable;
    define_statement->variable->name = name;
    define_statement->variable->type = type;

    if (define_type & Parse_Define_Enum) {
        int64 &constant = define_statement->variable->constant;
        if (define_statement->expression != NULL) {
            constant = parse_constant(name, define_statement->expression);
        } else {
            constant = previous != NULL ? previous->variable->constant + 1 : 0;
        }
    }
    if (define_type & Parse_Define_Struct and previous != NULL) {
        int64 offset = previous->variable->offset + previous->type.size;
        define_statement->variable->offset = offset;
    }
    if (define_type & Parse_Define_Union) {
        define_statement->variable->offset = 0;
    }

    scope_in()->objects[name.str] = define_statement->variable;

    Token sep = expect(Token_Comma | Token_Semicolon, "between variable definitions");
    if (sep.type & Token_Comma) {
        Token comma_name = expect(Token_Id, "after definition comma");
        define_statement->next = parse_define_statement(type, comma_name, define_type, define_statement);
    }
    return define_statement;
}

Function_Statement *Parser::parse_function_statement(Type return_type, Token name)
{
    Function_Statement *function_statement = ast.push(new Function_Statement);
    Object *prototype = scope_in()->object(name.str);

    if (scope_in()->owner != NULL) {
        throw errorf("cannot define function in nested statement", name);
    }

    Function function = {};
    function.name = name;
    function.return_type = return_type;

    // Token paren_begin = expect(Token_Paren_Begin, "after function name");

    // function.parameters = parse_define_statement(

    // ast.push(new Function);
}

Record_Statement *Parser::parse_record_statement(Token keyword)
{
    if (keyword.type & (Token_Struct | Token_Union))
        return parse_struct_statement(keyword);
    if (keyword.type & (Token_Enum))
        return parse_enum_statement();
    qcc_assert("expected 'enum', 'union', or 'struct' keyword", 0);
}

Record_Statement *Parser::parse_struct_statement(Token keyword)
{
    Record_Statement *struct_statement = ast.push(new Record_Statement);

    if (keyword.type & ~(Token_Struct | Token_Union)) {
        throw errorf("expected 'struct' or 'enum' keyword", keyword);
    }
    Token name = expect(Token_Id, "in struct statement");
    Type_Kind type_kind = token_to_type_kind(keyword.type);
    Record *record = scope_in()->record(type_kind, name.str);
    Token scope_begin = peek(Token_Scope_Begin);

    if (record != NULL) {
        if (record->type.kind != type_kind)
            throw errorf("'{}' is not defined as {}", name, keyword.str);
        if (scope_begin.ok)
            throw errorf("redefinition of {} '{}'", name, keyword);
    } else {
        if (!scope_begin.ok)
            throw errorf("unknown {} '{}'", name, keyword.str);

        record = new Record;
        Type *type = &record->type;
        type->kind = token_to_type_kind(keyword.type);
        type->scope = parse_record_scope_statement(keyword);
        type->size = type_system.struct_size(type);
    }

    struct_statement->type = &record->type;
    return struct_statement;
}

Scope_Statement *Parser::parse_record_scope_statement(Token keyword)
{
    Scope_Statement *scope = ast.push(new Scope_Statement);
    scope->owner = scope_in();
    stack.push_back(scope->owner);

    qcc_assert("expected '{' in scope statement", scan(Token_Scope_Begin).ok);
    while (peek_until(Token_Scope_End)) {
        Parse_Define_Type define_type;
        if (keyword.type & Token_Struct)
            define_type = Parse_Define_Struct;
        if (keyword.type & Token_Union)
            define_type = Parse_Define_Union;
        if (keyword.type & Token_Enum)
            define_type = Parse_Define_Enum;

        Type type = parse_type();
        Define_Statement *define_statement = parse_define_statement(type, scan(Token_Id), define_type, NULL);
        scope->body.push_back(define_statement);
    }

    stack.pop_back();
    return scope;
}

Enum_Statement *Parser::parse_enum_statement() {}

Statement *Parser::parse_scope_or_expression_statement()
{
    if (scan(Token_Scope_Begin).ok)
        return parse_scope_statement(Token_Scope_End);
    else
        return parse_expression_statement();
}

Expression *Parser::parse_boolean_expression()
{
    Token paren_begin = expect(Token_Paren_Begin, "before nested boolean expression");
    Expression *boolean_expression = parse_expression(NULL);
    Token paren_end = expect(Token_Paren_End, "after nested boolean expression");

    Type *type = type_system.expression_type(boolean_expression);
    if (type->kind & ~(Type_Scalar)) {
        throw errorf("expression must reduce to a scalar", paren_begin | paren_end);
    }
    return boolean_expression;
}

Condition_Statement *Parser::parse_condition_statement()
{
    qcc_assert("expected 'if' or 'else' token", scan(Token_If | Token_Else).ok);

    Condition_Statement *condition_statement = ast.push(new Condition_Statement);
    condition_statement->boolean = parse_boolean_expression();
    condition_statement->statement_if = parse_scope_or_expression_statement();
    if (scan(Token_Else).ok)
        condition_statement->statement_else = parse_scope_or_expression_statement();
    return condition_statement;
}

While_Statement *Parser::parse_while_statement()
{
    qcc_assert("expected 'while' token", scan(Token_If | Token_Else).ok);

    While_Statement *while_statement = ast.push(new While_Statement);
    while_statement->boolean = parse_boolean_expression();
    while_statement->statement = parse_scope_or_expression_statement();
    return while_statement;
}

For_Statement *Parser::parse_for_statement()
{
    qcc_assert("expected 'for' token", scan(Token_If | Token_Else).ok);

    For_Statement *for_statement = ast.push(new For_Statement);

    Token paren_begin = expect(Token_Paren_Begin, "before init expression");
    for_statement->init = parse_expression(NULL);
    Token semicolon_init = expect(Token_Paren_Begin, "after init expression");
    for_statement->boolean = parse_expression(NULL);
    Token semicolon_boolean = expect(Token_Paren_Begin, "after boolean expression");
    for_statement->loop = parse_expression(NULL);
    Token paren_end = expect(Token_Paren_Begin, "after loop expression");
    for_statement->statement = parse_scope_or_expression_statement();

    return for_statement;
}

Expression_Statement *Parser::parse_expression_statement()
{
    Expression_Statement *statement = ast.push(new Expression_Statement);
    statement->expression = NULL;

    while (peek_until(Token_Semicolon)) {
        statement->expression = parse_expression(statement->expression);
    }

    Token semicolon = scan(Token_Semicolon);
    if (!semicolon.ok) {
        throw errorf("expected ';' after expression statement", semicolon);
    }
    return statement;
}

Expression *Parser::parse_expression(Expression *previous)
{
    Expression *expression = NULL;

    if (Token sign; !previous and (sign = scan(Token_Add | Token_Sub)).ok) {
        expression = parse_unary_expression(sign, Expression_Rhs, parse_expression(NULL));
    }

    Token token = scan(Token_Mask_Expression);
    if (!token.ok) {
        throw errorf("unexpected token", token);
    }

    switch (token.type) {
    case Token_Id:
        expression = parse_id_expression(token);
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
        if (previous != NULL)
            expression = parse_unary_expression(token, Expression_Lhs, previous);
        else
            expression = parse_unary_expression(token, Expression_Rhs, parse_expression(NULL));
        break;

    case Token_Not:
    case Token_Bin_Not:
        expression = parse_unary_expression(token, Expression_Rhs, parse_expression(NULL));
        break;

    case Token_Comma:
        expression = parse_comma_expression(token, previous);
        break;

    case Token_Div:
    case Token_Mod:
    case Token_Bin_And:
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
        expression = parse_binary_expression(token, previous, parse_expression(NULL));
        break;

    default:
        expression = NULL;
    }

    if (peek(Token_Mask_Expression).ok)
        return parse_expression(expression);
    return expression;
}

Comma_Expression *Parser::parse_comma_expression(Token token, Expression *lhs)
{
    Comma_Expression *comma_expression = ast.push(new Comma_Expression);

    comma_expression->lhs = lhs;
    comma_expression->rhs = parse_expression(NULL);

    if (!comma_expression->lhs)
        throw errorf("missing left-hand side expression", token);
    if (!comma_expression->rhs)
        throw errorf("missing right-hand side expression", token);
    return comma_expression;
}

Id_Expression *Parser::parse_id_expression(Token token)
{
    Object *object = scope_in()->object(token.str);
    if (!object)
        throw errorf("use of unknown identifier", token);
    if (object->kind() != Object_Variable)
        throw errorf("identifier does not refer to a variable", token);

    Id_Expression *id_expression = ast.push(new Id_Expression);
    id_expression->object = object;
    id_expression->token = token;
    return id_expression;
}

Int_Expression *Parser::parse_int_expression(Token token)
{
    Int_Expression *int_expression = ast.push(new Int_Expression);
    Type *type = &int_expression->type;

    if (token.type & Token_Char) {
        std::string_view sequence = token.str.substr(1, token.str.size() - 1);
        std::string content = escape_string(sequence, 1);
        if (content.empty())
            throw errorf("empty character constant", token);
        if (content.size() > 1)
            throw errorf("multibyte character constant", token);
        int_expression->value = content[0];
        type->kind = Type_Char;
        type->size = 1;
    }
    if (token.type & (Token_Int | Token_Int_Bin | Token_Int_Hex)) {
        const char *number_begin = token.str.begin();
        const char *number_end = token.str.end();

        if (token.type & (Token_Int_Bin | Token_Int_Hex))
            number_begin = number_begin + 2;
        for (; number_end > number_begin and std::isalpha(*(number_end - 1)); number_end--)
            ;

        std::string_view suffix = {number_end + 1, token.str.end()};
        int_expression->flags = parse_int_flags(token, suffix);

        type->kind = Type_Int;
        if (int_expression->flags & (Int_L))
            type->mods |= Type_Long;
        if (int_expression->flags & (Int_U))
            type->mods |= Type_Unsigned;
        type->size = type_system.scalar_size(type->kind, type->mods);

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

    qcc_assert("cannot size int expression", type->size < 0);
    return int_expression;
}

uint32 Parser::parse_int_flags(Token token, std::string_view suffix)
{
    uint32 flags = 0;

    auto parse_flag = [&](std::string_view lowercase, std::string_view uppercase, uint32 includes, uint32 excludes) {
        if (!suffix.starts_with(lowercase) and !suffix.starts_with(uppercase))
            return;
        if (flags & (includes | excludes))
            throw errorf("invalid suffix '{}' on integer constant", token, suffix);
        flags |= includes;
        suffix.remove_prefix(lowercase.size());
    };

    while (!suffix.empty()) {
        parse_flag("ll", "LL", Int_LL, Int_L);
        parse_flag("l", "L", Int_L, Int_LL);
        parse_flag("u", "U", Int_U, 0);
    }
    return flags;
}

Float_Expression *Parser::parse_float_expression(Token token)
{
    Float_Expression *float_expression = ast.push(new Float_Expression);
    Type *type = &float_expression->type;

    const char *number_begin = token.str.begin();
    const char *number_end = token.str.end();
    char flag = 0;

    if (std::isalpha(number_end[-1]))
        flag = *--number_end;
    switch (flag) {
    case 'f':
    case 'F':
        type->kind = Type_Float;
        break;
    case 0:
    case 'l':
    case 'L':
        type->kind = Type_Double;
        break;
    }
    type->size = type_system.scalar_size(type->kind, type->mods);

    std::from_chars_result status = {0};
    if (token.type & Token_Float) {
        status = std::from_chars(number_begin, number_end, float_expression->value);
    }
    if (status.ec != std::errc{}) {
        std::string why = std::error_condition{status.ec}.message();
        throw errorf("cannot parse float constant ({})", token, why);
    }

    qcc_assert("cannot size float expression", type->size < 0);
    return float_expression;
}

String_Expression *Parser::parse_string_expression(Token token)
{
    String_Expression *string_expression = ast.push(new String_Expression);

    // Chop string quotes
    std::string_view raw_string = token.str.substr(1, token.str.size() - 2);
    string_expression->string = un_escape_string(raw_string, 1);
    return string_expression;
}

Unary_Expression *Parser::parse_unary_expression(Token operation, Expression_Order order, Expression *operand)
{
    Unary_Expression *unary_expression = ast.push(new Unary_Expression);
    unary_expression->order = order;
    unary_expression->operand = operand;
    unary_expression->operation = operation;

    if (!unary_expression->operand) {
        throw errorf("missing operand for operation '{}'", operation);
    }

    Type *operand_type = type_system.expression_type(operand);
    if (operand_type->kind & ~(Type_Scalar)) {
        throw errorf("cannot use operand on expression of type '{}'", operation, type_system.name(operand_type));
    }

    return unary_expression;
}

Binary_Expression *Parser::parse_binary_expression(Token operation, Expression *lhs, Expression *rhs)
{
    Binary_Expression *binary_expression = ast.push(new Binary_Expression);
    binary_expression->operation = operation;
    binary_expression->lhs = lhs;
    binary_expression->rhs = rhs;

    if (!binary_expression->lhs)
        throw errorf("missing left-hand side expression in binary expression", operation);
    if (!binary_expression->rhs)
        throw errorf("missing right-hand side expression in binary expression", operation);

    Type *lhs_type = type_system.expression_type(binary_expression->lhs);
    Type *rhs_type = type_system.expression_type(binary_expression->rhs);

    if (lhs_type->kind & ~(Type_Scalar)) {
        throw errorf("left-hand side expession is not a scalar", operation);
    } else {
        throw errorf("right-hand side expession is not a scalar", operation);
    }

    return binary_expression;
}

int64 Parser::parse_constant(Token token, Expression *expression)
{
    if (expression->kind() & Expression_Unary) {
        Unary_Expression *expression = (Unary_Expression *)expression;

#define Unary(type, op) \
    case type:          \
        return op(parse_constant(token, expression->operand))

        switch (expression->operation.type) {
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
        Binary_Expression *expression = (Binary_Expression *)expression;

#define Binary(type, op) \
    case type:           \
        return (parse_constant(token, expression->lhs) op parse_constant(token, expression->rhs))

        switch (expression->operation.type) {
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
        Int_Expression *expression = (Int_Expression *)expression;
        return expression->value;
    }

    if (expression->kind() & Expression_Nested) {
        Nested_Expression *expression = (Nested_Expression *)expression;
        return parse_constant(token, expression->operand);
    }

    if (expression->kind() & Expression_Ternary) {
        Ternary_Expression *expression = (Ternary_Expression *)expression;
        bool boolean = parse_constant(token, expression->boolean);
        return boolean ? parse_constant(token, expression->expression_if)
                       : parse_constant(token, expression->expression_else);
    }

    throw errorf("expression is not an integer constant", token);
}

Scope_Statement *Parser::scope_in()
{
    return stack.back();
}

bool Parser::is_eof() const
{
    return scanner.next.empty() and token_queue.empty();
}

bool Parser::peek_until(int128 mask)
{
    Token end = {};

    if (!(end = peek(mask)).ok and !is_eof())
        return true;
    if (!(end.ok))
        throw errorf("unexpected end of file", end);
    return false;
}

Token Parser::peek(int128 mask)
{
    Token token = {};

    if (token_queue.size() != 0) {
        token = token_queue.front();
    } else {
        token = token_queue.emplace_back(scanner.tokenize());
    }

    if (!token.type) {
        throw errorf("unrecognized token", token);
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
        token = scanner.tokenize();
    }

    token.ok = token.type & mask;
    if (!token.type) {
        throw errorf("unrecognized token", token);
    }
    if (!token.ok and token.type != Token_Eof) {
        token_queue.emplace_back(token);
    }

    return token;
}

Token Parser::expect(int128 mask, std::string_view context)
{
    Token token = scan(mask);

    if (!token.ok) {
        std::vector<std::string_view> expected = {};
        for (int128 type = 1; type != Token_Type_End; type <<= 1) {
            if (type & mask)
                expected.push_back(token_type_str((Token_Type)type));
        }
        throw errorf("expected token {} {}", token, fmt::join(expected, ", "), context);
    }

    return token;
}

} // namespace qcc
