#include "precedence.hpp"
#include "expression.hpp"

namespace qcc
{

int32 binary_operator_precedence(Token_Type op)
{
    switch (op) {
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
    case Token_Bitwise_And:
        return 8;
    case Token_Bitwise_Xor:
        return 9;
    case Token_Bitwise_Or:
        return 10;
    case Token_And:
        return 11;
    case Token_Or:
        return 12;
    case Token_Assign:
        return 14;
    default:
        qcc_assert(0, "operator not defined");
        return -1;
    }
}

int32 unary_operator_precedence(Token_Type op, Expression_Order order)
{
    switch (op) {
    case Token_Increment:
    case Token_Decrement:
        if (order == Expression_Rhs)
            return 1;
        if (order == Expression_Lhs)
            return 2;
    case Token_Add:
    case Token_Sub:
    case Token_Not:
    case Token_Bitwise_Not:
    case Token_Deref:
    case Token_Address:
    case Token_Sizeof:
        return 2;
    default:
        qcc_assert(0, "operator not defined");
        return -1;
    }
}

} // namespace qcc
