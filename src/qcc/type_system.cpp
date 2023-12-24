#include "type_system.hpp"
#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/format.h>
#include <ranges>

namespace qcc
{

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
        for (Variable *member : std::views::values(struct_statement->members)) {
            struct_alignment = std::max(struct_alignment, member->type.alignment());
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
    case Expression_Assign:
        return &((Assign_Expression *)expression)->variable->type;
    case Expression_Invoke:
        return &((Invoke_Expression *)expression)->function->return_type;
    case Expression_Ternary:
        return &((Ternary_Expression *)expression)->type;
    case Expression_Int:
        return &((Int_Expression *)expression)->type;
    case Expression_Float:
        return &((Float_Expression *)expression)->type;
    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)expression;
        return &((Variable *)id_expression->object)->type;
    }
    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        return &dot_expression->member->type;
    }
    case Expression_Comma: {
        Comma_Expression *comma_expression = (Comma_Expression *)expression;
        return expression_type(comma_expression->next);
    }
    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        return expression_type(nested_expression->operand);
    }
    case Expression_Address: {
        Address_Expression *address_expression = (Address_Expression *)expression;
        return &address_expression->type;
    }
    case Expression_Deref: {
        Deref_Expression *deref_expression = (Deref_Expression *)expression;
        return deref_expression->type;
    }

    default:
        qcc_assert("expression_type() does not support expression", 0);
        return NULL;
    }
}

int32 Type_System::expression_precedence(Expression *expression)
{
    if (expression->kind() & Expression_Unary) {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;
        switch (unary_expression->operation.type) {
        case Token_Increment:
        case Token_Decrement:
            if (unary_expression->order == Expression_Rhs)
                return 1;
            if (unary_expression->order == Expression_Lhs)
                return 2;
        case Token_Add:
        case Token_Sub:
        case Token_Not:
        case Token_Bin_Not:
        case Token_Deref:
        case Token_Address:
        case Token_Sizeof:
            return 2;
        default:
            break;
        }
    }

    if (expression->kind() & Expression_Binary) {
        Binary_Expression *binary_expression = (Binary_Expression *)expression;
        switch (binary_expression->operation.type) {
        case Token_Mul:
        case Token_Div:
        case Token_Mod:
            return 3;
        case Token_Add:
        case Token_Sub:
            return 4;
        case Token_Shift_L:
        case Token_Shift_R:
            return 5;
        case Token_Greater:
        case Token_Greater_Eq:
        case Token_Less:
        case Token_Less_Eq:
            return 6;
        case Token_Not_Eq:
        case Token_Eq:
            return 7;
        case Token_Bin_And:
            return 8;
        case Token_Bin_Xor:
            return 9;
        case Token_Bin_Or:
            return 10;
        case Token_And:
            return 11;
        case Token_Or:
            return 12;
        case Token_Assign:
        case Token_Add_Assign:
        case Token_Sub_Assign:
        case Token_Mul_Assign:
        case Token_Div_Assign:
        case Token_Mod_Assign:
        case Token_Shift_L_Assign:
        case Token_Shift_R_Assign:
        case Token_Bin_And_Assign:
        case Token_Bin_Xor_Assign:
        case Token_Bin_Or_Assign:
            return 14;
        default:
            break;
        }
    }

    if (expression->kind() & Expression_Comma) {
        return 15;
    }
    qcc_assert("expression_precedence() not defined", 0);
    return -1;
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
            Type *from_parameter_type = &from_parameter->variable->type;
            Type *into_parameter_type = &into_parameter->variable->type;
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
    qcc_assert("cannot size scalar", 0);
    return -1;
}

size_t Type_System::struct_size(Type *type)
{
    size_t size = 0;

    for (Variable *member : std::views::values(type->struct_statement->members)) {
        if (type->kind & Type_Union)
            size = std::max(size, member->type.size);
        if (type->kind & Type_Struct)
            size += member->type.size;
    }
    return size;
}

Type *Type_System::merge_type(Type *destination, Type *source)
{
    destination->token = source->token;
    destination->size = source->size;
    destination->kind = source->kind;
    destination->mods |= source->mods;
    destination->data = source->data;
    return destination;
}

Type *Type_System::orphan_type_push(Type *type)
{
    return orphan_types.emplace_back(type);
}

Type Type_System::clone_type(Ast &ast, Type *type)
{
    Type clone{};
    clone.token = type->token;
    clone.size = type->size;
    clone.kind = type->kind;
    clone.mods = type->mods;
    clone.cvr = type->cvr;
    clone.storage = type->storage;

    switch (type->kind) {
    case Type_Pointer: {
        Type *pointed_type = new Type{clone_type(ast, type->pointed_type)};
        clone.pointed_type = orphan_type_push(pointed_type);
        break;
    }

    case Type_Struct:
    case Type_Union: {
        Struct_Statement *struct_statement = ast.push(new Struct_Statement{});
        struct_statement->keyword = type->struct_statement->keyword;
        struct_statement->hash = type->struct_statement->hash;

        for (auto &[name, variable] : type->struct_statement->members) {
            struct_statement->members[name] = ast.push(new Variable{});
            struct_statement->members[name]->type = clone_type(ast, &variable->type);
            struct_statement->members[name]->name = variable->name;
            struct_statement->members[name]->mode = variable->mode;
        }
        clone.struct_statement = struct_statement;
        break;
    }

    case Type_Enum: {
        clone.enum_type = type->enum_type;
        break;
    }

    case Type_Array: {
        clone.array_type = type->array_type;
        break;
    }

    case Type_Function_Pointer: {
        qcc_assert("TODO! clone_type() for function pointers", 0);
    }

    default:
        break; // no metadata
    }

    return clone;
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
