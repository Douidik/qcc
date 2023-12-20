#ifndef QCC_ASM_HPP
#define QCC_ASM_HPP

#include "fwd.hpp"
#include "allocator.hpp"
#include <fmt/format.h>
#include <ostream>

namespace qcc
{

enum Label_Type
{
    Label_Return,
    Label_Ternary_Else,
    Label_Ternary_End,
    Label_Continue,
    Label_Break,
    Label_Else,
    Label_If_End,
};

struct Label
{
    Label_Type type;
    uint32 count;
};

struct Asm
{
    Ast &ast;
    Allocator &allocator;
    std::ostream &stream;
    std::string_view source;
    uint32 label_count;

    Asm(Ast &ast, Allocator &allocator, std::string_view source, std::ostream &stream);
    Label make_label(Label_Type type);
    virtual void make() = 0;
};

} // namespace qcc

namespace fmt
{
using namespace qcc;

template <>
struct formatter<Label>
{
    constexpr auto parse(format_parse_context &context)
    {
        return context.begin();
    }

    constexpr std::string_view name(const Label &label) const
    {
        if (label.type == Label_Return)
            return "return";
        if (label.type == Label_Ternary_Else)
            return "else";
        if (label.type == Label_Ternary_End)
            return "ternary_end";
        if (label.type == Label_Continue)
            return "continue";
        if (label.type == Label_Break)
            return "break";
        if (label.type == Label_Else)
            return "else";
        if (label.type == Label_If_End)
            return "if_end";
        return "?";
    }

    template <typename F>
    constexpr auto format(const Label &label, F &context) const
    {
        return fmt::format_to(context.out(), ".L.{}{}", name(label), label.count);
    }
};

} // namespace fmt

#endif
