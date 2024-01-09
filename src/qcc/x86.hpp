#ifndef QCC_X86_HPP
#define QCC_X86_HPP

#include "asm.hpp"
#include "source.hpp"

namespace qcc
{

struct X86 : Asm
{
    X86(Ast &ast, Allocator &allocator, std::string_view source, std::ostream &stream);

    void emit() override;
    void emit_statement(Statement *statement);
    void emit_scope_statement(Scope_Statement *scope_statement);
    void emit_function_statement(Function_Statement *function_statement);
    void emit_define_statement(Define_Statement *define_statement);
    void emit_condition_statement(Condition_Statement *condition_statement);
    void emit_while_statement(While_Statement *while_statement);
    void emit_for_statement(For_Statement *for_statement);
    void emit_return_statement(Return_Statement *return_statement);

    void emit_expression(Expression *expression, Register regs);
    void emit_int_expression(Int_Expression *int_expression, Register regs);
    void emit_id_expression(Id_Expression *id_expression, Register regs);
    void emit_ref_expression(Ref_Expression *ref_expression, Register regs);
    void emit_binary_expression(Binary_Expression *binary_expression, Register regs);
    void emit_unary_expression(Unary_Expression *unary_expression, Register regs);
    void emit_increment_expression(Unary_Expression *unary_expression, Register regs);
    void emit_invoke_expression(Invoke_Expression *invoke_expression, Register regs);
    void emit_nested_expression(Nested_Expression *nested_expression, Register regs);
    void emit_assign_expression(Assign_Expression *assign_expression, Register regs,
                                bool in_invoke_expression);
    void emit_dot_expression(Dot_Expression *dot_expression, Register regs);
    void emit_deref_expression(Deref_Expression *deref_expression, Register regs);
    void emit_address_expression(Address_Expression *address_expression, Register regs);

    Source emit_expression_source(Expression *expression, Register regs);
    void emit_source_operand(const Source *source, int64 size);
    void emit_push(const Source *source);
    void emit_pop(const Source *source);
    void emit_mov(const Source *destination, const Source *source, int64 size);
};

} // namespace qcc

#endif
