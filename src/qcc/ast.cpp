#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"

namespace qcc
{

Ast::~Ast()
{
    for (Statement *statement : statements)
        delete statement;
    for (Expression *expression : expressions)
        delete expression;
    for (Object *object : objects)
        delete object;
}

Variable *Ast::decode_designated_expression(Expression *expression)
{
    Id_Expression *id_expression = (Id_Expression *)search_designated_expression(expression);
    if (!id_expression or id_expression->kind() != Expression_Id)
        return NULL;
    if (id_expression->object->kind() != Object_Variable)
        return NULL;
    return (Variable *)id_expression->object;
}

Expression *Ast::search_designated_expression(Expression *expression)
{
    Expression *designated = NULL;

    // NOTE: do not implement cast expression even inferred ones
    switch (expression->kind()) {
    case Expression_Id:
        designated = expression;
        break;

    case Expression_Unary: {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;
        if (unary_expression->operation.type & (Token_Increment | Token_Decrement)) {
            designated = search_designated_expression(unary_expression->operand);
        }
        break;
    }

    case Expression_Binary: {
        Binary_Expression *binary_expression = (Binary_Expression *)expression;
        if (binary_expression->operation.type & (Token_Mask_Binary_Assign)) {
            designated = search_designated_expression(binary_expression->lhs);
        }
        break;
    }

    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        designated = search_designated_expression(nested_expression->operand);
        break;
    }

    default:
        break;
    }

    return designated;
}

void Ast::each_expression_of(Expression *expression, uint32 mask, std::vector<Expression *> &data)
{
    if (expression->kind() & mask)
        data.push_back(expression);

    switch (expression->kind()) {
    case Expression_Unary: {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;
        each_expression_of(unary_expression->operand, mask, data);
        break;
    }
    case Expression_Binary: {
        Binary_Expression *binary_expression = (Binary_Expression *)expression;
        each_expression_of(binary_expression->lhs, mask, data);
        each_expression_of(binary_expression->rhs, mask, data);
        break;
    }
    case Expression_Argument: {
        Argument_Expression *argument_expression = (Argument_Expression *)expression;
        each_expression_of(argument_expression->expression, mask, data);
        each_expression_of(argument_expression->next, mask, data);
        break;
    }
    case Expression_Invoke: {
        Invoke_Expression *invoke_expression = (Invoke_Expression *)expression;
        each_expression_of(invoke_expression->arguments, mask, data);
        break;
    }
    case Expression_Comma: {
        Comma_Expression *comma_expression = (Comma_Expression *)expression;
        each_expression_of(comma_expression->expression, mask, data);
        each_expression_of(comma_expression->next, mask, data);
        break;
    }
    case Expression_Ternary: {
        Ternary_Expression *ternary_expression = (Ternary_Expression *)expression;
        each_expression_of(ternary_expression->boolean, mask, data);
        each_expression_of(ternary_expression->expression_if, mask, data);
        each_expression_of(ternary_expression->expression_else, mask, data);
        break;
    }
    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        each_expression_of(nested_expression->operand, mask, data);
        break;
    }
    case Expression_Assign: {
        Assign_Expression *assign_expression = (Assign_Expression *)expression;
        each_expression_of(assign_expression->operand, mask, data);
        break;
    }
    case Expression_Cast: {
        Cast_Expression *cast_expression = (Cast_Expression *)expression;
        each_expression_of(cast_expression->expression, mask, data);
        break;
    }
    default:
        break;
    }
}

} // namespace qcc
