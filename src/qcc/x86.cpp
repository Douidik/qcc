#include "x86.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/ostream.h>
#include <unordered_map>

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

// Cast_Type_Key: 4 byte number that describe a type for cast:
// (with m: mods-byte, k: kind-byte) :: 0_m_k_k
// Cast_Key: 8 byte number, takes two "or-ed" cast_type_key that describe a cast

#define Cast_Type_Key(kind, mods) ((uint64)(((uint8)mods) << 16) | kind)
#define Cast_Key(from, into) ((from << 32) | into)

enum Cast_Type_Keys : uint64
{
    Key_Byte = Cast_Type_Key(Type_Char, 0),
    Key_Int16 = Cast_Type_Key(Type_Int, Type_Signed | Type_Short),
    Key_Int32 = Cast_Type_Key(Type_Int, Type_Signed),
    Key_Int64 = Cast_Type_Key(Type_Int, Type_Signed | Type_Long),

    Key_UByte = Cast_Type_Key(Type_Char, Type_Unsigned),
    Key_UInt32 = Cast_Type_Key(Type_Int, Type_Unsigned),
    Key_UInt16 = Cast_Type_Key(Type_Int, Type_Signed | Type_Short),
    Key_UInt64 = Cast_Type_Key(Type_Int, Type_Unsigned | Type_Long),

    Key_Float32 = Cast_Type_Key(Type_Float, 0),
    Key_Float64 = Cast_Type_Key(Type_Double, 0),
};

// Todo! implement real types (these are less straight-forward)
const std::unordered_map<uint64, std::string_view> Cast_Matrix = {
    {Cast_Key(Key_Byte, Key_Int32), "movsbl {}, {}"},  {Cast_Key(Key_UByte, Key_Int32), "movzbl {}, {}"},
    {Cast_Key(Key_Int16, Key_Int32), "movswl {}, {}"}, {Cast_Key(Key_UInt16, Key_Int32), "movzwl {}, {}"},
    {Cast_Key(Key_Int64, Key_Int32), "movsxd {}, {}"},
};

X86::X86(Ast &ast, Allocator &allocator, std::ostream &stream) :
    Asm(ast, allocator, stream)
{
}

void X86::emit()
{
    emitln("BITS 64");
    emitln("section .text");
    emitln("    global _start");
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
        emitln("_start:");
    emitln("{}:", function->name.str);
    emitln("    push rbp");
    emitln("    mov rbp, rsp");
    if (function->stack_size != 0)
        emitln("    sub rsp, {}", function->stack_size);

    emit_scope_statement(function_statement->scope);
    emitln("{}.return:", function->name.str);

    if (function->is_main) {
        emitln("    mov rax, 1");
        emitln("    mov rbx, rdi");
        emitln("    mov rsp, rbp");
        emitln("    pop rbp");
        emitln("    int 128");
    } else {
        emitln("    mov rsp, rbp");
        emitln("    pop rbp");
        emitln("    ret");
    }
    emitln("");
}

void X86::emit_define_statement(Define_Statement *define_statement)
{
    if (!define_statement->expression)
        return;
    emit_expression(define_statement->expression, Rax);

    Variable *variable = define_statement->variable;
    int64 size = variable->type()->size;
    emit_mov(variable, &Rax, size);

    if (define_statement->next != NULL)
        emit_define_statement(define_statement->next);
}

void X86::emit_condition_statement(Condition_Statement *condition_statement)
{
    emit_expression(condition_statement->boolean, Rax);
    emitln("    cmp rax, 0");

    Label end_label = emit_label(Label_If_End);
    if (condition_statement->statement_else != NULL) {
        Label else_label = emit_label(Label_Else);

        emitln("    je {}", else_label);
        emit_scope_statement(condition_statement->statement_if);
        emitln("    jmp {}", end_label);
        emitln("{}:", else_label);
        emit_scope_statement(condition_statement->statement_else);
        emitln("{}:", end_label);
    } else {
        emitln("    je {}", end_label);
        emit_scope_statement(condition_statement->statement_if);
        emitln("{}:", end_label);
    }
}

void X86::emit_while_statement(While_Statement *while_statement)
{
    Label continue_label = emit_label(Label_Continue);
    Label break_label = emit_label(Label_Continue);

    emitln("{}:", continue_label);
    emit_expression(while_statement->boolean, Rax);
    emitln("    cmp rax, 0");
    emitln("    je {}", break_label);
    emit_statement(while_statement->statement);
    emitln("{}:", break_label);
}

void X86::emit_for_statement(For_Statement *for_statement) {}

void X86::emit_return_statement(Return_Statement *return_statement)
{
    if (return_statement->expression != NULL)
        emit_expression(return_statement->expression, Rdi);
    emitln("    jmp {}.return", return_statement->function->name.str);
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
    case Expression_Cast:
        return emit_cast_expression((Cast_Expression *)expression, regs);
    default:
        break;
    }
}

void X86::emit_int_expression(Int_Expression *int_expression, Register regs)
{
    emitln("    mov {}, {}", regs[8], int_expression->value);
}

void X86::emit_id_expression(Id_Expression *id_expression, Register regs)
{
    Variable *variable = (Variable *)id_expression->object;

    // decay the array (do not cast it, may fall in infinite recursion)
    if (variable->type()->kind & Type_Array) {
	emit_lea(&regs, variable);
    } else {
        emit_mov(&regs, variable, variable->type()->size);
    }
}

void X86::emit_ref_expression(Ref_Expression *ref_expression, Register regs)
{
    Object *object = (Variable *)ref_expression->object;
    emit_mov(&regs, object->source(), object->type()->size);
}

// Hack! for now, we just push rax before fetching the rhs operand, we emit the binary
// operations with only two registers. Maybe find a way to use more register:
// - Some registers are not suitable for some instructions (eg: div)
// - Allocate stack memory when no registers left
// - We need to know what registers are in use at any time for allocation and save during invoke
void X86::emit_binary_expression(Binary_Expression *binary_expression, Register regs)
{
    emit_expression(binary_expression->lhs, Rax);
    emit_push(&Rax);

    if (binary_expression->operation.type & (Token_Shift_L | Token_Shift_R))
        emit_expression(binary_expression->rhs, Rcx);
    else
        emit_expression(binary_expression->rhs, Rdi);

    const char *f = (binary_expression->type.kind & Type_Fpr) ? "f" : "";
    const char *i = (binary_expression->type.mods & Type_Signed) ? "i" : "";

    if (binary_expression->type.kind & (Type_Pointer | Type_Array)) {
        int64 size = binary_expression->type.pointed_type->size;
        emitln("    imul rdi, {}", size);
    }

    emit_pop(&Rax);
    switch (binary_expression->operation.type) {
    case Token_Add:
        emitln("    {}add rax, rdi", f);
        break;
    case Token_Sub:
        emitln("    {}sub rax, rdi", f);
        break;
    case Token_Mul:
        emitln("    {}{}mul rax, rdi", i, f);
        break;
    case Token_Div:
        emitln("    {}{}div rax, rdi", i, f);
        break;
    case Token_Mod:
        emitln("    mod rax, rdi");
        break;

    case Token_Eq:
        emitln("    cmp rax, rdi");
        emitln("    sete al");
        emitln("    movzx rax, al");
        break;
    case Token_Not_Eq:
        emitln("    cmp rax, rdi");
        emitln("    setne al");
        emitln("    movzx rax, al");
        break;
    case Token_Less:
        emitln("    cmp rax, rdi");
        emitln("    setl al");
        emitln("    movzx rax, al");
        break;
    case Token_Less_Eq:
        emitln("    cmp rax, rdi");
        emitln("    setle al");
        emitln("    movzx rax, al");
        break;
    case Token_Greater:
        emitln("    cmp rdi, rax");
        emitln("    setl al");
        emitln("    movzx rax, al");
        break;
    case Token_Greater_Eq:
        emitln("    cmp rdi, rax");
        emitln("    setle al");
        emitln("    movzx rax, al");
        break;

    case Token_Bitwise_And:
        emitln("    and rax, rdi");
        break;
    case Token_Bitwise_Or:
        emitln("    or rax, rdi");
        break;
    case Token_Bitwise_Xor:
        emitln("    xor rax, rdi");
        break;

    // sal/sar instructions uses the count register
    case Token_Shift_L:
        emitln("    sal rax, cl");
        break;
    case Token_Shift_R:
        emitln("    sar rax, cl");
        break;

    default:
        break;
    }

    if (regs.gpr != Rax.gpr) {
        emitln("    mov {}, rax", regs[8]);
    }
}

void X86::emit_unary_expression(Unary_Expression *unary_expression, Register regs)
{
    if (unary_expression->operation.type & (Token_Increment | Token_Decrement)) {
        return emit_increment_expression(unary_expression, regs);
    }

    emit_expression(unary_expression->operand, Rax);
    switch (unary_expression->operation.type) {
    case Token_Sub:
        emitln("    neg rax");
        break;
    case Token_Not:
        emitln("    cmp rax, 0");
        emitln("    sete al");
        emitln("    movzx rax, al");
        break;
    case Token_Bitwise_Not:
        emitln("    not rax");
        break;
    default:
        break;
    }
}

void X86::emit_increment_expression(Unary_Expression *unary_expression, Register regs)
{
    // (++x): use the provided register for the operation
    // (x++): use an intermediate register for the operation
    //        the provided register remains unchanged
    // in both cases the designated object is directly affected by the increment
    emit_expression(unary_expression->operand, regs);

    if (unary_expression->order == Expression_Lhs) {
        emit_mov(&Rax, &regs, unary_expression->type.size);
        regs = Rax;
    }

    if (unary_expression->type.kind & (Type_Pointer)) {
        int64 size = unary_expression->type.pointed_type->size;
        if (unary_expression->operation.type & Token_Increment)
            emitln("    add {}, {}", regs[8], size);
        if (unary_expression->operation.type & Token_Decrement)
            emitln("    sub {}, {}", regs[8], size);
    } else {
        if (unary_expression->operation.type & Token_Increment)
            emitln("    inc {}", regs[8]);
        if (unary_expression->operation.type & Token_Decrement)
            emitln("    dec {}", regs[8]);
    }

    Source destination = emit_expression_source(unary_expression->operand, Rdi);
    emit_mov(&destination, &regs, unary_expression->type.size);
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
    emitln("    call {}", function->name.str);

    if (function->invoke_size != 0) {
        emitln("    add rsp, {}", function->invoke_size);
    }
    if (regs.gpr != Rdi.gpr) {
        emitln("    mov {}, rdi", regs[8]);
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
    Source destination = emit_expression_source(assign_expression->lhs, Rcx);

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
            emit_mov(&destination, &regs, size);
    }
}

void X86::emit_dot_expression(Dot_Expression *dot_expression, Register regs)
{
    Source source = emit_expression_source(dot_expression, regs);
    emit_mov(&regs, &source, dot_expression->member->type()->size);
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
        Source source = emit_expression_source(dot_expression->operand, regs);
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
        Source address = emit_expression_source(deref_expression->operand, regs);
        emit_mov(&regs, &address, 8);
        return regs.with_indirection();
    }

    case Expression_Unary: {
        emit_unary_expression((Unary_Expression *)expression, regs);
        return regs;
    }

    case Expression_Binary: {
        emit_binary_expression((Binary_Expression *)expression, regs);
        return regs;
    }

    default:
        qcc_todo("emit expression kind");
        return Source{};
    }
}

void X86::emit_deref_expression(Deref_Expression *deref_expression, Register regs)
{
    Register indirect = regs.with_indirection();
    emit_expression(deref_expression->operand, regs);
    emit_mov(&regs, &indirect, deref_expression->type->size);
}

void X86::emit_address_expression(Address_Expression *address_expression, Register regs)
{
    Source address = emit_expression_source(address_expression->operand, regs);
    emit_lea(&regs, &address);
}

void X86::emit_cast_expression(Cast_Expression *cast_expression, Register regs)
{
    const Type *from = cast_expression->from;
    const Type *into = &cast_expression->into;

    uint64 from_key = Cast_Type_Key(from->kind, from->mods);
    uint64 into_key = Cast_Type_Key(into->kind, into->mods);
    uint64 cast_key = Cast_Key(from_key, into_key);
    auto cast_match = Cast_Matrix.find(cast_key);

    emit_expression(cast_expression->operand, regs);
    if (cast_match != Cast_Matrix.end()) {
        std::string_view cast_asm = cast_match->second;
        emitf("    "), emitln(cast_asm, regs[into->size], regs[from->size]);
    }
}

void X86::emit_source_operand(const Source *source, int64 size)
{
    qcc_assert(size <= 8, "source does not fit in an x86 register");
    qcc_assert(!source->offset or (source->indirect or source->location & (Source_Stack | Source_Data)),
               "cannot offset a register without indirection");
    qcc_assert(!source->indirect or source->location & Source_Gpr, "cannot indirect a non register source");

    switch (source->location) {
    case Source_Stack:
        emitf("{} [rbp {:+}]", Spec[size], source->address + source->offset);
        break;
    case Source_Gpr:
        if (source->indirect) {
            emitf("[{} {:+}]", Gpr[source->gpr][8], source->offset);
        } else {
            emitf("{}", Gpr[source->gpr][size]);
        }
        break;
    default:
        qcc_todo("emit_source for this source type");
    }
}

void X86::emit_push(const Source *source)
{
    emitf("    push ");
    emit_source_operand(source, 8);
    emitln("");
}

void X86::emit_pop(const Source *source)
{
    emitf("    pop ");
    emit_source_operand(source, 8);
    emitln("");
}

void X86::emit_mov(const Source *destination, const Source *source, int64 size)
{
    emitf("    mov ");
    emit_source_operand(destination, size);
    emitf(", ");
    emit_source_operand(source, size);
    emitln("");
}

void X86::emit_lea(const Source *destination, const Source *source)
{
    emitf("    lea ");
    emit_source_operand(destination, 8);
    emitf(", ");
    emit_source_operand(source, 8);
    emitln("");
}

} // namespace qcc
