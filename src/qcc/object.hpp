#ifndef QCC_OBJECT_HPP
#define QCC_OBJECT_HPP

#include "fwd.hpp"
#include "type_system.hpp"

namespace qcc
{

enum Object_Kind : uint32
{
    Object_Function = Bit(uint32, 0),
    Object_Variable = Bit(uint32, 1),
    Object_Typedef = Bit(uint32, 2),
    Object_Record = Bit(uint32, 3),
    Object_Struct_Member = Bit(uint32, 4),
    Object_Enum_Member = Bit(uint32, 5),
};

struct Object
{
    Token name;

    virtual ~Object() = default;
    virtual Object_Kind kind() const = 0;
};

struct Function : Object
{
    Define_Statement *parameters;
    Type return_type;

    Object_Kind kind() const override
    {
        return Object_Function;
    }
};

struct Variable : Object
{
    Type type;

    union {
	int64 offset;
	int64 constant;
    };

    Object_Kind kind() const override
    {
        return Object_Variable;
    }
};

struct Typedef : Object
{
    Type type;

    Object_Kind kind() const override
    {
        return Object_Typedef;
    }
};

struct Record : Object
{
    Type type;

    Object_Kind kind() const override
    {
        return Object_Record;
    }
};

} // namespace qcc

#endif
