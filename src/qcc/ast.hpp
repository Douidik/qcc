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
    
    template <typename T, typename D = std::decay_t<T>>
    T *push(T *value)
    {
        if constexpr (std::is_base_of_v<D, Statement>) {
            return statements.emplace_back(value);
        }
        if constexpr (std::is_base_of_v<D, Expression>) {
            return expressions.emplace_back(value);
        }
        if constexpr (std::is_base_of_v<D, Object>) {
            return objects.emplace_back(value);
        }
    }
};

} // namespace qcc

#endif
