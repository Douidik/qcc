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

Object *Ast::decode_designated_expression(Expression *expression)
{
    Expression *designated = search_designated_expression(expression);
    if (!designated)
        return NULL;

    switch (designated->kind()) {
    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)designated;
        return id_expression->object;
    }

    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)designated;
        return dot_expression->member;
    }

    case Expression_Deref: {
        Deref_Expression *deref_expression = (Deref_Expression *)designated;
        return deref_expression->object;
    }

    case Expression_Address: {
        Address_Expression *address_expression = (Address_Expression *)designated;
        return address_expression->object;
    }

    default:
        qcc_assert("decode_designated_expression() unexpected expression of type", 0);
        return NULL;
    }
}

Expression *Ast::search_designated_expression(Expression *expression)
{
    Expression *designated = NULL;

    switch (expression->kind()) {
    case Expression_Id:
    case Expression_Dot:
    case Expression_Deref:
    case Expression_Address:
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

} // namespace qcc
