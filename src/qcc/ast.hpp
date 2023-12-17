#ifndef QCC_AST_HPP
#define QCC_AST_HPP

#include "fwd.hpp"
#include "type_system.hpp"
#include <vector>

namespace qcc
{

struct Ast
{
    Type_System type_system;
    std::vector<Statement *> statements;
    std::vector<Expression *> expressions;
    std::vector<Object *> objects;
    Scope_Statement *main_statement;

    ~Ast();
    Variable *decode_designated_expression(Expression *expression);
    Expression *search_designated_expression(Expression *expression);

    template <typename T, typename D = std::decay_t<T>>
    T *push(T *value)
    {
        if constexpr (std::is_base_of_v<Statement, D>) {
            return (T *)statements.emplace_back(value);
        }
        if constexpr (std::is_base_of_v<Expression, D>) {
            return (T *)expressions.emplace_back(value);
        }
        if constexpr (std::is_base_of_v<Object, D>) {
            return (T *)objects.emplace_back(value);
        }
	return NULL;
    }
};

} // namespace qcc

#endif
