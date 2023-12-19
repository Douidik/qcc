#ifndef QCC_X86_HPP
#define QCC_X86_HPP

#include "asm.hpp"

namespace qcc
{

struct X86 : Asm
{
    X86(Ast &ast, std::string_view source, std::ostream &stream);

    void make() override;
    void make_statement(Statement *statement);
    void make_scope_statement(Scope_Statement *scope_statement);
    void make_function_statement(Function_Statement *function_statement);
    void make_define_statement(Define_Statement *define_statement);
    void make_condition_statement(Condition_Statement *condition_statement);
    void make_while_statement(While_Statement *while_statement);
    void make_for_statement(For_Statement *for_statement);
    void make_return_statement(Return_Statement *return_statement);
    
    
    void make_expression(Expression *expression);
    void make_int_expression(Int_Expression *int_expression);
    void make_id_expression(Id_Expression *id_expression);
    void make_binary_expression(Binary_Expression *binary_expression);
    void make_unary_expression(Unary_Expression *unary_expression);
    void make_assign_expression(Assign_Expression *assign_expression);
    void make_invoke_expression(Invoke_Expression *invoke_expression);
    
    void make_variable_load(Variable *variable, size_t offset = 0);
    void make_variable_store(Variable *variable, size_t offset = 0);
};

// enum X86_Instruction_Mod : uint32
// {
//     X86_F = Bit(uint32, 1),
//     X86_I = Bit(uint32, 1),
//     X86_I = Bit(uint32, 1),
// };

// struct X86_Instruction
// {
//     std::string_view name;
// };

} // namespace qcc

#endif
