#include "type_system.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <ranges>

namespace qcc
{

std::string Type::name()
{
    if (token.ok) {
        return std::string(token.str);
    }

    // Generate a fallback type name if the token is unavailable
    std::vector<std::string_view> stream = {};
    if (storage != Type_Local) {
        stream.push_back(type_storage_name(storage));
    }
    for (uint32 cvr_mask = 1; cvr_mask != Type_Cvr_End; cvr_mask <<= 1) {
        if (cvr & cvr_mask)
            stream.push_back(type_cvr_name((Type_Cvr)cvr_mask));
    }
    for (uint32 mods_mask = 1; mods_mask != Type_Mod_End; mods_mask <<= 1) {
        if (mods_mask != Type_Signed and mods_mask & mods)
            stream.push_back(type_mod_name((Type_Mod)mods_mask));
    }
    stream.push_back(type_kind_name(kind));
    return fmt::format("{}", fmt::join(stream, " "));
}

Type *Type::base()
{
    switch (kind) {
    case Type_Array:
        return array_type->base();
    case Type_Pointer:
        return pointed_type->base();
    case Type_Enum:
        return enum_type;
    default:
        return this;
    }
}

size_t Type::alignment()
{
    switch (kind) {
    case Type_Union:
    case Type_Struct: {
        size_t struct_alignment = 0;
        for (auto [_, member] : struct_statement->members) {
            struct_alignment = std::max(struct_alignment, member->type()->alignment());
        }
        return struct_alignment;
    }

    case Type_Array:
        return array_type->alignment();
    default:
        return size;
    }
}

Type_System::~Type_System()
{
    for (Type *type : orphan_types) {
        delete type;
    }
}

Type *Type_System::expression_type(Expression *expression)
{
    switch (expression->kind()) {
    case Expression_Unary:
        return ((Unary_Expression *)expression)->type;
    case Expression_Binary:
        return ((Binary_Expression *)expression)->type;
    case Expression_Cast:
        return ((Cast_Expression *)expression)->into;
    case Expression_Ref:
        return ((Ref_Expression *)expression)->type;
    case Expression_Assign:
        return ((Assign_Expression *)expression)->type;
    case Expression_Invoke:
        return &((Invoke_Expression *)expression)->function->return_type;
    case Expression_Ternary:
        return &((Ternary_Expression *)expression)->type;
    case Expression_Int:
        return &((Int_Expression *)expression)->type;
    case Expression_Float:
        return &((Float_Expression *)expression)->type;
    case Expression_Address:
        return &((Address_Expression *)expression)->type;
    case Expression_Deref:
        return ((Deref_Expression *)expression)->type;
    case Expression_Subscript:
        return ((Subscript_Expression *)expression)->type;
    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)expression;
        return id_expression->object->type();
    }
    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        return dot_expression->member->type();
    }
    case Expression_Comma: {
        Comma_Expression *comma_expression = (Comma_Expression *)expression;
        return expression_type(comma_expression->next);
    }
    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        return expression_type(nested_expression->operand);
    }

    default:
        qcc_todo("type expression kind");
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
            Type *from_parameter_type = from_parameter->variable->type();
            Type *into_parameter_type = into_parameter->variable->type();
            worst_parameter_cast |= cast(from_parameter_type, into_parameter_type);

            from_parameter = from_parameter->next;
            into_parameter = into_parameter->next;
        }
        return worst_parameter_cast;
    }
    case Type_Union:
    case Type_Struct: {
        if (from->struct_statement->hash != into->struct_statement->hash)
            return Type_Cast_Error;
        return Type_Cast_Same;
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

size_t Type_System::scalar_size(Type_Kind kind, uint32 mods)
{
    if (kind & Type_Char)
        return 1;
    if (kind & Type_Int and mods & Type_Short)
        return 2;
    if (kind & Type_Int and mods & Type_Long)
        return 8;
    if (kind & Type_Int)
        return 4;
    if (kind & Type_Float)
        return 4;
    if (kind & Type_Double)
        return 8;
    if (kind & Type_Pointer)
        return 8;
    qcc_todo("support for type");
    return -1;
}

size_t Type_System::struct_size(Type *type)
{
    size_t size = 0;

    for (Variable *member : std::views::values(type->struct_statement->members)) {
        if (type->kind & Type_Union)
            size = std::max(size, member->type()->size);
        if (type->kind & Type_Struct)
            size += member->type()->size;
    }
    return size;
}

Type *Type_System::merge_type(Type *destination, Type *source)
{
    destination->size = source->size;
    destination->kind = source->kind;
    destination->mods |= source->mods;
    destination->meta = source->meta;
    return destination;
}

Type *Type_System::orphan_type_push(Type *type)
{
    return orphan_types.emplace_back(type);
}

// Type Type_System::clone_type(Ast &ast, Type *type)
// {
//     Type clone = {};
//     clone.token = type->token;
//     clone.size = type->size;
//     clone.kind = type->kind;
//     clone.mods = type->mods;
//     clone.cvr = type->cvr;
//     clone.storage = type->storage;

//     switch (type->kind) {
//     case Type_Pointer: {
//         Type *pointed_type = new Type{clone_type(ast, type->pointed_type)};
//         clone.pointed_type = orphan_type_push(pointed_type);
//         break;
//     }

//     case Type_Struct:
//     case Type_Union: {
//         Struct_Statement *struct_statement = ast.push(new Struct_Statement{});
//         struct_statement->keyword = type->struct_statement->keyword;
//         struct_statement->hash = type->struct_statement->hash;

//         for (auto &[name, variable] : type->struct_statement->members) {
//             struct_statement->members[name] = ast.push(new Variable{});
//             // struct_statement->members[name]->type = clone_type(ast, &variable->type);

//             struct_statement->members[name]->name = variable->name;
//             struct_statement->members[name]->mode = variable->mode;
//             struct_statement->members[name]->meta = variable->meta;
//         }
//         clone.struct_statement = struct_statement;
//         break;
//     }

//     case Type_Enum: {
//         clone.enum_type = type->enum_type;
//         break;
//     }

//     case Type_Array: {
//         clone.array_type = type->array_type;
//         break;
//     }

//     case Type_Function_Pointer: {
//         qcc_assert("Todo! clone_type() for function pointers", 0);
//     }

//     default:
//         break; // no metadata
//     }

//     return clone;
// }

} // namespace qcc
