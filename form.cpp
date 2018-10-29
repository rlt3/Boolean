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
 * The cummulative Node is Z. The current node we are appending to is Y.
 * The idea of this is to have a for-loop over each grandchild where the number
 * of children is unknown. So, we Y accumulates one grandchild of each child
 * at a time until there are no more children left to add. If there are no more
 * children then Y is added to Z.
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

Node
distribute_children (char parent, char clause, Node tree)
{
    Node Z(parent);
    Node Y(clause);
    distribute_node(Z, Y, tree.children.begin(), tree.children.end());

    if ((parent == '+' && !Z.is_dnf()) ||
        (parent == '*' && !Z.is_cnf())) {
        std::set<Node> new_children;
        for (auto &child : Z.children)
            new_children.insert(distribute_children(parent, clause, child));
        Z.children.clear();
        for (auto &child : new_children)
            Z.add_reduction(child);
    }

    return Z;
}

Node
to_cnf (Node tree)
{
    return distribute_children('*', '+', tree);
}

Node
to_dnf (Node tree)
{
    return distribute_children('+', '*', tree);
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

    if (!expr.is_cnf())
        expr = to_cnf(expr);
    else if (!expr.is_dnf())
        expr = to_dnf(expr);

    expr.print_tree();
    std::cout << expr.logical_str() << std::endl;

    return 0;
}
