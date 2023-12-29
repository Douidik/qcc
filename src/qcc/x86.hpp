#ifndef QCC_X86_HPP
#define QCC_X86_HPP

#include "asm.hpp"
#include "source.hpp"
#include <array>

namespace qcc
{

typedef std::array<std::string_view, 9> X86_Register;
typedef std::array<std::string_view, 9> X86_Specifier;

struct X86 : Asm
{
    X86(Ast &ast, Allocator &allocator, std::string_view source, std::ostream &stream);

    void make() override;
    void make_statement(Statement *statement);
    void make_scope_statement(Scope_Statement *scope_statement);
    void make_function_statement(Function_Statement *function_statement);
    void make_define_statement(Define_Statement *define_statement);
    void make_condition_statement(Condition_Statement *condition_statement);
    void make_while_statement(While_Statement *while_statement);
    void make_for_statement(For_Statement *for_statement);
    void make_return_statement(Return_Statement *return_statement);

    void make_expression(Expression *expression, const X86_Register &regs);
    void make_int_expression(Int_Expression *int_expression, const X86_Register &regs);
    void make_id_expression(Id_Expression *id_expression, const X86_Register &regs);
    void make_ref_expression(Ref_Expression *ref_expression, const X86_Register &regs);
    void make_binary_expression(Binary_Expression *binary_expression, const X86_Register &regs);
    void make_unary_expression(Unary_Expression *unary_expression, const X86_Register &regs);
    void make_invoke_expression(Invoke_Expression *invoke_expression, const X86_Register &regs);
    void make_nested_expression(Nested_Expression *nested_expression, const X86_Register &regs);
    void make_assign_expression(Assign_Expression *assign_expression, const X86_Register &regs,
                                int64 offset = 0);
    void make_dot_expression(Dot_Expression *dot_expression, const X86_Register &regs);
    void make_deref_expression(Deref_Expression *deref_expression, const X86_Register &regs);
    void make_address_expression(Address_Expression *address_expression, const X86_Register &regs);

    void make_source(Source *source, int64 size, int64 offset = 0);
    void make_variable_push(Object *object, int64 offset = 0);
    void make_variable_pop(Object *object, int64 offset = 0);
    void make_variable_get(Object *object, const X86_Register &regs, int64 offset = 0);
    void make_variable_set(Object *object, const X86_Register &regs, int64 offset = 0);
};

} // namespace qcc

#endif
