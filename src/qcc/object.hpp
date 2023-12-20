#ifndef QCC_OBJECT_HPP
#define QCC_OBJECT_HPP

#include "fwd.hpp"
#include "type_system.hpp"
#include <vector>

namespace qcc
{

enum Object_Kind : uint32
{
    Object_Function = Bit(uint32, 0),
    Object_Variable = Bit(uint32, 1),
    Object_Typedef = Bit(uint32, 2),
    Object_Record = Bit(uint32, 3),
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
    std::vector<Variable *> locals;
    Type return_type;
    size_t stack_size;
    bool is_main;
    
    Object_Kind kind() const override
    {
        return Object_Function;
    }
};

enum Source_Type : uint32
{
    Source_None = 0,
    Source_Stack = Bit(uint32, 1),
    Source_Gpr = Bit(uint32, 2),
    Source_Fpr = Bit(uint32, 3),
};

struct Variable : Object
{
    Type type;
    int64 constant;
    Source_Type source;

    union {
	int64 gpr;
	int64 fpr;
	int64 stack_offset;
	int64 data_offset;
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
