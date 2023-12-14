#ifndef QCC_REGEX_FORMAT_HPP
#define QCC_REGEX_FORMAT_HPP

#include "escape_sequence.hpp"
#include "node.hpp"
#include "state.hpp"
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace qcc::regex
{

struct Format
{
    std::ostream &stream;
    std::string_view name;
    Node *head;

    Format(std::ostream &stream, std::string_view name, Node *head);

    void format(std::string_view fmt, auto &&...args)
    {
        fmt::print(stream, fmt::runtime(fmt), args...);
    }

    void format_graph();
    void format_node(Node *node);
    void format_def(Node *node);
    void format_cxn(Node *node, Node *edge);
    void format_subgraph(Node *node, std::string_view style);
};

} // namespace qcc::regex

namespace fmt
{
using namespace qcc;
using namespace qcc::regex;

template <>
struct formatter<State>
{
    constexpr auto parse(format_parse_context &context)
    {
        return context.begin();
    }

    std::string esc(std::string_view s) {
	return escape_string(s, 2);
    }

    constexpr auto format(const State &state, auto &context)
    {
        switch (state.option) {
        case Regex_Eps:
            return format_to(context.out(), "&Sigma;");

        case Regex_Any:
            return format_to(context.out(), "&alpha;");

        case Regex_None:
            return format_to(context.out(), "&times;");

        case Regex_Not:
            return format_to(context.out(), "!");

        case Regex_Dash:
            return format_to(context.out(), "/");

        case Regex_Str:
            return format_to(context.out(), "'{}'", esc(state.str));

        case Regex_Set: {
            switch (state.str.size()) {
            case 0:
                return format_to(context.out(), "[]");

            case 1:
                return format_to(context.out(), "[{}]", esc(state.str));

            default: {
                std::string_view a = state.str.substr(0, 1);
                std::string_view b = state.str.substr(state.str.size() - 1, 1);
                return format_to(context.out(), "[{}..{}]", esc(a), esc(b));
            }
            }
        }

        case Regex_Scope: {
            std::string_view a{&state.range[0], 1};
            std::string_view b{&state.range[1], 1};
            return format_to(context.out(), "[{}..{}]", esc(a), esc(b));
        }

        default:
            return context.out();
        }
    }
};

} // namespace fmt

#endif
