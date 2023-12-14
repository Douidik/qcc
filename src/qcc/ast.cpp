#include "ast.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "object.hpp"

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

} // namespace qcc
