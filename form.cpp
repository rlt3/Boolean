#include <iostream>
#include "Node.hpp"
#include "Parse.hpp"

void
usage (char *prog)
{
    fprintf(stderr, "Usage: %s <expression>\n", prog);
    exit(1);
}

/*
 * We append values of the child at the iterator to Y. If there is no next
 * child (the iterator is at the end), then we can append Y to Z and start with
 * a new Y.
 */
void
distribute_node (Node &Z,
                 Node &Y,
                 const std::set<Node>::iterator &it,
                 const std::set<Node>::iterator &end)
{
    const Node &child = *it;

    if (!child.is_operator()) {
        Y.add_child(child);
        if (std::next(it) == end) {
            Z.add_reduction(Y);
        } else {
            distribute_node(Z, Y, std::next(it), end);
        }
    }
    else {
        /* 
         * We keep this extra Node on the stack because the last child in this
         * recursive 'for' loop is responsible for changing the final node in
         * the string of nodes. If we didn't overwrite it every loop then all of
         * its previously appended values would accumulate.
         */
        Node N;
        for (auto &grandchild : child.children) {
            N = Y;
            N.add_reduction(grandchild);
            if (std::next(it) == end) {
                Z.add_reduction(N);
            } else {
                distribute_node(Z, N, std::next(it), end);
            }
        }
    }
}

Node distribute_children (Node tree, char parent, char clause);

/*
 * Call the `distribute_children' for each child of the given tree and reassign
 * the tree's children to the set of distributed children.
 */
Node&
map_children (Node &tree, char parent, char clause)
{
    std::set<Node> new_children;
    for (auto &child : tree.children)
        new_children.insert(distribute_children(child, parent, clause));
    tree.children.clear();
    for (auto &child : new_children)
        tree.add_reduction(child);
    return tree;
}

/*
 * Return whether sub is a subset of tree. This is a naive search which
 * basically returns true if sub matches tree one-for-one and then sub runs out
 * of nodes.
 *
 * We can use this to do the filtering of redundant sets. First, we gather the
 * Nodes which are the minimum nodes (seen below). That set is also the set of
 * subs (as in subset matcher) when then are used to further remove redundant
 * sets of expressions.
 *
 * This will require explicit sorting of nodes since this is going to be a
 * one-for-one matcher and not a general, does-this-node-contain-at-any-point
 * some value.
 */
bool
subset_match (Node &tree, Node &sub)
{
    return false;
}

/*
 * We setup Z and Y so they can be used as references.  Z is the cummulative
 * Node where all different values of Y are inserted into. Y is used as an
 * intermediate node that has all the values of each child of the tree
 * iteratively appended to it.
 *
 * This is effectively an algorithm that creates, through the use of recursive
 * function calls, an N-deep 'for loop' for the children of the given tree.
 * Imagine the tree for 'ab+cd+ef', there would be 3 for loops. The string would
 * be a+c+e, then a+c+f, then a+d+e, etc. just like a for-loop works.
 *
 */
Node
distribute_children (Node tree, char parent, char clause)
{
    std::set<Node> filter;
    Node Z(parent);
    Node Y(clause);

    if (tree.children.size() == 0)
        return tree;

    distribute_node(Z, Y, tree.children.begin(), tree.children.end());

    //int min_set = -1;
    //for (auto &child : Z.children) {
    //    int size = child.values().size();
    //    if (min_set == -1 || size < min_set) {
    //        min_set = size;
    //        filter.clear();
    //    }
    //    if (size == min_set) {
    //        filter.insert(child);
    //    }
    //}
    //Z.children = filter;

    if ((parent == '+' && !Z.is_dnf()) || (parent == '*' && !Z.is_cnf()))
        map_children(Z, parent, clause);

    return Z;
}

Node
to_cnf (Node &tree)
{
    if (tree.type == "*")
        return map_children(tree, '*', '+');
    return distribute_children(tree, '*', '+');
}

Node
to_dnf (Node &tree)
{
    if (tree.type == "+")
        return map_children(tree, '+', '*');
    return distribute_children(tree, '+', '*');
}

int
main (int argc, char **argv)
{
    Node expr;

    if (argc != 2)
        usage(argv[0]);

    if (strlen(argv[1]) == 0)
        usage(argv[0]);

    set_input(std::string(argv[1]));

    expr = parse_input();
    expr.print_tree();

    //if (!expr.is_cnf())
    //    expr = to_cnf(expr);
    //else if (!expr.is_dnf())
    //    expr = to_dnf(expr);

    //if (!expr.is_cnf())
    //    expr = to_cnf(expr);

    expr.print_tree();
    std::cout << expr.logical_str() << std::endl;

    return 0;
}
