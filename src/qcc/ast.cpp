#include "ast.hpp"
#include "expression.hpp"
#include "object.hpp"
#include "statement.hpp"
#include <fmt/ostream.h>

namespace qcc
{

Ast::~Ast()
{
    for (Statement *statement : statements)
        delete statement;
    for (Expression *expression : expressions)
        delete expression;
    for (Object *object : objects)
        delete object;
}

Object *Ast::decode_designated_expression(Expression *expression)
{
    switch (expression->kind()) {
    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)expression;
        return id_expression->object;
    }

    case Expression_Ref: {
        Ref_Expression *ref_expression = (Ref_Expression *)expression;
        return ref_expression->object;
    }

    case Expression_Address: {
        Address_Expression *address_expression = (Address_Expression *)expression;
        return address_expression->object;
    }

    case Expression_Deref: {
        Deref_Expression *deref_expression = (Deref_Expression *)expression;
        return decode_designated_expression(deref_expression->operand);
    }

    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        return dot_expression->member;
    }

    case Expression_Unary: {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;
        if (unary_expression->operation.type & (Token_Increment | Token_Decrement))
            return decode_designated_expression(unary_expression->operand);
    }

    case Expression_Assign: {
        Assign_Expression *assign_expression = (Assign_Expression *)expression;
        return decode_designated_expression(assign_expression->lhs);
    }

    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        return decode_designated_expression(nested_expression->operand);
    }

    default:
        qcc_assert(0, "expression is not designated");
        return NULL;
    }
}

#define Ws fmt::print(stream, "{:{}}", "", indent * 4)

void Ast::dump_statement(std::ostream &stream, Statement *statement, int32 indent)
{
    switch (statement->kind()) {
    case Statement_Scope: {
        Scope_Statement *scope_statement = (Scope_Statement *)statement;
        Ws, fmt::println(stream, "Scope_Statement: ");

        Ws, fmt::println(stream, "*Objects: ");
        for (Object *object : views::values(scope_statement->objects))
            dump_object(stream, object, indent + 1);

        Ws, fmt::println(stream, "*Records: ");
        for (Record *record : views::values(scope_statement->records))
            dump_object(stream, record, indent + 1);

        Ws, fmt::println(stream, "*Scope: ");
        for (Statement *in_statement : scope_statement->body) {
            Ws, fmt::println(stream, "~");
            dump_statement(stream, in_statement, indent + 1);
        }

        return;
    }
    case Statement_Struct: {
        Struct_Statement *struct_statement = (Struct_Statement *)statement;
        Ws, fmt::println(stream, "Struct_Statement (keyword: {}): ", struct_statement->keyword.str);

        Ws, fmt::println(stream, "*Members: ");
        for (Variable *member : views::values(struct_statement->members)) {
            dump_object(stream, member, indent);
        }
        return;
    }

    case Statement_Function: {
        Function_Statement *function_statement = (Function_Statement *)statement;
        Ws, fmt::println(stream, "Function_Statement:");

        Ws, fmt::println(stream, "*Function: ");
        dump_object(stream, function_statement->function, indent + 1);

        Ws, fmt::println(stream, "*Scope: ");
        dump_statement(stream, function_statement->scope, indent + 1);
        return;
    }

    case Statement_Define: {
        Define_Statement *define_statement = (Define_Statement *)statement;
        Ws, fmt::println(stream, "Define_Statement:");

        Ws, fmt::println(stream, "*Variable:");
        dump_object(stream, define_statement->variable, indent + 1);
        if (define_statement->expression != NULL) {
            Ws, fmt::println(stream, "*Expression:");
            dump_expression(stream, define_statement->expression, indent + 1);
        }
        if (define_statement->next != NULL) {
            Ws, fmt::println(stream, "*Next:");
            dump_statement(stream, define_statement->next, indent + 1);
        }
        return;
    }

    case Statement_Expression: {
        Expression_Statement *expression_statement = (Expression_Statement *)statement;
        dump_expression(stream, expression_statement->expression, indent);
        return;
    }

    case Statement_Condition: {
        Condition_Statement *condition_statement = (Condition_Statement *)statement;

        Ws, fmt::println(stream, "*Boolean:");
        dump_expression(stream, condition_statement->boolean, indent + 1);
        Ws, fmt::println(stream, "*Statement_If:");
        dump_statement(stream, condition_statement->statement_if, indent + 1);

        if (condition_statement->statement_else != NULL) {
            Ws, fmt::println(stream, "*Statement_Else:");
            dump_statement(stream, condition_statement->statement_else, indent + 1);
        }
        return;
    }

    case Statement_While: {
        While_Statement *while_statement = (While_Statement *)statement;

        Ws, fmt::println(stream, "*Boolean:");
        dump_expression(stream, while_statement->boolean, indent + 1);
        Ws, fmt::println(stream, "*Statement:");
        dump_statement(stream, while_statement->statement, indent + 1);
        return;
    }

    case Statement_For: {
        For_Statement *for_statement = (For_Statement *)statement;

        Ws, fmt::println(stream, "*Init:");
        dump_expression(stream, for_statement->init, indent + 1);
        Ws, fmt::println(stream, "*Boolean:");
        dump_expression(stream, for_statement->boolean, indent + 1);
        Ws, fmt::println(stream, "*Loop:");
        dump_expression(stream, for_statement->loop, indent + 1);
        Ws, fmt::println(stream, "*Statement:");
        dump_statement(stream, for_statement->statement, indent + 1);
        return;
    }

    case Statement_Record: {
        return;
    }

    case Statement_Return: {
        Return_Statement *return_statement = (Return_Statement *)statement;
        Function *function = return_statement->function;
        Ws, fmt::println(stream, "Return_Statement (function: '{}'):", function->name.str);

        Ws, fmt::println(stream, "*Expression:");
        dump_expression(stream, return_statement->expression, indent + 1);
        return;
    }

    default:
        qcc_todo("support statement kind");
    }
}

void Ast::dump_expression(std::ostream &stream, Expression *expression, int32 indent)
{
    switch (expression->kind()) {
    case Expression_Unary: {
        Unary_Expression *unary_expression = (Unary_Expression *)expression;
        Ws, fmt::println(stream, "Unary_Expression (operation: {}): ", unary_expression->operation.str);
        dump_expression(stream, unary_expression->operand, indent + 1);
        return;
    }

    case Expression_Binary: {
        Binary_Expression *binary_expression = (Binary_Expression *)expression;
        Ws, fmt::println(stream, "Binary_Expression (operation: {}): ", binary_expression->operation.str);

        Ws, fmt::println(stream, "*Lhs:");
        dump_expression(stream, binary_expression->lhs, indent + 1);
        Ws, fmt::println(stream, "*Rhs:");
        dump_expression(stream, binary_expression->rhs, indent + 1);
        return;
    }

    case Expression_Argument: {
        Argument_Expression *argument_expression = (Argument_Expression *)expression;
        dump_expression(stream, argument_expression->assign_expression, indent);
        if (argument_expression->next != NULL)
            dump_expression(stream, argument_expression->next, indent);
        return;
    }

    case Expression_Invoke: {
        Invoke_Expression *invoke_expression = (Invoke_Expression *)expression;
        Function *function = invoke_expression->function;
        Ws, fmt::println(stream, "Invoke_Expression (function: {}, use_time: {}): ", function->name.str,
                         invoke_expression->use_time);

        if (invoke_expression->arguments != NULL) {
            Ws, fmt::println(stream, "*Arguments:");
            dump_expression(stream, invoke_expression->arguments, indent + 1);
        }
        return;
    }

    case Expression_Comma: {
        Comma_Expression *comma_expression = (Comma_Expression *)expression;
        Ws, fmt::println(stream, "Comma_Expression: ");
        Ws, fmt::println(stream, "*Expression:");
        dump_expression(stream, comma_expression->expression, indent + 1);
        if (comma_expression->next != NULL) {
            Ws, fmt::println(stream, "*Next:");
            dump_expression(stream, comma_expression->next, indent + 1);
        }
        return;
    }

    case Expression_Int: {
        Int_Expression *int_expression = (Int_Expression *)expression;
        Ws, fmt::print(stream, "Int_Expression (");
        fmt::print(stream, "type: {}, ", int_expression->type.name());
        fmt::print(stream, "flags: {}, ", int_expression->flags);
        if (int_expression->type.mods & (Type_Unsigned))
            fmt::print(stream, "value: {}", (uint64)int_expression->value);
        else
            fmt::print(stream, "value: {}", (int64)int_expression->value);
        fmt::println(stream, "): ");
        return;
    }

    case Expression_Float: {
        Float_Expression *float_expression = (Float_Expression *)expression;
        Ws, fmt::print(stream, "Float_Expression (");
        fmt::print(stream, "type: {}, ", float_expression->type.name());
        fmt::print(stream, "value: {}", float_expression->value);
        fmt::println(stream, "): ");
        return;
    }

    case Expression_Id: {
        Id_Expression *id_expression = (Id_Expression *)expression;
        Ws, fmt::println(stream, "Id_Expression (name: {}): ", id_expression->str());
        dump_object(stream, id_expression->object, indent + 1);
        return;
    }

    case Expression_Nested: {
        Nested_Expression *nested_expression = (Nested_Expression *)expression;
        Ws, fmt::println(stream, "Nested_Expression: ");
        dump_expression(stream, nested_expression->operand, indent + 1);
        return;
    }

    case Expression_Assign: {
        Assign_Expression *assign_expression = (Assign_Expression *)expression;
        Ws, fmt::println(stream, "Assign_Expression: ");
        Ws, fmt::println(stream, "*Lhs: ");
        dump_expression(stream, assign_expression->lhs, indent + 1);
        Ws, fmt::println(stream, "*Rhs: ");
        dump_expression(stream, assign_expression->rhs, indent + 1);
        return;
    }

    case Expression_Cast: {
        Cast_Expression *cast_expression = (Cast_Expression *)expression;
        Ws, fmt::print(stream, "Cast_Expression (");
        fmt::print(stream, "from: {}, ", cast_expression->from->name());
        fmt::print(stream, "into: {}, ", cast_expression->into->name());
        fmt::println(stream, "): ");

        fmt::println(stream, "*Expression: ");
        dump_expression(stream, cast_expression->expression, indent + 1);
        return;
    }

    case Expression_Dot: {
        Dot_Expression *dot_expression = (Dot_Expression *)expression;
        Ws, fmt::println(stream, "Dot_Expression: ");

        Ws, fmt::println(stream, "*Expression: ");
        dump_expression(stream, dot_expression->expression, indent + 1);
        Ws, fmt::println(stream, "*Member: ");
        dump_object(stream, dot_expression->member, indent + 1);
        return;
    }

    case Expression_Deref: {
        Deref_Expression *deref_expression = (Deref_Expression *)expression;
        Ws, fmt::println(stream, "Deref_Expression (type: {}): ", deref_expression->type->name());
        Ws, fmt::println(stream, "*Expression");
        dump_expression(stream, deref_expression->operand, indent + 1);
        return;
    }

    case Expression_Address: {
        Address_Expression *address_expression = (Address_Expression *)expression;
        Ws, fmt::println(stream, "Address_Expression (type: {}): ", address_expression->type.name());
        Ws, fmt::println(stream, "*Object");
        dump_object(stream, address_expression->object, indent + 1);
        return;
    }

    case Expression_Ref: {
        Ref_Expression *ref_expression = (Ref_Expression *)expression;
        Ws, fmt::println(stream, "Ref_Expression (type: {}): ", ref_expression->type->name());
        Ws, fmt::println(stream, "*Object");
        dump_object(stream, ref_expression->object, indent + 1);
        return;
    }

    default:
        qcc_todo("support expression kind");
    }
}

void Ast::dump_object(std::ostream &stream, Object *object, int32 indent)
{
    switch (object->kind()) {
    case Object_Function: {
        Function *function = (Function *)object;
        Ws, fmt::print(stream, "Function (");
        fmt::print(stream, "return_type: {}, ", function->return_type.name());
        fmt::print(stream, "name: {}, ", function->name.str);
        fmt::print(stream, "stack_size: {}", function->stack_size);
        fmt::println(stream, "): ");

        if (function->parameters != NULL) {
            Ws, fmt::println(stream, "*Parameters: ");
            dump_statement(stream, function->parameters, indent + 1);
        }
        return;
    }

    case Object_Variable: {
        Variable *variable = (Variable *)object;
        Ws, fmt::print(stream, "Variable (");
        fmt::print(stream, "name: {}, ", variable->name.str);
        fmt::print(stream, "type: {}, ", variable->type.name());
        fmt::print(stream, "define_mode: {}, ", define_mode_str(variable->env));
        fmt::print(stream, "meta: {}, ", variable->meta);

        switch (variable->location) {
        case Source_Stack:
            fmt::print(stream, "stack: {}", variable->address);
            break;
        case Source_Data:
            fmt::print(stream, "data: {}", variable->address);
            break;
        case Source_Gpr:
            fmt::print(stream, "gpr: {}", variable->gpr);
            break;
        case Source_Fpr:
            fmt::print(stream, "fpr: {}", variable->fpr);
            break;
        default:
            fmt::print(stream, "not_allocated!", variable->fpr);
            break;
        }
        fmt::println(stream, ")");
        return;
    }

        // case Object_Typedef: {
        // 	return;
        // }

    case Object_Record: {
        Record *record = (Record *)object;
        Ws, fmt::println(stream, "Record (type: {})", record->type.name());
        return;
    }

    default:
        qcc_todo("support object kind");
    }
}

} // namespace qcc
