#ifndef QCC_EXPRESSION_HPP
#define QCC_EXPRESSION_HPP

#include "fwd.hpp"
#include "scan/token.hpp"
#include "type_system.hpp"

namespace qcc
{

enum Expression_Kind : uint32
{
    Expression_Unary = Bit(uint32, 0),
    Expression_Binary = Bit(uint32, 1),
    Expression_Argument = Bit(uint32, 2),
    Expression_Invoke = Bit(uint32, 3),
    Expression_Comma = Bit(uint32, 4),
    Expression_Ternary = Bit(uint32, 5),
    Expression_String = Bit(uint32, 6),
    Expression_Int = Bit(uint32, 7),
    Expression_Float = Bit(uint32, 8),
    Expression_Id = Bit(uint32, 9),
    Expression_Nested = Bit(uint32, 10),
    Expression_Assign = Bit(uint32, 11),
    Expression_Cast = Bit(uint32, 12),
    Expression_Dot = Bit(uint32, 13),
};

enum Expression_Order : uint32
{
    Expression_Lhs,
    Expression_Rhs,
};

struct Expression
{
    virtual ~Expression() = default;
    virtual Expression_Kind kind() const = 0;
};

struct Unary_Expression : Expression
{
    Token operation;
    Expression *operand;
    Expression_Order order;
    Type *type;

    Expression_Kind kind() const override
    {
        return Expression_Unary;
    }
};

struct Binary_Expression : Expression
{
    Token operation;
    Expression *lhs;
    Expression *rhs;
    Type *type;

    Expression_Kind kind() const override
    {
        return Expression_Binary;
    }
};

struct Argument_Expression : Expression
{
    Expression *expression;
    Variable *parameter;
    Argument_Expression *previous;
    Argument_Expression *next;

    Expression_Kind kind() const override
    {
        return Expression_Argument;
    }
};

struct Invoke_Expression : Expression
{
    Function *function;
    Argument_Expression *arguments;
    uint32 use_time; // used for save used registers when invoking

    Expression_Kind kind() const override
    {
        return Expression_Invoke;
    }
};

struct Comma_Expression : Expression
{
    Expression *expression;
    Expression *next;

    Expression_Kind kind() const override
    {
        return Expression_Comma;
    }
};

struct String_Expression : Expression
{
    std::string string;

    Expression_Kind kind() const override
    {
        return Expression_String;
    }
};

struct Ternary_Expression : Expression
{
    Type type;
    Expression *boolean;
    Expression *expression_if;
    Expression *expression_else;

    Expression_Kind kind() const override
    {
        return Expression_Ternary;
    }
};

enum Int_Flag : uint32
{
    Int_U = Bit(uint32, 0),
    Int_L = Bit(uint32, 1),
    Int_LL = Bit(uint32, 2),
};

struct Int_Expression : Expression
{
    Type type;
    uint64 value;
    uint32 flags;

    Expression_Kind kind() const override
    {
        return Expression_Int;
    }
};

struct Float_Expression : Expression
{
    Type type;
    float64 value;

    Expression_Kind kind() const override
    {
        return Expression_Float;
    }
};

struct Id_Expression : Expression
{
    Object *object;
    Token token;

    std::string_view str() const
    {
        return token.str;
    }

    Expression_Kind kind() const override
    {
        return Expression_Id;
    }
};

struct Nested_Expression : Expression
{
    Expression *operand;

    Expression_Kind kind() const override
    {
        return Expression_Nested;
    }
};

struct Assign_Expression : Expression
{
    Variable *variable;
    Expression *operand;

    Expression_Kind kind() const override
    {
        return Expression_Assign;
    }
};

struct Cast_Expression : Expression
{
    Type *from;
    Type *into;
    Expression *expression;

    Expression_Kind kind() const override
    {
        return Expression_Cast;
    }
};

struct Dot_Expression : Expression
{
    Variable *from;
    Variable *member;

    Expression_Kind kind() const override
    {
        return Expression_Dot;
    }
};

} // namespace qcc

#endif
