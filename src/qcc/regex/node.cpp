#include "node.hpp"

namespace qcc::regex
{

bool Node_Cmp::operator()(const Node *a, const Node *b) const
{
    return a->id < b->id;
}

Node::Node(State state) : state(state), id(0), edges() {}

Node::Node(Option option) : state{.option = option}, id(0), edges() {}

size_t Node::submit(std::string_view expr, size_t n) const
{
    size_t match = state.submit(expr, n);

    if (match != npos) {
        if (!has_edges() and match >= expr.size())
            return match;

        for (const Node *edge : edges) {
            size_t match_fwd = edge->submit(expr, match);
            if (match_fwd != npos)
                return match_fwd;
        }

        if (!has_edges())
            return match;
    }

    return npos;
}

Node *Node::push(Node *node)
{
    node->map_sequence_ids(end()->id + 1);
    return *edges.insert(node).first;
}

Node *Node::merge(Node *node)
{
    node->map_sequence_ids(end()->id + 1);
    return concat(node);
}

Node *Node::concat(Node *node)
{
    for (Node *member : members()) {
        if (!member->has_edges())
            member->edges.insert(node);
    }
    return node;
}

void Node::map_sequence_ids(int base)
{
    for (Node *member : members())
        member->id += base;
}

Node *Node::end()
{
    Node *end = this;

    for (Node *member : members())
        end = end->id > member->id ? end : member;

    return end;
}

Node *Node::max_edge() const
{
    return !edges.empty() ? *edges.rbegin() : NULL;
}

bool Node::has_edges() const
{
    return !edges.empty() and max_edge()->id > id;
}

Node_Set &Node::make_members(Node_Set &set)
{
    set.insert(this);

    for (Node *edge : edges) {
        if (edge->id > id)
            edge->make_members(set);
    }

    return set;
}

Node_Set Node::members()
{
    Node_Set set{};
    return make_members(set);
}

} // namespace qcc::regex
