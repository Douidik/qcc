#ifndef QCC_PARSER_HPP
#define QCC_PARSER_HPP

#include "fwd.hpp"
#include "scan/scanner.hpp"
#include "source_snippet.hpp"
#include "type_system.hpp"

namespace qcc
{

enum Parse_Define_Type : uint32
{
    Parse_Define_Struct = Bit(uint32, 1),
    Parse_Define_Union = Bit(uint32, 2),
    Parse_Define_Enum = Bit(uint32, 3),
    Parse_Define_Parameter = Bit(uint32, 4),
    Parse_Define_Variable = Bit(uint32, 5),
};

struct Parser
{
    Ast &ast;
    Scanner &scanner;
    Type_System &type_system;
    std::deque<Token> token_queue;
    std::deque<Statement *> context;

    Parser(Ast &ast, Scanner &scanner);

    Statement *parse();
    Statement *parse_statement();
    Type parse_type();

    Statement *parse_define_or_function_statement();
    Define_Statement *parse_define_statement(Type type, Token name, Parse_Define_Type define_type,
                                             Define_Statement *previous, int128 end_mask);
    Define_Statement *parse_comma_define_statement(Define_Statement *define_statement,
                                                   Parse_Define_Type define_type, int128 end_mask);
    Function_Statement *parse_function_statement(Type return_type, Token name);
    Scope_Statement *parse_scope_statement(Scope_Statement *scope_statement, uint32 statement_mask,
                                           int128 end_mask);

    Record_Statement *parse_record_statement(Token keyword);
    Scope_Statement *parse_struct_scope_statement(Token keyword);
    Scope_Statement *parse_enum_scope_statement();

    Expression *parse_boolean_expression();
    Scope_Statement *parse_flow_scope_statement();
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
    Expression *parse_assign_expression(Token token, Expression *variable_expression);
    Cast_Expression *parse_cast_expression(Token token, Expression *expression, Type *type);
    Dot_Expression *parse_dot_expression(Token token, Expression *previous);

    void parse_scope_stack(std::vector<Variable *> &stack, Scope_Statement *scope_statement);
    void parse_function_stack(Function_Statement *function_statement);

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
