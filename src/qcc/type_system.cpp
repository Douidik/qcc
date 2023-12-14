#include "type_system.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/format.h>
#include <ranges>

namespace qcc
{

Struct_Member::~Struct_Member()
{
    delete next;
}

Enum_Member::~Enum_Member()
{
    delete next;
}

void Type_System::init()
{
    void_type.kind = Type_Void;
    void_type.size = 0;
}

Type *Type_System::object_type(Object *object)
{
    switch (object->kind()) {
    case Object_Variable:
        return &((Variable *)object)->type;
    case Object_Typedef:
        return &((Typedef *)object)->type;
    default:
        return &void_type;
    }
}

Type *Type_System::expression_type(Expression *expression)
{
    switch (expression->kind()) {
    case Expression_Unary:
        return expression_type(((Unary_Expression *)expression)->operand);
    case Expression_Binary:
        return expression_type(((Binary_Expression *)expression)->lhs);
    case Expression_Invoke:
        return &((Invoke_Expression *)expression)->function->return_type;
    case Expression_Ternary:
        return &((Ternary_Expression *)expression)->type;
    case Expression_Int:
        return &((Int_Expression *)expression)->type;
    case Expression_Float:
        return &((Float_Expression *)expression)->type;
    default:
        return &void_type;
    }
}

uint32 Type_System::cast(Type *from, Type *into)
{
    uint32 kinds = from->kind | into->kind;

    switch (kinds) {
    case Type_Void:
        return Type_Cast_Same;
    case Type_Pointer:
        return cast(from->pointed_type, into->pointed_type);

    case Type_Function_Pointer: {
        Function *from_function = from->function;
        Function *into_function = into->function;

        uint32 return_type_cast = cast(&from_function->return_type, &into_function->return_type);
        if (return_type_cast & Type_Cast_Error)
            return Type_Cast_Error;

        Define_Statement *from_parameter = from_function->parameters;
        Define_Statement *into_parameter = into_function->parameters;
        uint32 worst_parameter_cast = Type_Cast_Same;

        while (from_parameter and into_parameter) {
            if (from_parameter != NULL ^ into_parameter != NULL)
                return Type_Cast_Error;
            Type *from_parameter_type = &from_parameter->type;
            Type *into_parameter_type = &into_parameter->type;
            worst_parameter_cast |= cast(from_parameter_type, into_parameter_type);

            from_parameter = from_parameter->next;
            into_parameter = into_parameter->next;
        }
        return worst_parameter_cast;
    }

    case Type_Union:
    case Type_Struct: {
        // TODO! struct type cast
        if (from->scope == into->scope)
            return Type_Cast_Same;
        return Type_Cast_Error;

        // if (from->scope->objects.size() != into->scope->objects.size())
        //     return Type_Cast_Error;

        // uint32 worst_member_cast = Type_Cast_Same;
        // auto from_it = std::views::keys(from->scope->objects);
        // auto into_it = std::views::keys(into->scope->objects);

        // // for (auto &[from_member, into_member] : std::views::zip(from_it, into_it)) {
        // // }

        // while (from_it != into->scope->objects.end() and into_it != into->scope->objects.end()) {
        // Struct_Member *from_member = (Struct_Member *
        // }

        // while (from_it != from->scope->objects.end() and into_it != into->scope->objects.end()) {
        //     if (from_parameter != NULL ^ into_parameter != NULL)
        //         return Type_Cast_Error;
        //     Type *from_parameter_type = &from_parameter->type;
        //     Type *into_parameter_type = &into_parameter->type;
        //     worst_parameter_cast |= cast(from_parameter_type, into_parameter_type);

        //     from_parameter = from_parameter->next;
        //     into_parameter = into_parameter->next;
        // }

        // for (const auto &[name, member] : from->scope->objects) {
        //     qcc_assert("non-struct-member object in struct scope", member->kind() & Object_Struct_Member);

        //     Struct_Member *from_member = from->struct_members;
        //     Struct_Member *into_member = from->struct_members;
        // }

        // Struct_Member *from_member = from->struct_members;
        // Struct_Member *into_member = from->struct_members;
        // uint32 worst_member_cast = Type_Cast_Same;

        // while (from_member and into_member) {
        //     if (from_member != NULL ^ into_member != NULL)
        //         return Type_Cast_Error;
        //     Type *from_member_type = &from_member->type;
        //     Type *into_member_type = &into_member->type;
        //     worst_member_cast |= cast(from_member_type, into_member_type);

        //     from_member = from_member->next;
        //     into_member = into_member->next;
        // }
        // return worst_member_cast;
    }

    default:
        break;
    }

    if (kinds & (Type_Scalar)) {
        uint32 cast = Type_Cast_Same;

        if ((kinds & Type_Float) and (kinds & Type_Int))
            cast |= Type_Cast_Inferred;
        if ((kinds & Type_Float) and (kinds & Type_Pointer))
            cast |= Type_Cast_Error;
        if (from->size > into->size)
            cast |= Type_Cast_Narrowed;
        return cast;
    }

    return Type_Cast_Error;
}

int64 Type_System::scalar_size(Type_Kind kind, uint32 mods)
{
    if (kind & Type_Char)
        return 1;
    if (kind & Type_Int and !mods)
        return 4;
    if (kind & Type_Int and mods & Type_Short)
        return 2;
    if (kind & Type_Int and mods & Type_Long)
        return 8;
    if (kind & Type_Float)
        return 4;
    if (kind & Type_Double)
        return 8;
    if (kind & Type_Pointer)
        return 8;
    return -1;
}

int64 Type_System::struct_size(Type *type)
{
    qcc_assert("type is not a struct or an union", type->kind & (Type_Struct | Type_Union));

    int64 size = 0;
    for (Object *object : std::views::values(type->scope->objects)) {
        qcc_assert("non-member object in struct declaration", object->kind() & Object_Struct_Member);
        Struct_Member *member = (Struct_Member *)object;
        size += member->type.size;
    }

    return size;
}

Type *Type_System::copy(Type *destination, Type *source)
{
    destination->kind = source->kind;
    destination->size = source->size;
    destination->mods = source->mods;
    destination->data = source->data;
    return destination;
}

std::string Type_System::name(Type *type)
{
    if (type->token.ok) {
        return std::string(type->token.str);
    }

    // Fallback-name
    std::vector<std::string_view> stream = {};
    if (type->storage != Type_Local) {
        stream.push_back(type_storage_name(type->storage));
    }
    for (uint32 cvr = 1; cvr != Type_Cvr_End; cvr <<= 1) {
        if (cvr & type->cvr)
            stream.push_back(type_cvr_name((Type_Cvr)cvr));
    }
    for (uint32 mod = 1; mod != Type_Mod_End; mod <<= 1) {
        if (mod & type->mods)
            stream.push_back(type_mod_name((Type_Mod)mod));
    }
    stream.push_back(type_kind_name(type->kind));
    return fmt::format("{}", fmt::join(stream, " "));
}

} // namespace qcc
