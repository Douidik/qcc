#ifndef QCC_OPERATORS_HPP
#define QCC_OPERATORS_HPP

#include "fwd.hpp"
#include "scan/token.hpp"

namespace qcc
{

const int32 Lowest_Precedence = 15;

int32 binary_operator_precedence(Token_Type op);
int32 unary_operator_precedence(Token_Type op, Expression_Order order);

} // namespace qcc

#endif
