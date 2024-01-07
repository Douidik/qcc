#include "x86.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/ostream.h>

namespace qcc
{

const Register Rax{8, "rax", "eax", "ax", "al"};
const Register Rbx{9, "rbx", "ebx", "bx", "bl"};
const Register Rcx{10, "rcx", "ecx", "cx", "cl"};
const Register Rdx{11, "rdx", "edx", "dx", "dl"};
const Register Rbp{12, "rbp", "ebp", "bp", "bpl"};
const Register Rsi{13, "rsi", "esi", "si", "sil"};
const Register Rdi{14, "rdi", "edi", "di", "dil"};
const Register Rsp{15, "rsp", "esp", "sp", "spl"};

const Register(Gpr[16]) = {
    {0, "r15", "r15d", "r15w", "r15b"},
    {1, "r14", "r14d", "r14w", "r14b"},
    {2, "r13", "r13d", "r13w", "r13b"},
    {3, "r12", "r12d", "r12w", "r12b"},
    {4, "r11", "r11d", "r11w", "r11b"},
    {5, "r10", "r10d", "r10w", "r10b"},
    {6, "r9", "r9d", "r9w", "r9b"},
    {7, "r8", "r8d", "r8w", "r8b"},
    Rax,
    Rbx,
    Rcx,
    Rdx,
    Rbp,
    Rsi,
    Rdi,
    Rsp,
};

const std::string_view Spec[9] = {"0?", "byte", "word", "3?", "dword", "5?", "6?", "7?", "qword"};

X86::X86(Ast &ast, Allocator &allocator, std::string_view source, std::ostream &stream) :
    Asm(ast, allocator, source, stream)
{
}

void X86::emit()
{
    fmt::println(stream, "BITS 64");
    fmt::println(stream, "section .text");
    fmt::println(stream, "    global _start");
    emit_statement(ast.main_statement);
}

void X86::emit_statement(Statement *statement)
{
    switch (statement->kind()) {
    case Statement_Scope:
        return emit_scope_statement((Scope_Statement *)statement);
    case Statement_Function:
        return emit_function_statement((Function_Statement *)statement);
    case Statement_Define:
        return emit_define_statement((Define_Statement *)statement);
    case Statement_Expression:
        return emit_expression(((Expression_Statement *)statement)->expression, Rax);
    case Statement_Condition:
        return emit_condition_statement((Condition_Statement *)statement);
    case Statement_While:
        return emit_while_statement((While_Statement *)statement);
    case Statement_For:
        return emit_for_statement((For_Statement *)statement);
    case Statement_Return:
        return emit_return_statement((Return_Statement *)statement);
    default:
        break;
    }
}

void X86::emit_scope_statement(Scope_Statement *scope_statement)
{
    for (Statement *statement : scope_statement->body) {
        emit_statement(statement);
    }
}

void X86::emit_function_statement(Function_Statement *function_statement)
{
    if (!function_statement->scope)
        return;

    Function *function = function_statement->function;

    if (function->is_main)
        fmt::println(stream, "_start:");
    fmt::println(stream, "{}:", function->name.str);
    fmt::println(stream, "    push rbp");
    fmt::println(stream, "    mov rbp, rsp");
    if (function->stack_size != 0)
        fmt::println(stream, "    sub rsp, {}", function->stack_size);

    emit_scope_statement(function_statement->scope);
    fmt::println(stream, "{}.return:", function->name.str);

    if (function->is_main) {
        fmt::println(stream, "    mov rax, 1");
        fmt::println(stream, "    mov rbx, rdi");
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

void X86::emit_define_statement(Define_Statement *define_statement)
{
    if (!define_statement->expression)
        return;
    emit_expression(define_statement->expression, Rax);

    Variable *variable = define_statement->variable;
    int64 size = variable->type.size;
    emit_set(variable, Rax, size);

    if (define_statement->next != NULL)
        emit_define_statement(define_statement->next);
}

void X86::emit_condition_statement(Condition_Statement *condition_statement)
{
    emit_expression(condition_statement->boolean, Rax);
    fmt::println(stream, "    cmp rax, 0");

    Label end_label = emit_label(Label_If_End);
    if (condition_statement->statement_else != NULL) {
        Label else_label = emit_label(Label_Else);

        fmt::println(stream, "    je {}", else_label);
        emit_scope_statement(condition_statement->statement_if);
        fmt::println(stream, "    jmp {}", end_label);
        fmt::println(stream, "{}:", else_label);
        emit_scope_statement(condition_statement->statement_else);
        fmt::println(stream, "{}:", end_label);
    } else {
        fmt::println(stream, "    je {}", end_label);
        emit_scope_statement(condition_statement->statement_if);
        fmt::println(stream, "{}:", end_label);
    }
}

void X86::emit_while_statement(While_Statement *while_statement)
{
    Label continue_label = emit_label(Label_Continue);
    Label break_label = emit_label(Label_Continue);

    fmt::println(stream, "{}:", continue_label);
    emit_expression(while_statement->boolean, Rax);
    fmt::println(stream, "    cmp rax, 0");
    fmt::println(stream, "    je {}", break_label);
    emit_statement(while_statement->statement);
    fmt::println(stream, "{}:", break_label);
}

void X86::emit_for_statement(For_Statement *for_statement) {}

void X86::emit_return_statement(Return_Statement *return_statement)
{
    if (return_statement->expression != NULL)
        emit_expression(return_statement->expression, Rdi);
    fmt::println(stream, "    jmp {}.return", return_statement->function->name.str);
}

void X86::emit_expression(Expression *expression, Register regs)
{
    switch (expression->kind()) {
    case Expression_Binary:
        return emit_binary_expression((Binary_Expression *)expression, regs);
    case Expression_Unary:
        return emit_unary_expression((Unary_Expression *)expression, regs);
    case Expression_Int:
        return emit_int_expression((Int_Expression *)expression, regs);
    case Expression_Id:
        return emit_id_expression((Id_Expression *)expression, regs);
    case Expression_Ref:
        return emit_ref_expression((Ref_Expression *)expression, regs);
    case Expression_Invoke:
        return emit_invoke_expression((Invoke_Expression *)expression, regs);
    case Expression_Nested:
        return emit_nested_expression((Nested_Expression *)expression, regs);
    case Expression_Assign:
        return emit_assign_expression((Assign_Expression *)expression, regs, false);
    case Expression_Dot:
        return emit_dot_expression((Dot_Expression *)expression, regs);
    case Expression_Deref:
        return emit_deref_expression((Deref_Expression *)expression, regs);
    case Expression_Address:
        return emit_address_expression((Address_Expression *)expression, regs);
    default:
        break;
    }
}

void X86::emit_int_expression(Int_Expression *int_expression, Register regs)
{
    fmt::println(stream, "    mov {}, {}", regs[8], int_expression->value);
}

void X86::emit_id_expression(Id_Expression *id_expression, Register regs)
{
    Variable *variable = (Variable *)id_expression->object;
    emit_get(variable, regs, variable->type.size);
}

void X86::emit_ref_expression(Ref_Expression *ref_expression, Register regs)
{
    Variable *variable = (Variable *)ref_expression->object;
    emit_get(variable, regs, variable->type.size);
}

// Hack! for now, we just push rax before fetching the rhs operand, we emit the binary
// operations with only two registers. Maybe find a way to use more register:
// - Some registers are not suitable for some instructions (eg: div)
// - Allocate stack memory when no registers are left
// - We need to know what registers are in use at any time for allocation and save during invoke
void X86::emit_binary_expression(Binary_Expression *binary_expression, Register regs)
{
    emit_expression(binary_expression->lhs, Rax), emit_push(&Rax);

    if (binary_expression->operation.type & (Token_Mask_Shift))
        emit_expression(binary_expression->rhs, Rcx);
    else
        emit_expression(binary_expression->rhs, Rdi);

    const char *f = (binary_expression->type->kind & Type_Fpr) ? "f" : "";
    const char *i = (binary_expression->type->mods & Type_Signed) ? "i" : "";

    emit_pop(&Rax);
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
        fmt::println(stream, "    {}{}mul rax, rdi", i, f);
        break;
    case Token_Div:
    case Token_Div_Assign:
        fmt::println(stream, "    {}{}div rax, rdi", i, f);
        break;
    case Token_Mod:
    case Token_Mod_Assign:
        fmt::println(stream, "    mod rax, rdi");
        break;

    case Token_Eq:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    sete al");
        fmt::println(stream, "    movzx rax, al");
        break;
    case Token_Not_Eq:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    setne al");
        fmt::println(stream, "    movzx rax, al");
        break;
    case Token_Less:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    setl al");
        fmt::println(stream, "    movzx rax, al");
        break;
    case Token_Less_Eq:
        fmt::println(stream, "    cmp rax, rdi");
        fmt::println(stream, "    setle al");
        fmt::println(stream, "    movzx rax, al");
        break;
    case Token_Greater:
        fmt::println(stream, "    cmp rdi, rax");
        fmt::println(stream, "    setl al");
        fmt::println(stream, "    movzx rax, al");
        break;
    case Token_Greater_Eq:
        fmt::println(stream, "    cmp rdi, rax");
        fmt::println(stream, "    setle al");
        fmt::println(stream, "    movzx rax, al");
        break;

    case Token_Bitwise_And:
    case Token_Bin_And_Assign:
        fmt::println(stream, "    and rax, rdi");
        break;
    case Token_Bitwise_Or:
    case Token_Bin_Or_Assign:
        fmt::println(stream, "    or rax, rdi");
        break;
    case Token_Bitwise_Xor:
    case Token_Bin_Xor_Assign:
        fmt::println(stream, "    xor rax, rdi");
        break;

    // sal/sar instructions uses the count register
    case Token_Shift_L:
    case Token_Shift_L_Assign:
        fmt::println(stream, "    sal rax, cl");
        break;
    case Token_Shift_R:
    case Token_Shift_R_Assign:
        fmt::println(stream, "    sar rax, cl");
        break;

    default:
        break;
    }

    if (binary_expression->operation.type & (Token_Mask_Binary_Assign)) {
        Object *object = ast.decode_designated_expression(binary_expression);
        int64 size = object->object_type()->size;
        emit_set(object->source(), Rax, size);
    }
    if (regs.gpr != Rax.gpr) {
        fmt::println(stream, "    mov {}, rax", regs[8]);
    }
}

void X86::emit_unary_expression(Unary_Expression *unary_expression, Register regs)
{
    bool is_increment = unary_expression->operation.type & (Token_Increment | Token_Decrement);
    emit_expression(unary_expression->operand, Rax);

    if (is_increment and unary_expression->order == Expression_Lhs)
        fmt::println(stream, "    push [rsp]");

    switch (unary_expression->operation.type) {
    case Token_Sub:
        fmt::println(stream, "    neg rax");
        break;
    case Token_Not:
        fmt::println(stream, "    cmp rax, 0");
        fmt::println(stream, "    sete al");
        fmt::println(stream, "    movzx rax, al");
        break;
    case Token_Bitwise_Not:
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
    if (is_increment and unary_expression->order == Expression_Rhs) {
        Object *object = ast.decode_designated_expression(unary_expression);
        int64 size = object->object_type()->size;
        emit_set(object->source(), Rax, size);
    }
}

void X86::emit_invoke_expression(Invoke_Expression *invoke_expression, Register regs)
{
    auto &uses_timeline = allocator.uses_timeline;
    uint32 use_time = invoke_expression->use_time;
    Function *function = invoke_expression->function;

    const auto on_register = [](Variable *variable) -> bool {
        return variable->location & (Source_Gpr | Source_Fpr);
    };

    if (use_time < uses_timeline.size()) {
        for (Variable *variable : uses_timeline[use_time] | views::filter(on_register))
            emit_push(variable);
    }

    Argument_Expression *argument_expression = invoke_expression->arguments;
    for (; argument_expression != NULL and argument_expression->next != NULL;
         argument_expression = argument_expression->next) {
    }
    for (; argument_expression != NULL; argument_expression = argument_expression->previous) {
        Assign_Expression *assign_expression = argument_expression->assign_expression;
        emit_assign_expression(assign_expression, regs, true);
    }
    fmt::println(stream, "    call {}", function->name.str);

    if (function->invoke_size != 0) {
        fmt::println(stream, "    add rsp, {}", function->invoke_size);
    }
    if (regs.gpr != Rdi.gpr) {
        fmt::println(stream, "    mov {}, rdi", regs[8]);
    }

    if (use_time < uses_timeline.size()) {
        for (Variable *variable : uses_timeline[use_time] | views::filter(on_register) | views::reverse)
            emit_pop(variable);
    }
}

void X86::emit_nested_expression(Nested_Expression *nested_expression, Register regs)
{
    emit_expression(nested_expression->operand, regs);
}

void X86::emit_assign_expression(Assign_Expression *assign_expression, Register regs,
                                 bool in_invoke_expression)
{
    int64 size = assign_expression->type->size;
    Source destination = emit_expression_source(assign_expression->lhs, regs);

    if (assign_expression->type->kind & Type_Aggregate) {
        Source source = emit_expression_source(assign_expression->rhs, regs);
        int64 offset = Round_Up(size, 8);

        while (offset != 0) {
            int64 chunk_size = 8;
            for (; chunk_size > offset; chunk_size >>= 1) {
            }
            offset -= chunk_size;

            Source source_chunk = source.with_offset(offset);
            if (in_invoke_expression) {
                emit_push(&source_chunk);
            } else {
                Source destination_chunk = destination.with_offset(offset);
                emit_mov(&destination_chunk, &source_chunk, chunk_size);
            }
        }
    } else {
        emit_expression(assign_expression->rhs, regs);
        if (in_invoke_expression)
            emit_push(&regs);
        else
            emit_set(&destination, regs, size);
    }
}

void X86::emit_dot_expression(Dot_Expression *dot_expression, Register regs)
{
    Source source = emit_expression_source(dot_expression, regs);
    emit_get(&source, regs, dot_expression->member->type.size);
}

Source X86::emit_expression_source(Expression *expression, Register regs)
{
    Source source;

    switch (expression->kind()) {
    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)expression;
        return *id_expression->object->source();
    }

    case Expression_Ref: {
        Ref_Expression *ref_expression = (Ref_Expression *)expression;
        return *ref_expression->object->source();
    }

    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        Source source = emit_expression_source(dot_expression->expression, regs);
        int64 offset = dot_expression->member->struct_offset;
        return source.with_offset(offset);
    }

    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        return emit_expression_source(nested_expression->operand, regs);
    }

    case Expression_Address: {
        emit_address_expression((Address_Expression *)expression, regs);
        return regs;
    }

    case Expression_Deref: {
        Deref_Expression *deref_expression = (Deref_Expression *)expression;
        emit_get(deref_expression->object->source(), regs, 8);
        return regs.with_indirection();
    }

    default:
        qcc_todo("emit expression kind");
        return Source{};
    }
}

void X86::emit_deref_expression(Deref_Expression *deref_expression, Register regs)
{
    Source *address_source = deref_expression->object->source();
    qcc_assert(address_source != NULL, "object is not sourced");
    size_t size = deref_expression->type->size;

    emit_mov(&regs, address_source, 8);
    emit_set(&regs, regs.with_indirection(), size);
}

void X86::emit_address_expression(Address_Expression *address_expression, Register regs)
{
    Source *source = address_expression->object->source();
    qcc_assert(source != NULL, "object is not sourced");

    if (source->location & Source_Stack)
        fmt::println(stream, "    lea {}, [rbp {:+}]", regs[8], source->address + source->offset);
}

void X86::emit_source_operand(const Source *source, int64 size)
{
    qcc_assert(size <= 8, "source does not fit in an x86_64 register");
    qcc_assert(!source->offset or
                   (source->has_indirection or source->location & (Source_Stack | Source_Data)),
               "cannot offset a register without indirection");
    qcc_assert(!source->has_indirection or source->location & Source_Gpr,
               "cannot indirect a non register source");

    switch (source->location) {
    case Source_Stack:
        fmt::print(stream, "{} [rbp {:+}]", Spec[size], source->address + source->offset);
        break;
    case Source_Gpr:
        if (source->has_indirection) {
            fmt::print(stream, "[{} {:+}]", Gpr[source->gpr][8], source->offset);
        } else {
            fmt::print(stream, "{}", Gpr[source->gpr][size]);
        }
        break;
    default:
        qcc_todo("emit_source for this source type");
    }
}

void X86::emit_push(const Source *source)
{
    fmt::print(stream, "    push ");
    emit_source_operand(source, 8);
    fmt::println(stream, "");
}

void X86::emit_pop(const Source *source)
{
    fmt::print(stream, "    pop ");
    emit_source_operand(source, 8);
    fmt::println(stream, "");
}

void X86::emit_get(const Source *source, Register regs, int64 size)
{
    fmt::print(stream, "    mov ");
    emit_source_operand(&regs, size);
    fmt::print(stream, ", ");
    emit_source_operand(source, size);
    fmt::println(stream, "");
}

void X86::emit_set(const Source *source, Register regs, int64 size)
{
    fmt::print(stream, "    mov ");
    emit_source_operand(source, size);
    fmt::print(stream, ", ");
    emit_source_operand(&regs, size);
    fmt::println(stream, "");
}

void X86::emit_mov(const Source *destination, const Source *source, int64 size)
{
    fmt::print(stream, "    mov ");
    emit_source_operand(destination, size);
    fmt::print(stream, ", ");
    emit_source_operand(source, size);
    fmt::println(stream, "");
}

} // namespace qcc
