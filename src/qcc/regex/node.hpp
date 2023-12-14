#ifndef QCC_REGEX_NODE_HPP
#define QCC_REGEX_NODE_HPP

#include "arena.hpp"
#include "state.hpp"
#include <set>

namespace qcc::regex
{

struct Node_Cmp
{
    bool operator()(const Node *a, const Node *b) const;
};

using Node_Set = std::set<Node *, Node_Cmp>;
using Node_Arena = Arena<Node, 256>;

struct Node
{
    State state;
    Node_Set edges;
    int id;

    Node(State state);
    Node(Option option = Regex_Monostate);
    size_t submit(std::string_view expr, size_t n) const;
    
    Node *push(Node *node);
    Node *merge(Node *node);
    Node *concat(Node *node);
    Node *end();
    Node *max_edge() const;
    bool has_edges() const;

    void map_sequence_ids(int base);
    Node_Set &make_members(Node_Set &set);
    Node_Set members();
};

} // namespace qcc::regex

#endif
