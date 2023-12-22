#ifndef QCC_ASM_HPP
#define QCC_ASM_HPP

#include "allocator.hpp"
#include "fwd.hpp"
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
        switch (label.type) {
        case Label_Return:
            return "return";
        case Label_Ternary_Else:
            return "ternary_else";
        case Label_Ternary_End:
            return "ternary_end";
        case Label_Continue:
            return "continue";
        case Label_Break:
            return "break";
        case Label_Else:
            return "else";
        case Label_If_End:
            return "if_end";
        default:
            return "?";
        }
    }

    template <typename F>
    constexpr auto format(const Label &label, F &context) const
    {
        return fmt::format_to(context.out(), ".L.{}{}", name(label), label.count);
    }
};

} // namespace fmt

#endif
