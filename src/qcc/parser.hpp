#ifndef QCC_PARSER_HPP
#define QCC_PARSER_HPP

#include "fwd.hpp"
#include "scan/scanner.hpp"
#include "source_snippet.hpp"
#include "type_system.hpp"

namespace qcc
{

struct Parser
{
    Ast &ast;
    Scanner &scanner;
    Type_System type_system;
    std::deque<Token> token_queue;
    std::deque<Statement *> context;

    Parser(Ast &ast, Scanner &scanner);

    Statement *parse();
    Statement *parse_statement();
    void parse_type_cvr(Type *type, Token token, bool is_pointer_type);
    Type parse_type();
    Type parse_pointer_type(Type pointed_type);
    Type parse_struct_type(Token keyword);
    Struct_Statement *parse_struct_statement(Token keyword);

    Statement *parse_define_or_function_statement();
    Define_Statement *parse_define_statement(Type type, Token name, Define_Mode mode,
                                             Define_Statement *previous, int128 end_mask);
    Define_Statement *parse_comma_define_statement(Define_Statement *define_statement, Define_Mode mode,
                                                   int128 end_mask);
    Function_Statement *parse_function_statement(Type return_type, Token name);
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
    Expression *parse_expression(Expression *previous);
    Comma_Expression *parse_comma_expression(Token token, Expression *expression);
    Comma_Expression *parse_comma_node_expression(Token token, Expression *expression);
    Unary_Expression *parse_increment_expression(Token operation, Expression *previous);
    Unary_Expression *parse_unary_expression(Token operation, Expression_Order order, Expression *operand);
    Expression *parse_binary_expression(Token operation, Expression *lhs, Expression *rhs);
    Id_Expression *parse_id_expression(Token token);
    String_Expression *parse_string_expression(Token token);
    Int_Expression *parse_int_expression(Token token);
    Float_Expression *parse_float_expression(Token token);
    Nested_Expression *parse_nested_expression(Token token);
    Argument_Expression *parse_argument_expression(Token token, Function *function,
                                                   Define_Statement *parameter);
    Invoke_Expression *parse_invoke_expression(Token token, Expression *function_expression);
    Cast_Expression *parse_cast_expression(Token token, Expression *expression, Type *type);
    Dot_Expression *parse_dot_expression(Token token, Expression *previous);
    Dot_Expression *parse_designated_dot_expression(Token token, Variable *record, Variable *member);
    Deref_Expression *parse_deref_expression(Token token, Expression *operand);
    Address_Expression *parse_address_expression(Token token, Expression *operand);

    Assign_Expression *parse_assign_expression(Token token, Expression *lhs, Expression *rhs);
    Assign_Expression *parse_designated_assign_expression(Token token, Variable *variable,
                                                          Expression *expression);
    Assign_Expression *parse_scalar_copy_expression(Token token, Variable *variable, Expression *expression);
    Assign_Expression *parse_struct_copy_expression(Token token, Variable *variable, Expression *expression);
    Assign_Expression *parse_array_copy_expression(Token token, Variable *variable, Expression *expression);

    Expression *cast_if_needed(Token token, Expression *expression, Type *type);

    Scope_Statement *context_scope();
    Statement *context_of(uint32 statement_mask);
    int64 parse_constant(Token token, Expression *expression);

    bool is_eof() const;
    Token peek_until(int128 mask);
    Token peek(int128 mask);
    Token scan(int128 mask);
    Token expect(int128 mask, std::string_view context);

    Error errorf(std::string_view fmt, Token token, auto... args) const
    {
        return Error{"parser error", make_source_snippet(scanner.source, token, fmt, args...)};
    }
};

} // namespace qcc

#endif
