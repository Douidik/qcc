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
    Statement_Struct = Bit(uint32, 1),
    Statement_Function = Bit(uint32, 2),
    Statement_Define = Bit(uint32, 3),
    Statement_Expression = Bit(uint32, 4),
    Statement_Condition = Bit(uint32, 5),
    Statement_While = Bit(uint32, 6),
    Statement_For = Bit(uint32, 7),
    Statement_Jump = Bit(uint32, 8),
    Statement_Record = Bit(uint32, 9),
    Statement_Return = Bit(uint32, 10),
    Statement_Kind_Each = Bit(uint32, 11) - 1,
};

constexpr std::string_view statement_kind_str(Statement_Kind kind)
{
    switch (kind) {
    case Statement_Scope:
        return "scope";
    case Statement_Struct:
        return "struct";
    case Statement_Function:
        return "function";
    case Statement_Define:
        return "define";
    case Statement_Expression:
        return "expression";
    case Statement_Condition:
        return "condition";
    case Statement_While:
        return "while";
    case Statement_For:
        return "for";
    case Statement_Jump:
        return "jump";
    case Statement_Record:
        return "record";
    case Statement_Return:
        return "return";
    default:
        return "?";
    }
}

struct Statement
{
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

struct Struct_Statement : Statement
{
    uint64 hash;
    Token keyword;
    std::unordered_map<std::string_view, Variable *> members;
    
    Statement_Kind kind() const override
    {
        return Statement_Struct;
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

struct Return_Statement : Statement
{
    Expression *expression;
    Function *function;

    Statement_Kind kind() const override
    {
        return Statement_Return;
    }
};

} // namespace qcc

#endif
