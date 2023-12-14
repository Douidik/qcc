#ifndef QCC_TYPE_SYSTEM_HPP
#define QCC_TYPE_SYSTEM_HPP

#include "fwd.hpp"
#include "scan/token.hpp"
#include <fmt/core.h>

namespace qcc
{

enum Type_Cast : uint32
{
    Type_Cast_Same = Bit(uint32, 0),
    Type_Cast_Inferred = Bit(uint32, 1),
    Type_Cast_Narrowed = Bit(uint32, 2),
    Type_Cast_Transmuted = Bit(uint32, 3),
    Type_Cast_Error = Bit(uint32, 4),
};

enum Type_Kind : uint32
{
    Type_Undefined = 0,
    Type_Char = Bit(uint32, 0),
    Type_Int = Bit(uint32, 1),
    Type_Float = Bit(uint32, 2),
    Type_Double = Bit(uint32, 3),
    Type_Pointer = Bit(uint32, 4),
    Type_Struct = Bit(uint32, 5),
    Type_Union = Bit(uint32, 6),
    Type_Enum = Bit(uint32, 7),
    Type_Array = Bit(uint32, 8),
    Type_Function_Pointer = Bit(uint32, 9),
    Type_Void = Bit(uint32, 10),
    Type_Scalar = (Type_Char | Type_Int | Type_Float | Type_Double | Type_Pointer),
    Type_Record = (Type_Struct | Type_Union | Type_Enum),
};

constexpr std::string_view type_kind_name(Type_Kind kind)
{
    switch (kind) {
    case Type_Char:
        return "char";
    case Type_Int:
        return "int";
    case Type_Float:
        return "float";
    case Type_Double:
        return "double";
    case Type_Pointer:
        return "pointer";
    case Type_Struct:
        return "struct";
    case Type_Union:
        return "union";
    case Type_Enum:
        return "enum";
    case Type_Array:
        return "array";
    case Type_Function_Pointer:
        return "function-pointer";
    case Type_Void:
        return "void";
    default:
        return "?";
    }
}

constexpr Type_Kind token_to_type_kind(Token_Type type)
{
    switch (type) {
    case Token_Char_Type:
        return Type_Char;
    case Token_Int_Type:
        return Type_Int;
    case Token_Float_Type:
        return Type_Float;
    case Token_Double_Type:
        return Type_Double;
    case Token_Struct:
        return Type_Struct;
    case Token_Union:
        return Type_Union;
    case Token_Enum:
        return Type_Enum;
    default:
        return Type_Undefined;
    }
}

enum Type_Mod : uint32
{
    Type_Signed = Bit(uint32, 0),
    Type_Unsigned = Bit(uint32, 1),
    Type_Short = Bit(uint32, 2),
    Type_Long = Bit(uint32, 3),
    Type_Mod_End = Bit(uint32, 4),
};

constexpr std::string_view type_mod_name(Type_Mod mod)
{
    switch (mod) {
    case Type_Signed:
        return "signed";
    case Type_Unsigned:
        return "unsigned";
    case Type_Short:
        return "short";
    case Type_Long:
        return "long";
    default:
        return "?";
    }
}

enum Type_Cvr : uint32
{
    Type_Const = Bit(uint32, 0),
    Type_Volatile = Bit(uint32, 1),
    Type_Restrict = Bit(uint32, 2),
    Type_Cvr_End = Bit(uint32, 3),
};

constexpr std::string_view type_cvr_name(Type_Cvr cvr)
{
    switch (cvr) {
    case Type_Const:
        return "const";
    case Type_Volatile:
        return "volatile";
    case Type_Restrict:
        return "restrict";
    default:
        return "?";
    }
}

enum Type_Storage : uint32
{
    Type_Local = 0,
    Type_Extern = 1,
    Type_Register = 2,
    Type_Static = 3,
    Type_Auto = 4,
};

constexpr std::string_view type_storage_name(Type_Storage storage)
{
    switch (storage) {
    case Type_Local:
        return "local";
    case Type_Extern:
        return "extern";
    case Type_Register:
        return "register";
    case Type_Static:
        return "static";
    case Type_Auto:
        return "auto";
    }
}

struct Type
{
    Token token;
    int64 size;
    Type_Kind kind;
    uint32 mods;
    uint32 cvr;
    Type_Storage storage;

    union {
        struct
        {
	    Scope_Statement *scope;
            Function *function;
            Type *array_type;
            Type *enum_type;
            Type *pointed_type;
        };
        void *data;
    };
};

// struct Struct_Member
// {
//     ~Struct_Member();

//     Type type;
//     Token name;
//     size_t offset;
//     Struct_Member *next;
// };

// struct Enum_Member
// {
//     ~Enum_Member();

//     Type type;
//     Token name;
//     Int_Expression *expression;
//     Enum_Member *next;
// };

struct Type_System
{
    Type void_type;

    void init();
    Type *find_fundamental_type(Type_Kind kind, uint32 mods);

    Type *object_type(Object *object);
    Type *expression_type(Expression *expression);
    uint32 cast(Type *from, Type *into);
    int64 scalar_size(Type_Kind kind, uint32 mods);
    int64 struct_size(Type *type);
    Type *copy(Type *destination, Type *source);
    std::string name(Type *type);

    Error errorf(std::string_view fmt, auto... args) const
    {
        return Error{"type_system error", fmt::format(fmt::runtime(fmt), args...)};
    }
};

} // namespace qcc

#endif
