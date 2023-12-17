#ifndef QCC_STATEMENT_HPP
#define QCC_STATEMENT_HPP

#include "fwd.hpp"
#include "scan/token.hpp"
#include "type_system.hpp"
#include <unordered_map>
#include <vector>

namespace qcc
{

enum Statement_Kind : uint32
{
    Statement_Scope = Bit(uint32, 0),
    Statement_Function = Bit(uint32, 1),
    Statement_Define = Bit(uint32, 2),
    Statement_Expression = Bit(uint32, 3),
    Statement_Condition = Bit(uint32, 4),
    Statement_While = Bit(uint32, 5),
    Statement_For = Bit(uint32, 6),
    Statement_Jump = Bit(uint32, 7),
    Statement_Record = Bit(uint32, 8),
};

struct Statement
{
    Token token;

    virtual ~Statement() = default;
    virtual Statement_Kind kind() const = 0;
};

struct Scope_Statement : Statement
{
    Scope_Statement *owner;
    std::vector<Statement *> body;
    std::unordered_map<std::string_view, Object *> objects;
    std::unordered_map<std::string_view, Record *> records;

    Object *object(std::string_view name);
    Record *record(Type_Kind kind, std::string_view name);

    Statement_Kind kind() const override
    {
        return Statement_Scope;
    }
};

struct Function_Statement : Statement
{
    Function *function;
    Scope_Statement *scope;

    Statement_Kind kind() const override
    {
        return Statement_Function;
    }
};

struct Define_Statement : Statement
{
    Expression *expression;
    Variable *variable;
    Define_Statement *next;

    Statement_Kind kind() const override
    {
        return Statement_Define;
    }
};

struct Expression_Statement : Statement
{
    Expression *expression;

    Statement_Kind kind() const override
    {
        return Statement_Expression;
    }
};

struct Condition_Statement : Statement
{
    Expression *boolean;
    Scope_Statement *statement_if;
    Scope_Statement *statement_else;

    Statement_Kind kind() const override
    {
        return Statement_Condition;
    }
};

struct While_Statement : Statement
{
    Expression *boolean;
    Scope_Statement *statement;

    Statement_Kind kind() const override
    {
        return Statement_While;
    }
};

struct For_Statement : Statement
{
    Expression *init;
    Expression *boolean;
    Expression *loop;
    Scope_Statement *statement;

    Statement_Kind kind() const override
    {
        return Statement_For;
    }
};

struct Jump_Statement : Statement
{
    Statement_Kind kind() const override
    {
        return Statement_Jump;
    }
};

struct Record_Statement : Statement
{
    Type *type;

    Statement_Kind kind() const override
    {
        return Statement_Record;
    }
};

} // namespace qcc

#endif
