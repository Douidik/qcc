#include "allocator.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <ranges>

namespace qcc
{

Allocator::Allocator(Ast &ast, int32 gpr_count, int32 fpr_count) :
    ast(ast), gpr_count(gpr_count), fpr_count(fpr_count)
{
}

void Allocator::allocate()
{
    parse_statement_use_ranges(ast.main_statement);

    for (uint32 use_time = 0; use_time < uses_timeline.size(); use_time++) {
        for (Variable *variable : uses_timeline[use_time]) {
            Use_Range use_range = uses_range[variable];

            if (use_time == use_range.begin)
                parse_begin_of_use(variable);
            if (use_time == use_range.end)
                parse_end_of_use(variable);
        }
    }

    for (Statement *statement : ast.main_statement->body) {
        if (statement->kind() & Statement_Function)
            create_function_stack((Function_Statement *)statement);
    }

    // for (auto [variable, use_range] : uses_range) {
    //     std::string_view source_typename = "";
    //     switch (variable->location) {
    //     case Source_None:
    //         source_typename = "none";
    //         break;
    //     case Source_Stack:
    //         source_typename = "stack";
    //         break;
    //     case Source_Gpr:
    //         source_typename = "gpr";
    //         break;
    //     case Source_Fpr:
    //         source_typename = "fpr";
    //         break;
    //     default:
    //         source_typename = "?";
    //         break;
    //     }

    //     fmt::println("'{}' ({}: {:+}) [{} {}]", variable->name.str, source_typename, variable->address,
    //                  use_range.begin, use_range.end);
    // }
}

int64 Allocator::create_function_stack_push(Variable *variable, int64 offset, int64 alignment)
{
    int64 size = variable->type()->size;
    int64 address = Round_Up(offset, alignment);

    if (variable->env & Define_Parameter) {
        // __cdecl function: parameters are at the top of the invoker function
        // we offset by 16 because we pushed the return address and the bsp
        variable->address = +(address + 16);
    } else {
        variable->address = -(address + size);
    }

    // Todo! maybe not needed since we can only access the struct members by a dot expression
    // that generates the address by offseting a pre-parsed value to the struct address
    // if (variable->type.kind & (Type_Struct | Type_Union)) {
    //     Struct_Statement *struct_statement = variable->type.struct_statement;
    //     int64 struct_alignment = variable->type.alignment();

    //     for (Variable *member : views::values(struct_statement->members)) {
    //         member->location = variable->location;
    //         member->mode |= variable->mode;

    //         if (variable->type.kind & Type_Struct) {
    //             offset = create_function_stack_push(member, offset, struct_alignment);
    //         }
    //         if (variable->type.kind & Type_Union) {
    //             create_function_stack_push(member, offset, struct_alignment);
    //         }
    //     }
    // }
    return address + size;
}

void Allocator::create_function_stack(Function_Statement *function_statement)
{
    if (!function_statement->scope)
        return;

    auto on_stack = [](Variable *variable) -> bool {
        return variable->location & Source_Stack;
    };

    Function *function = function_statement->function;
    size_t offset;
    size_t alignment;
    Define_Statement *parameter;

    offset = 0;
    for (parameter = function->parameters; parameter != NULL; parameter = parameter->next) {
        offset += create_function_stack_push(parameter->variable, offset, 8);
    }
    function->invoke_size = Round_Up(offset, 8);

    offset = 0, alignment = 0;
    for (Variable *variable : function->locals | views::filter(on_stack)) {
        alignment = Max(alignment, variable->type()->alignment());
    }
    for (Variable *variable : function->locals | views::filter(on_stack)) {
        offset += create_function_stack_push(variable, offset, alignment);
    }
    function->stack_size = Round_Up(offset, 8);
}

void Allocator::parse_begin_of_use(Variable *variable)
{
    if (variable->location != Source_None) {
        return;
    }

    else if (variable->type()->kind & Type_Aggregate or variable->env & Define_Parameter) {
        variable->location = Source_Stack;
    }

    else if (variable->type()->kind & Type_Gpr and gpr_count != 0) {
        variable->location = Source_Gpr, variable->gpr = gpr_count--;
    }

    else if (variable->type()->kind & Type_Fpr and fpr_count != 0) {
        variable->location = Source_Fpr, variable->fpr = fpr_count--;
    }

    else if (!variable->location) {
        variable->location = Source_Stack;
    }
}

void Allocator::parse_end_of_use(Variable *variable)
{
    if (variable->location & Source_Gpr) {
        gpr_count++;
    }
    if (variable->location & Source_Fpr) {
        fpr_count++;
    }
}

void Allocator::parse_new_use(Variable *variable)
{
    if (variable->type()->kind & Type_Void)
        return;

    if (uses_range.contains(variable)) {
        for (uint32 use_time = uses_range[variable].end; use_time < uses_timeline.size(); use_time++) {
            uses_timeline[use_time].insert(variable);
        }
    } else {
        uses_range[variable].begin = uses_timeline.size();
    }

    uses_range[variable].end = uses_timeline.size();
    uses_timeline.push_back(std::set{variable});
}

void Allocator::parse_statement_use_ranges(Statement *statement)
{
    switch (statement->kind()) {
    case Statement_Scope: {
        Scope_Statement *scope_statement = (Scope_Statement *)statement;

        for (Statement *body_statement : scope_statement->body) {
            parse_statement_use_ranges(body_statement);
        }
        break;
    }

    case Statement_Function: {
        Function_Statement *function_statement = (Function_Statement *)statement;
        Function *function = function_statement->function;

        if (function->parameters != NULL and function_statement->scope != NULL) {
            parse_statement_use_ranges(function->parameters);
        }
        if (function_statement->scope != NULL) {
            parse_statement_use_ranges(function_statement->scope);
        }
        break;
    }

    case Statement_Condition: {
        Condition_Statement *condition_statement = (Condition_Statement *)statement;
        parse_statement_use_ranges(condition_statement->statement_if);
        if (condition_statement->statement_else != NULL)
            parse_statement_use_ranges(condition_statement->statement_else);
        break;
    }

    case Statement_While: {
        While_Statement *while_statement = (While_Statement *)statement;
        parse_statement_use_ranges(while_statement->statement);
        break;
    }

    case Statement_For: {
        For_Statement *for_statement = (For_Statement *)statement;
        parse_statement_use_ranges(for_statement->statement);
        break;
    }

    case Statement_Define: {
        Define_Statement *define_statement = (Define_Statement *)statement;

        for (; define_statement != NULL; define_statement = define_statement->next) {
            Variable *variable = define_statement->variable;

            if (variable->env & Define_Parameter or define_statement->expression != NULL)
                parse_new_use(variable);
            if (define_statement->expression != NULL)
                parse_expression_use_ranges(define_statement->expression);
        }
        break;
    }

    case Statement_Return: {
        Return_Statement *return_statement = (Return_Statement *)statement;
        parse_expression_use_ranges(return_statement->expression);
        break;
    }

    case Statement_Expression: {
        Expression_Statement *expression_statement = (Expression_Statement *)statement;
        parse_expression_use_ranges(expression_statement->expression);
        break;
    }

    default:
        break;
    }
}

void Allocator::parse_expression_use_ranges(Expression *expression)
{
    switch (expression->kind()) {
    case Expression_Unary: {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;
        parse_expression_use_ranges(unary_expression->operand);
        break;
    }

    case Expression_Binary: {
        Binary_Expression *binary_expression = (Binary_Expression *)expression;
        parse_expression_use_ranges(binary_expression->lhs);
        parse_expression_use_ranges(binary_expression->rhs);
        break;
    }

    case Expression_Argument: {
        Argument_Expression *argument_expression = (Argument_Expression *)expression;
        Assign_Expression *assign_expression = argument_expression->assign_expression;
        parse_expression_use_ranges(assign_expression->rhs);
        if (argument_expression->next != NULL)
            parse_expression_use_ranges(argument_expression->next);
        break;
    }

    case Expression_Invoke: {
        Invoke_Expression *invoke_expression = (Invoke_Expression *)expression;
        if (invoke_expression->arguments != NULL)
            parse_expression_use_ranges(invoke_expression->arguments);
        invoke_expression->use_time = Max((int64)uses_timeline.size() - 1, 0L);
        break;
    }

    case Expression_Comma: {
        Comma_Expression *comma_expression = (Comma_Expression *)expression;
        parse_expression_use_ranges(comma_expression->expression);
        parse_expression_use_ranges(comma_expression->next);
        break;
    }

    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)expression;
        if (id_expression->object->kind() & Object_Variable) {
            parse_new_use((Variable *)id_expression->object);
        }
        break;
    }

    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        parse_expression_use_ranges(nested_expression->operand);
        break;
    }

    case Expression_Assign: {
        Assign_Expression *assign_expression = (Assign_Expression *)expression;
        parse_expression_use_ranges(assign_expression->lhs);
        parse_expression_use_ranges(assign_expression->rhs);
        break;
    }

    case Expression_Cast: {
        Cast_Expression *cast_expression = (Cast_Expression *)expression;
        parse_expression_use_ranges(cast_expression->operand);
        break;
    }

    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        parse_expression_use_ranges(dot_expression->operand);
        break;
    }

    case Expression_Deref: {
        Deref_Expression *deref_expression = (Deref_Expression *)expression;
        parse_expression_use_ranges(deref_expression->operand);
        break;
    }

    case Expression_Address: {
        Address_Expression *address_expression = (Address_Expression *)expression;
        parse_expression_use_ranges(address_expression->operand);
        break;
    }

    case Expression_Ref: {
        Ref_Expression *ref_expression = (Ref_Expression *)expression;
        if (ref_expression->object->kind() & Object_Variable) {
            parse_new_use((Variable *)ref_expression->object);
        }
        break;
    }

    default:
        break;
    }
}

} // namespace qcc
