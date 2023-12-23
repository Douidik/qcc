#include "x86.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/ostream.h>

namespace qcc
{

#define Make_Register(qword, dword, word, byte)       \
    {                                                 \
        "", byte, word, "", dword, "", "", "", qword, \
    }
#define Make_Specifier Make_Register

const X86_Register Rax = Make_Register("rax", "eax", "ax", "al");
const X86_Register Rbx = Make_Register("rbx", "ebx", "bx", "bl");
const X86_Register Rcx = Make_Register("rcx", "ecx", "cx", "cl");
const X86_Register Rdx = Make_Register("rdx", "edx", "dx", "dl");
const X86_Register Rbp = Make_Register("rbp", "ebp", "bp", "bpl");
const X86_Register Rsi = Make_Register("rsi", "esi", "si", "sil");
const X86_Register Rdi = Make_Register("rdi", "edi", "di", "dil");
const X86_Register Rsp = Make_Register("rsp", "esp", "sp", "spl");

const X86_Register(Gpr[8]) = {
    Make_Register("r15", "r15d", "r15w", "r15b"), Make_Register("r14", "r14d", "r14w", "r14b"),
    Make_Register("r13", "r13d", "r13w", "r13b"), Make_Register("r12", "r12d", "r12w", "r12b"),
    Make_Register("r11", "r11d", "r11w", "r11b"), Make_Register("r10", "r10d", "r10w", "r10b"),
    Make_Register("r9", "r9d", "r9w", "r9b"),     Make_Register("r8", "r8d", "r8w", "r8b"),
};

const X86_Specifier Spec = Make_Specifier("qword", "dword", "word", "byte");

X86::X86(Ast &ast, Allocator &allocator, std::string_view source, std::ostream &stream) :
    Asm(ast, allocator, source, stream)
{
}

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
        return make_expression(((Expression_Statement *)statement)->expression, Rax);
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

    if (function->is_main)
        fmt::println(stream, "_start:");
    fmt::println(stream, "{}:", function->name.str);
    fmt::println(stream, "    push rbp");
    fmt::println(stream, "    mov rbp, rsp");
    if (function->stack_size != 0)
        fmt::println(stream, "    sub rsp, {}", function->stack_size);

    // __cdecl function fetch arguments from stack
    // from function(argument_0, ..., argument_n - 1, argument_n)
    // now, the invoked function stack looks like
    // rbp
    // return_address
    // argument_0
    // argument_1
    // ...
    // argument_n
    Define_Statement *parameter_statement = function->parameters;
    size_t argument_offset = 16; // (rbp + return_address)
    for (; parameter_statement != NULL; parameter_statement = parameter_statement->next) {
        size_t size = parameter_statement->variable->type.size;
        fmt::println(stream, "    mov {}, {} [rbp + {}]", Rax[size], Spec[size], argument_offset);
        make_variable_set(parameter_statement->variable, Rax);
        argument_offset += 8;
    }

    make_scope_statement(function_statement->scope);
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

void X86::make_define_statement(Define_Statement *define_statement)
{
    if (!define_statement->expression)
        return;
    make_expression(define_statement->expression, Rax);
    make_variable_set(define_statement->variable, Rax);
    if (define_statement->next != NULL)
        make_define_statement(define_statement->next);
}

void X86::make_condition_statement(Condition_Statement *condition_statement)
{
    make_expression(condition_statement->boolean, Rax);
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
    make_expression(while_statement->boolean, Rax);
    fmt::println(stream, "    cmp rax, 0");
    fmt::println(stream, "    je {}", break_label);
    make_statement(while_statement->statement);
    fmt::println(stream, "{}:", break_label);
}

void X86::make_for_statement(For_Statement *for_statement) {}

void X86::make_return_statement(Return_Statement *return_statement)
{
    if (return_statement->expression != NULL)
        make_expression(return_statement->expression, Rdi);
    fmt::println(stream, "    jmp {}.return", return_statement->function->name.str);
}

void X86::make_expression(Expression *expression, const X86_Register &regs)
{
    switch (expression->kind()) {
    case Expression_Assign:
        return make_assign_expression((Assign_Expression *)expression, regs);
    case Expression_Binary:
        return make_binary_expression((Binary_Expression *)expression, regs);
    case Expression_Unary:
        return make_unary_expression((Unary_Expression *)expression, regs);
    case Expression_Int:
        return make_int_expression((Int_Expression *)expression, regs);
    case Expression_Id:
        return make_id_expression((Id_Expression *)expression, regs);
    case Expression_Invoke:
        return make_invoke_expression((Invoke_Expression *)expression, regs);
    default:
        break;
    }
}

void X86::make_int_expression(Int_Expression *int_expression, const X86_Register &regs)
{
    fmt::println("    mov {}, {}", regs[8], int_expression->value);
}

void X86::make_id_expression(Id_Expression *id_expression, const X86_Register &regs)
{
    Variable *variable = (Variable *)id_expression->object;
    make_variable_get(variable, regs);
}

void X86::make_binary_expression(Binary_Expression *binary_expression, const X86_Register &regs)
{
    make_expression(binary_expression->lhs, Rax);
    make_expression(binary_expression->rhs, Rdi);

    const char *f = (binary_expression->type->kind & Type_Fpr) ? "f" : "";

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

    if (binary_expression->operation.type & (Token_Mask_Binary_Assign)) {
        make_variable_set(ast.decode_designated_expression(binary_expression), Rax);
    }
}

void X86::make_unary_expression(Unary_Expression *unary_expression, const X86_Register &regs)
{
    bool is_increment = unary_expression->operation.type & (Token_Increment | Token_Decrement);
    make_expression(unary_expression->operand, Rax);

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
    if (is_increment and unary_expression->order == Expression_Rhs) {
        make_variable_set(ast.decode_designated_expression(unary_expression), Rax);
    }
}

void X86::make_invoke_expression(Invoke_Expression *invoke_expression, const X86_Register &regs)
{
    auto &uses_timeline = allocator.uses_timeline;
    uint32 use_time = invoke_expression->use_time;
    Function *function = invoke_expression->function;

    const auto on_register = [](Variable *variable) -> bool {
        return variable->location & (Type_Gpr | Type_Fpr);
    };

    if (use_time < uses_timeline.size()) {
        for (Variable *variable : uses_timeline[use_time] | views::filter(on_register))
            make_variable_push(variable);
    }

    Argument_Expression *argument_expression = invoke_expression->arguments;
    for (; argument_expression != NULL and argument_expression->next != NULL;
         argument_expression = argument_expression->next) {
    }
    for (; argument_expression != NULL; argument_expression = argument_expression->previous) {
        Assign_Expression *assign_expression = argument_expression->assign_expression;
	make_assign_expression(assign_expression, regs);
    }

    fmt::println(stream, "    call {}", function->name.str);
    if (regs != Rdi) {
        fmt::println(stream, "    mov {}, rdi", regs[8]);
    }
    if (function->invoke_size != 0) {
        fmt::println(stream, "    sub rsp, {}", function->invoke_size);
    }

    if (use_time < uses_timeline.size()) {
        for (Variable *variable : uses_timeline[use_time] | views::filter(on_register) | views::reverse)
            make_variable_pop(variable);
    }
}

// void X86::make_invoke_expression(Invoke_Expression *invoke_expression, const X86_Register &regs)
// {
//     bool needs_register_save = invoke_expression->use_time < allocator.uses_timeline.size();
//     std::set<Variable *> *used_variables = NULL;

//     if (needs_register_save) {
//         used_variables = &allocator.uses_timeline.at(invoke_expression->use_time);
//         for (Variable *variable : *used_variables) {
//             if (variable->source & (Type_Fpr | Type_Gpr))
//                 make_variable_push(variable);
//         }
//     }

//     // __cdecl function invoke: arguments are passed on the stack from right to left
//     // function(argument_0, argument_1, ..., argument_n)
//     // argument_0
//     // argument_1
//     // ...
//     // argument_n
//     // saved registers
//     Argument_Expression *argument_expression = invoke_expression->arguments;
//     size_t argument_stack_size = 0;

//     for (; argument_expression->next != NULL; argument_expression = argument_expression->next) {
//     }
//     for (; argument_expression != NULL; argument_expression = argument_expression->previous) {
//         size_t size = argument_expression->parameter->type.size;
//         make_expression(argument_expression->expression, Rax);
//         fmt::println(stream, "    push rax");
//         argument_stack_size += 8;
//     }

//     fmt::println(stream, "    call {}", invoke_expression->function->name.str);
//     if (regs != Rdi) {
//         fmt::println(stream, "    mov {}, rdi", regs[8]);
//     }
//     if (argument_stack_size != 0) {
//         fmt::println(stream, "    sub rsp, {}", argument_stack_size);
//     }

//     if (needs_register_save) {
//         for (Variable *variable : *used_variables | std::views::reverse) {
//             if (variable->source & (Type_Fpr | Type_Gpr))
//                 make_variable_pop(variable);
//         }
//     }
// }

void X86::make_assign_expression(Assign_Expression *assign_expression, const X86_Register &regs)
{
    make_expression(assign_expression->expression, regs);
    make_variable_set(assign_expression->variable, regs);

    if (assign_expression->next != NULL)
	make_assign_expression(assign_expression->next, regs);
}

void X86::make_deref_expression(Deref_Expression *deref_expression, const X86_Register &regs)
{
    size_t size = deref_expression->type->size;

    switch (deref_expression->object->kind()) {
    case Object_Variable: {
        Variable *variable = (Variable *)deref_expression->object;

        switch (variable->location) {
        case Source_Stack:
            fmt::println(stream, "mov {}, [rbp - {:+}]", regs[8], variable->address);
            fmt::println(stream, "mov {}, [{}]", regs[8], regs[8]);
            break;
        default:
            qcc_assert("TODO! make_deref_expression for this variable source type", 0);
        }
        break;
    }
    default:
        qcc_assert("TODO! make_deref_expression for this object kind", 0);
    }
}

void X86::make_address_expression(Address_Expression *address_expression, const X86_Register &regs)
{
    switch (address_expression->object->kind()) {
    case Object_Variable: {
        Variable *variable = (Variable *)address_expression->object;

        switch (variable->location) {
        case Source_Stack:
            fmt::println(stream, "lea {}, [rpb - {}]", regs[8], variable->address);
            break;
        default:
            qcc_assert("TODO! make_address_expression for this variable source type", 0);
        }
        break;
    }
    default:
        qcc_assert("TODO! make_address_expression for this object kind", 0);
    }
}

auto decode_variable_and_size(Object *object)
{
    qcc_assert("object is not a variable", object->kind() & Object_Variable);
    return std::make_tuple((Variable *)object, ((Variable *)object)->type.size);
}

void X86::make_source(Source *source, int64 size)
{
    switch (source->location) {
    case Source_Stack:
        fmt::print(stream, "{} [rbp {:+}]", Spec[size], source->address);
        break;
    case Source_Gpr:
        fmt::print(stream, "{}", Gpr[source->gpr][size]);
        break;
    default:
        qcc_assert("TODO! make_source for this source type", 0);
    }
}

void X86::make_variable_push(Object *object)
{
    auto [variable, size] = decode_variable_and_size(object);
    fmt::print("    push"), make_source(variable, size), fmt::print("\n");
}

void X86::make_variable_pop(Object *object)
{
    auto [variable, size] = decode_variable_and_size(object);
    fmt::print("    pop"), make_source(variable, size), fmt::print("\n");
}

void X86::make_variable_get(Object *object, const X86_Register &regs)
{
    auto [variable, size] = decode_variable_and_size(object);
    fmt::print("    mov {}", regs[size]);
    fmt::print(", "), make_source(variable, size), fmt::print("\n");
}

void X86::make_variable_set(Object *object, const X86_Register &regs)
{
    auto [variable, size] = decode_variable_and_size(object);
    fmt::print("    mov"), make_source(variable, size), fmt::print(", {}\n", regs[size]);
}

} // namespace qcc
