#include "format.hpp"

namespace qcc::regex
{

Format::Format(std::ostream &stream, std::string_view name, Node *head) : name(name), head(head), stream(stream)
{
    format_graph();
}

void Format::format_graph()
{
    format("strict digraph {{\n");

    if (head != NULL) {
        format(R"(rankdir=LR;bgcolor="#F9F9F9";compound=true{})", '\n');
        format(R"({:?} [shape="none"]{})", name, '\n');
        format(R"({:?} -> "{}" [label="{}"]{})", name, fmt::ptr(head), head->state, '\n');

        for (Node *node : head->members())
            format_node(node);
    }

    format("}}\n");
}

void Format::format_node(Node *node)
{
    switch (node->state.option) {
    case Regex_Not:
        return format_subgraph(node, R"(style=filled;bgcolor="#FBF3F3")");
    case Regex_Dash:
        return format_subgraph(node, R"(style=filled;bgcolor="#F4FDFF")");

    default:
        format_def(node);
        for (Node *edge : node->edges)
            format_cxn(node, edge);
    }
}

void Format::format_def(Node *node)
{
    auto shape = node->has_edges() ? "square" : "circle";
    format(R"("{}" [shape="{}", label="{}"]{})", fmt::ptr(node), shape, node->id, '\n');
}

void Format::format_cxn(Node *node, Node *edge)
{
    format(R"("{}" -> "{}" [label="{}"]{})", fmt::ptr(node), fmt::ptr(edge), edge->state, '\n');
}

void Format::format_subgraph(Node *node, std::string_view style)
{
    Node *sequence = node->state.sequence;

    format("subgraph cluster_{} {{\n", fmt::ptr(node));
    format("{}", style);
    format_def(node);
    format_cxn(node, sequence);

    for (Node *member : sequence->members()) {
        format_node(member);
    }
    format("}}\n");

    Node *end = sequence->end();
    for (Node *edge : node->edges) {
        format_cxn(end, edge);
    }
}

} // namespace qcc::regex
