#ifndef QCC_SOURCE_HPP
#define QCC_SOURCE_HPP

#include "fwd.hpp"

namespace qcc
{

enum Source_Location : uint32
{
    Source_None = 0,
    Source_Stack = Bit(uint32, 1),
    Source_Member = Bit(uint32, 2),
    Source_Gpr = Bit(uint32, 3),
    Source_Fpr = Bit(uint32, 4),
    Source_Data = Bit(uint32, 4),
};

struct Source
{
    Source_Location location;

    union {
        int64 gpr;         // Source_Gpr
        int64 fpr;         // Source_Fpr
        int64 address;     // Source_Stack
        int64 data_offset; // Source_Data
    };
};

} // namespace qcc

#endif
