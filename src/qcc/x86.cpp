#include "x86.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/ostream.h>

namespace qcc
{

X86::X86(Ast &ast, std::string_view source, std::ostream &stream) : Asm(ast, source, stream) {}

void X86::make()
{
    fmt::println("BITS 64");
    fmt::println("section .text");
    fmt::println("    global _start");
    make_statement(ast.main_statement);
}

void X86::make_statement(Statement *statement)
{
    switch (statement->kind()) {
    case Statement_Scope:
        return make_scope_statement((Scope_Statement *)statement);
    case Statement_Function:
        return make_function_statement((Function_Statement *)statement);
    case Statement_Define:
        return make_define_statement((Define_Statement *)statement);
    case Statement_Expression:
        return make_expression(((Expression_Statement *)statement)->expression);
    case Statement_Condition:
        return make_condition_statement((Condition_Statement *)statement);
    case Statement_While:
        return make_while_statement((While_Statement *)statement);
    case Statement_For:
        return make_for_statement((For_Statement *)statement);
    case Statement_Return:
        return make_return_statement((Return_Statement *)statement);
    default:
        break;
    }
}

void X86::make_scope_statement(Scope_Statement *scope_statement)
{
    for (Statement *statement : scope_statement->body) {
        make_statement(statement);
    }
}

void X86::make_function_statement(Function_Statement *function_statement)
{
    if (!function_statement->scope)
        return;

    Function *function = function_statement->function;

    fmt::println(stream, "{}:", function->is_main ? "_start" : function->name.str);
    fmt::println(stream, "    push rbp");
    fmt::println(stream, "    mov rbp, rsp");
    fmt::println(stream, "    sub rsp, {}", function->stack_size);
    
    if (function_statement->scope != NULL) {
        make_scope_statement(function_statement->scope);
    }

    fmt::println(stream, "{}.return:", function->name.str);

    if (function->is_main) {
        fmt::println(stream, "    mov rax, 1");
        fmt::println(stream, "    pop rbx");
        fmt::println(stream, "    mov rsp, rbp");
        fmt::println(stream, "    pop rbp");
        fmt::println(stream, "    int 128");
    } else {
        fmt::println(stream, "    mov rsp, rbp");
        fmt::println(stream, "    pop rbp");
        fmt::println(stream, "    ret");
    }
    fmt::println(stream, "");
}

void X86::make_define_statement(Define_Statement *define_statement)
{
    if (!define_statement->expression)
        return;
    make_expression(define_statement->expression);
    make_variable_store(define_statement->variable);
    if (define_statement->next != NULL)
        make_define_statement(define_statement->next);
}

void X86::make_condition_statement(Condition_Statement *condition_statement)
{
    make_expression(condition_statement->boolean);
    fmt::println(stream, "    pop rax");
    fmt::println(stream, "    cmp rax, 0");

    Label end_label = make_label(Label_If_End);
    if (condition_statement->statement_else != NULL) {
        Label else_label = make_label(Label_Else);

        fmt::println(stream, "    je {}", else_label);
        make_scope_statement(condition_statement->statement_if);
        fmt::println(stream, "    jmp {}", end_label);
        fmt::println(stream, "{}:", else_label);
        make_scope_statement(condition_statement->statement_else);
        fmt::println(stream, "{}:", end_label);
    } else {
        fmt::println(stream, "    je {}", end_label);
        make_scope_statement(condition_statement->statement_if);
        fmt::println(stream, "{}:", end_label);
    }
}

void X86::make_while_statement(While_Statement *while_statement)
{
    Label continue_label = make_label(Label_Continue);
    Label break_label = make_label(Label_Continue);

    fmt::println(stream, "{}:", continue_label);
    make_expression(while_statement->boolean);
    fmt::println(stream, "    cmp rax, 0");
    fmt::println(stream, "    je {}", break_label);
    make_statement(while_statement->statement);
    fmt::println(stream, "{}:", break_label);
}

void X86::make_for_statement(For_Statement *for_statement) {}

void X86::make_return_statement(Return_Statement *return_statement)
{
    if (return_statement->expression != NULL)
	make_expression(return_statement->expression);
    fmt::println(stream, "    jmp {}.return", return_statement->function->name.str);
}

void X86::make_expression(Expression *expression)
{
    switch (expression->kind()) {
    case Expression_Assign:
        return make_assign_expression((Assign_Expression *)expression);
    case Expression_Binary:
        return make_binary_expression((Binary_Expression *)expression);
    case Expression_Unary:
        return make_unary_expression((Unary_Expression *)expression);
    case Expression_Int:
        return make_int_expression((Int_Expression *)expression);
    case Expression_Id:
        return make_id_expression((Id_Expression *)expression);
    case Expression_Invoke:
        return make_invoke_expression((Invoke_Expression *)expression);
    default:
        break;
    }
}

void X86::make_int_expression(Int_Expression *int_expression)
{
    fmt::println("    push {}", int_expression->value);
}

void X86::make_id_expression(Id_Expression *id_expression)
{
    make_variable_load((Variable *)id_expression->object);
}

void X86::make_binary_expression(Binary_Expression *binary_expression)
{
    make_expression(binary_expression->lhs);
    make_expression(binary_expression->rhs);

    fmt::println(stream, "    pop rdi");
    fmt::println(stream, "    pop rax");

    const char *f = (binary_expression->type->kind & (Type_Float | Type_Double)) ? "f" : "";

    switch (binary_expression->operation.type) {
    case Token_Add:
    case Token_Add_Assign:
        fmt::println(stream, "    {}add rax, rdi", f);
        break;
    case Token_Sub:
    case Token_Sub_Assign:
        fmt::println(stream, "    {}sub rax, rdi", f);
        break;
    case Token_Mul:
    case Token_Mul_Assign:
        fmt::println(stream, "    {}mul rax, rdi", f);
        break;
    case Token_Div:
    case Token_Div_Assign:
        fmt::println(stream, "    {}div rax, rdi", f);
        break;
    case Token_Mod:
    case Token_Mod_Assign:
        fmt::println(stream, "    mod rax, rdi");
        break;

    case Token_Eq:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    sete al");
        fmt::println(stream, "    movzb rax, al");
        break;
    case Token_Not_Eq:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    setne al");
        fmt::println(stream, "    movzb rax, al");
        break;
    case Token_Less:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    setl al");
        fmt::println(stream, "    movzb rax, al");
        break;
    case Token_Less_Eq:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    setle al");
        fmt::println(stream, "    movzb rax, al");
        break;
    case Token_Greater:
        fmt::println(stream, "    cmp rdi, rax");
        fmt::println(stream, "    setl al");
        fmt::println(stream, "    movzb rax, al");
        break;
    case Token_Greater_Eq:
        fmt::println(stream, "    cmp rdi, rax");
        fmt::println(stream, "    setle al");
        fmt::println(stream, "    movzb rax, al");
        break;

    case Token_Bin_And:
    case Token_Bin_And_Assign:
        fmt::println(stream, "    mod rax, rdi");
        break;
    case Token_Bin_Or:
    case Token_Bin_Or_Assign:
        fmt::println(stream, "    or rax, rdi");
        break;
    case Token_Bin_Xor:
    case Token_Bin_Xor_Assign:
        fmt::println(stream, "    xor rax, rdi");
        break;

    // sal/sar instructions uses the count register
    case Token_Shift_L:
    case Token_Shift_L_Assign:
        fmt::println(stream, "    mov cl, dil");
        fmt::println(stream, "    sal rax, cl");
        break;
    case Token_Shift_R:
    case Token_Shift_R_Assign:
        fmt::println(stream, "    mov cl, dil");
        fmt::println(stream, "    sar rax, cl");
        break;

    default:
        break;
    }

    fmt::println(stream, "    push rax");
    if (binary_expression->operation.type & (Token_Mask_Binary_Assign))
        make_variable_store(ast.decode_designated_expression(binary_expression));
}

void X86::make_unary_expression(Unary_Expression *unary_expression)
{
    bool is_increment = unary_expression->operation.type & (Token_Increment | Token_Decrement);
    make_expression(unary_expression->operand);

    if (is_increment and unary_expression->order == Expression_Lhs)
        fmt::println(stream, "    push [rsp]");
    fmt::println(stream, "    pop rax");

    switch (unary_expression->operation.type) {
    case Token_Sub:
        fmt::println(stream, "    neg rax");
        break;
    case Token_Not:
        fmt::println(stream, "    cmp rax, 0");
        fmt::println(stream, "    sete al");
        fmt::println(stream, "    movzx rax, al");
        break;
    case Token_Bin_Not:
        fmt::println(stream, "    not rax");
        break;
    case Token_Increment:
        fmt::println(stream, "    inc rax");
        break;
    case Token_Decrement:
        fmt::println(stream, "    dec rax");
        break;
    default:
        break;
    }

    fmt::println(stream, "    push rax");
    if (is_increment and unary_expression->order == Expression_Rhs) {
        make_variable_store(ast.decode_designated_expression(unary_expression));
    }
}

void X86::make_invoke_expression(Invoke_Expression *invoke_expression)
{
    Argument_Expression *argument_expression = invoke_expression->arguments;

    for (; argument_expression != NULL; argument_expression = argument_expression->next) {
	// parameter offset of 16 because:
	// 8 (Return address)
	// 8 (Stack base)
        make_expression(argument_expression->expression);
	make_variable_store(argument_expression->parameter, 16);
    }
    fmt::println(stream, "    call {}", invoke_expression->function->name.str);
}

void X86::make_assign_expression(Assign_Expression *assign_expression)
{
    make_expression(assign_expression->operand);
    make_variable_store(assign_expression->variable);
}

void X86::make_variable_load(Variable *variable, size_t offset)
{
    fmt::println(stream, "    mov rax, [rbp - {}]", variable->address + offset);
    switch (variable->type.size) {
    case 1:
        fmt::println(stream, "    mov al, byte [rbp - {}]", variable->address + offset);
        break;
    case 2:
        fmt::println(stream, "    mov ax, word [rbp - {}]", variable->address + offset);
        break;
    case 4:
        fmt::println(stream, "    mov eax, dword [rbp - {}]", variable->address + offset);
        break;
    case 8:
        fmt::println(stream, "    mov rax, [rbp - {}]", variable->address + offset);
        break;
    default:
        qcc_assert("x86 does not support register of this size", 0);
    }

    fmt::println(stream, "    push rax");
}

void X86::make_variable_store(Variable *variable, size_t offset)
{
    fmt::println("    pop rdi");

    switch (variable->type.size) {
    case 1:
        fmt::println(stream, "    mov byte [rbp - {}], dil", variable->address + offset);
        break;
    case 2:
        fmt::println(stream, "    mov word [rbp - {}], di", variable->address + offset);
        break;
    case 4:
        fmt::println(stream, "    mov dword [rbp - {}], edi", variable->address + offset);
        break;
    case 8:
        fmt::println(stream, "    mov qword [rbp - {}], rdi", variable->address + offset);
        break;
    default:
        qcc_assert("x86 does not support register of this size", 0);
    }
}

} // namespace qcc
