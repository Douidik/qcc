#ifndef QCC_SOURCE_HPP
#define QCC_SOURCE_HPP

#include "fwd.hpp"

namespace qcc
{

enum Source_Location : uint32
{
    Source_None = 0,
    Source_Stack = Bit(uint32, 1),
    Source_Gpr = Bit(uint32, 3),
    Source_Fpr = Bit(uint32, 4),
    Source_Data = Bit(uint32, 5),
};

struct Source
{
    Source_Location location;
    int64 offset; // Stack-offset / Indirect-register-addressing-offset
    bool has_indirection;

    union {
        int64 gpr;         // Source_Gpr
        int64 fpr;         // Source_Fpr
        int64 address;     // Source_Stack
        int64 data_offset; // Source_Data
        int64 n;
    };

    Source with_offset(int64 x) const
    {
        qcc_assert(location & (Source_Stack | Source_Data) or has_indirection, "cannot offset source");

        Source s = *this;
        s.offset = offset + x;
        return s;
    }
};

struct Register : Source
{
    // Index register names by size
    union {
        struct
        {
            const char *_0;
            const char *byte;
            const char *word;
            const char *_3;
            const char *dword;
            const char *_5;
            const char *_6;
            const char *_7;
            const char *qword;
        };
        const char *name[11];
    };

    std::string_view operator[](size_t n) const
    {
        return name[n];
    }

    Register(int64 index, const char *qword, const char *dword, const char *word, const char *byte) :
        Source{Source_Gpr, 0, false, index}, name{"0?", byte, word, "3?", dword, "5?", "6?", "7?", qword}
    {
    }

    Register with_indirection() const
    {
        Register r = *this;
        r.has_indirection = true;
        return r;
    }
};

} // namespace qcc

#endif
