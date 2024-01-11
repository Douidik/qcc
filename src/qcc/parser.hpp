#ifndef QCC_PARSER_HPP
#define QCC_PARSER_HPP

#include "fwd.hpp"
#include "operators.hpp"
#include "source_snippet.hpp"
#include "type_system.hpp"
#include <deque>

namespace qcc
{

struct Declarator
{
    Type type;
    Token name;
    bool has_matched_function;
};

// This refers to what's commonly called values categories (lvalue, rvalue, funciton designators)
enum Expression_Category
{
    Expression_L,
    Expression_R,
    Expression_Fd,
};

struct Parser
{
    Ast &ast;
    Token *source;
    Type_System type_system;
    std::deque<Statement *> context;
    bool verbose;

    Parser(Ast &ast, Token *source, bool verbose);

    Statement *parse();
    Statement *parse_statement();
    void parse_type_cvr(Type *type, Token token, uint32 cvr_allowed);
    Type parse_type();

    Declarator parse_declarator(Type type_base, Define_Env env);
    Type parse_pointer_declarator(Type pointed_type);
    Type parse_array_declarator(Type array_type);

    Type *parse_struct_type(Token keyword);
    Struct_Statement *parse_struct_statement(Token keyword);

    Statement *parse_define_statement(Type type_base, Define_Env env, Define_Statement *previous,
                                      int128 end_mask);
    Define_Statement *parse_comma_define_statement(Define_Statement *define_statement, Type type_base,
                                                   Define_Env env, int128 end_mask);
    Function_Statement *parse_function_statement(Declarator declarator);
    Scope_Statement *parse_scope_statement(Scope_Statement *scope_statement, uint32 statement_mask,
                                           int128 end_mask);

    Record_Statement *parse_record_statement(Token keyword);
    Scope_Statement *parse_struct_scope_statement(Token keyword);
    Scope_Statement *parse_enum_scope_statement(Token keyword, Type *enum_type);

    Expression *parse_boolean_expression();
    Scope_Statement *parse_maybe_inlined_scope_statement();
    Condition_Statement *parse_condition_statement();
    While_Statement *parse_while_statement();
    For_Statement *parse_for_statement();
    Return_Statement *parse_return_statement();

    Expression_Statement *parse_expression_statement();
    Expression *parse_expression(Expression *previous = NULL, int32 precedence = Lowest_Precedence);
    Comma_Expression *parse_comma_expression(Token token, Expression *expression);
    Expression *parse_increment_expression(Token operation, Expression *operand, int32 precedence);
    Expression *parse_unary_expression(Token operation, Expression_Order order, Expression *operand,
                                       int32 precedence);
    Expression *parse_binary_expression(Token operation, Expression *lhs, int32 precedence);
    Expression *parse_binary_assign_expression(Token operation, Expression *lhs);
    Id_Expression *parse_id_expression(Token token);
    String_Expression *parse_string_expression(Token token);
    Int_Expression *parse_int_expression(Token token);
    Float_Expression *parse_float_expression(Token token);
    Nested_Expression *parse_nested_expression(Token token);
    Argument_Expression *parse_argument_expression(Token token, Function *function,
                                                   Define_Statement *parameter);
    Invoke_Expression *parse_invoke_expression(Token token, Expression *function_expression);
    Cast_Expression *parse_cast_expression(Token token, Expression *expression, Type type);
    Dot_Expression *parse_dot_expression(Token token, Expression *previous);
    Dot_Expression *parse_arrow_expression(Token token, Expression *previous);
    Deref_Expression *parse_deref_expression(Token token, Expression *operand);
    Address_Expression *parse_address_expression(Token token, Expression *operand);
    Expression *parse_subscript_expression(Token token, Expression *operand);
    Assign_Expression *parse_assign_expression(Token token, Expression *lhs, Expression *rhs);
    Ref_Expression *parse_ref_expression(Object *object, Type *type);

    Cast_Expression *cast_array_decay(Token token, Expression *expression);
    Expression *cast_if_needed(Token token, Expression *expression, Type type);
    Expression *typecheck_binary_operand(Expression *operand, Token operation);
    Expression *typecheck_binary_expression(Binary_Expression *binary_expression);
    Expression *typecheck_unary_operand(Expression *operand, Token operation);
    int64 parse_constant(Token token, Expression *expression);

    bool token_is_typedef(Token token);
    Expression_Category categorize_expression(Expression *expression);
    Scope_Statement *context_scope();
    Statement *context_of(uint32 statement_mask);
    Statement *context_push(Statement *statement);
    Statement *context_pop();

    Token peek_until(int128 mask);
    Token peek(int128 mask);
    Token scan(int128 mask);
    Token enqueue(Token token);
    Token expect(int128 mask, std::string_view context);

    Error errorf(std::string_view fmt, Token token, auto... args) const
    {
        return Error{"parser error", make_source_snippet(token, fmt, args...)};
    }
};

} // namespace qcc

#endif
