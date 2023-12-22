#ifndef QCC_TYPE_SYSTEM_HPP
#define QCC_TYPE_SYSTEM_HPP

#include "fwd.hpp"
#include "scan/token.hpp"
#include <fmt/core.h>
#include <vector>

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
    Type_Kind_Each = Bit(uint32, 11) - 1,
    Type_Scalar = (Type_Char | Type_Int | Type_Float | Type_Double | Type_Pointer),
    Type_Record = (Type_Struct | Type_Union | Type_Enum),
    Type_Gpr = (Type_Char | Type_Int | Type_Pointer | Type_Enum),
    Type_Fpr = (Type_Double | Type_Float),
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
    case Token_Void_Type:
        return Type_Void;
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
    default:
        return "?";
    }
}

struct Type
{
    Token token;
    size_t size;
    Type_Kind kind;
    uint32 mods;
    uint32 cvr;
    Type_Storage storage;

    union {
        Scope_Statement *scope;
        Type *pointed_type;
        Function *function;
        Type *array_type;
        Type *enum_type;
        void *data;
    };

    Type *base();
};

struct Type_System
{
    const Type void_type = {{}, 0, Type_Void};
    const Type int_type = {{}, 4, Type_Int, Type_Signed};
    const Type char_type = {{}, 1, Type_Char, 0};
    const Type float_type = {{}, 4, Type_Float};
    const Type double_type = {{}, 8, Type_Double};
    std::vector<Type *> orphan_types;

    Type *find_fundamental_type(Type_Kind kind, uint32 mods);
    Type *expression_type(Expression *expression);
    int32 expression_precedence(Expression *expression);
    uint32 cast(Type *from, Type *into);
    size_t scalar_size(Type_Kind kind, uint32 mods);
    size_t struct_size(Type *type);
    Type *merge(Type *destination, Type *source);
    Type *orphan_type_push(Type *type);
    std::string name(Type *type);

    Error errorf(std::string_view fmt, auto... args) const
    {
        return Error{"type_system error", fmt::format(fmt::runtime(fmt), args...)};
    }
};

} // namespace qcc

#endif
