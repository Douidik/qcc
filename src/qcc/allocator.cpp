#include "allocator.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"

namespace qcc
{

Allocator::Allocator(Ast &ast, uint32 gpr_count, uint32 fpr_count) :
    ast(ast), gpr_count(gpr_count), fpr_count(fpr_count)
{
}

void Allocator::allocate()
{
    parse_statement_use_ranges(ast.main_statement);

    for (uint32 use_time = 0; use_time < uses_timeline.size(); use_time++) {
        for (Variable *variable : uses_timeline[use_time]) {
            if (use_time == uses_range[variable].begin)
                parse_begin_of_use(variable);
            if (use_time == uses_range[variable].end)
                parse_end_of_use(variable);
        }
    }

    for (Statement *statement : ast.main_statement->body) {
        if (statement->kind() & Statement_Function)
            parse_function_stack((Function_Statement *)statement);
    }

    // for (auto [variable, use_range] : uses_range) {
    //     std::string_view source_typename = "";
    //     switch (variable->source) {
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
    //     }

    //     fmt::println("'{}' ({}: {}) [{} - {}]", variable->name.str, source_typename, variable->stack_offset,
    //                  use_range.begin, use_range.end);
    // }
}

void Allocator::parse_function_stack(Function_Statement *function_statement)
{
    if (!function_statement->scope)
        return;

    Function *function = function_statement->function;
    size_t offset = 0;
    size_t alignment = 0;

    for (Variable *variable : function->locals) {
        if (variable->source & Source_Stack) {
            alignment = std::max(alignment, variable->type.size);
        }
    }
    for (Variable *variable : function->locals) {
        if (variable->source & Source_Stack) {
            // Snap the offset to the alignment
            size_t aligned = (offset / alignment) * alignment;
            if (offset + variable->type.size > aligned + alignment)
                variable->stack_offset = aligned + alignment + variable->type.size;
            else
                variable->stack_offset = offset + variable->type.size;
            offset = variable->stack_offset;
        }
    }
    function->stack_size = offset;
}

void Allocator::parse_begin_of_use(Variable *variable)
{
    if (variable->type.kind & Type_Gpr and gpr_count != 0) {
        variable->source = Source_Gpr, variable->gpr = gpr_count--;
    }
    if (variable->type.kind & Type_Fpr and fpr_count != 0) {
        variable->source = Source_Fpr, variable->fpr = fpr_count--;
    }
    if (!variable->source) {
        variable->source = Source_Stack;
    }
}

void Allocator::parse_end_of_use(Variable *variable)
{
    if (variable->source & Source_Gpr) {
        gpr_count++;
    }
    if (variable->source & Source_Fpr) {
        fpr_count++;
    }
}

void Allocator::parse_new_use(Variable *variable)
{
    if (variable->type.kind & Type_Void)
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

        if (function->parameters != NULL)
            parse_statement_use_ranges(function->parameters);
        if (function_statement->scope != NULL)
            parse_statement_use_ranges(function_statement->scope);
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
            if (define_statement->type & Define_Parameter or define_statement->expression != NULL)
                parse_new_use(define_statement->variable);
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
        parse_expression_use_ranges(argument_expression->expression);
        break;
    }

    case Expression_Invoke: {
        Invoke_Expression *invoke_expression = (Invoke_Expression *)expression;
	if (uses_timeline.size() != 0)
	    invoke_expression->use_time = uses_timeline.size();
        parse_expression_use_ranges(invoke_expression->arguments);
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
        parse_new_use(assign_expression->variable);
        parse_expression_use_ranges(assign_expression->operand);
        break;
    }

    case Expression_Cast: {
        Cast_Expression *cast_expression = (Cast_Expression *)expression;
        parse_expression_use_ranges(cast_expression->expression);
        break;
    }

    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        parse_new_use(dot_expression->from);
        break;
    }

    default:
        break;
    }
}

} // namespace qcc
