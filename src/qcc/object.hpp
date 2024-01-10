#ifndef QCC_OBJECT_HPP
#define QCC_OBJECT_HPP

#include "fwd.hpp"
#include "source.hpp"
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
    Object_String = Bit(uint32, 4),
};

struct Object
{
    Token name;

    virtual ~Object() = default;
    virtual Object_Kind kind() const = 0;
    virtual Type *type() = 0;

    virtual Source *source() const
    {
        return NULL;
    }

    virtual bool has_assign() const
    {
        return false;
    };
};

struct Function : Object
{
    Define_Statement *parameters;
    std::vector<Variable *> locals;
    Type return_type;
    int64 invoke_size;
    int64 stack_size;
    bool is_main;

    Object_Kind kind() const override
    {
        return Object_Function;
    }

    Type *type() override
    {
        qcc_todo("type function objects");
        return NULL;
    }
};

enum Define_Env : uint32
{
    Define_Unknown = Bit(uint32, 0),
    Define_Struct = Bit(uint32, 1),
    Define_Union = Bit(uint32, 2),
    Define_Enum = Bit(uint32, 3),
    Define_Parameter = Bit(uint32, 4),
    Define_Var = Bit(uint32, 5),
};

constexpr std::string_view define_env_str(uint32 env)
{
    switch (env) {
    case Define_Struct:
        return "struct-member";
    case Define_Union:
        return "union-member";
    case Define_Enum:
        return "enum-member";
    case Define_Parameter:
        return "parameter";
    case Define_Var:
        return "variable";
    default:
        return "?";
    }
}

struct Variable : Object, Source
{
    Type var_type;
    uint32 env;

    union {
        int64 enum_constant;
        int64 struct_offset;
        int64 meta;
    };

    Source *source() const override
    {
        return (Source *)this;
    }

    Object_Kind kind() const override
    {
        return Object_Variable;
    }

    Type *type() override
    {
        return &var_type;
    }

    bool has_assign() const override
    {
        return !(var_type.cvr & Type_Const);
    }
};

struct Typedef : Object
{
    Type typedef_type;

    Object_Kind kind() const override
    {
        return Object_Typedef;
    }

    Type *type() override
    {
        return &typedef_type;
    }
};

struct Record : Object
{
    Type record_type;

    Object_Kind kind() const override
    {
        return Object_Record;
    }

    Type *type() override
    {
        return &record_type;
    }
};

struct String : Object, Source
{
    std::string data;
    Type string_type;

    Source *source() const override
    {
        return (Source *)this;
    }

    Object_Kind kind() const override
    {
        return Object_Variable;
    }

    Type *type() override
    {
        return &string_type;
    }
};

} // namespace qcc

#endif
