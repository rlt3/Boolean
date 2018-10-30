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

/*
 * For any child of N, if that child C can contain another child S then C is
 * redundant and should be filtered. We use this filtering process to find the
 * minimum sets.
 */
void
minimum_sets (Node &N)
{
    std::set<Node> filtered;
    std::set<Node> children;

    children = N.children;

    for (auto &child : children) {
        for (auto &set : children) {
            bool contains = true;
            if (set == child)
                continue;
            /* 
             * if the child doesn't have any children but the set does, the
             * child cannot contain that set.
             */
            if (child.children.size() == 0 && set.children.size() > 0) {
                contains = false;
            }
            /*
             * If the set has no children, then we test if child contains that
             * set itself (because it must be a variable).
             */
            else if (child.children.size() > 0 && set.children.size() == 0) {
                if (!child.children.count(set))
                    contains = false;
            }
            /*
             * Neither set or child will be equal here (because of the guard
             * above). Thus if they both have no children, they must be
             * different.
             */
            else if (child.children.size() == 0 && set.children.size() == 0) {
                contains = false;
            }
            /*
             * Otherwise we test that child can hold each and every child of
             * set.
             */
            else {
                for (auto &set_member : set.children) {
                    if (!child.children.count(set_member)) {
                        contains = false;
                        break;
                    }
                }
            }
            if (contains)
                filtered.insert(child);
        }
    }

    /* add all non-filtered children to N */
    N.children.clear();
    for (auto &child : children) {
        if (!filtered.count(child))
            N.children.insert(child);
    }
}

Node to_cnf (Node &tree);
Node to_dnf (Node &tree);

/*
 * This converts the entire expression tree to CNF form from the leaves up to
 * the root node.
 *
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
conversion_dfs (Node tree, char expr_type, char clause_type)
{
    std::set<Node> new_children;
    Node Z(expr_type);
    Node Y(clause_type);
    int good_form = false;

    if (tree.children.size() == 0)
        return tree;

    for (auto &child : tree.children)
        new_children.insert(conversion_dfs(child, expr_type, clause_type));
    tree.children.clear();
    for (auto &child : new_children)
        tree.add_reduction(child);

    switch (expr_type) {
        case '*': if (tree.is_cnf()) { good_form = true; } break;
        case '+': if (tree.is_dnf()) { good_form = true; } break;
    }

    if (good_form) {
        /*
         * TODO:
         * Could be 'minimize sets' which converts it to the opposite form,
         * does reductions, and other things.
         */
        minimum_sets(tree);
        return tree;
    } else {
        distribute_node(Z, Y, tree.children.begin(), tree.children.end());
        minimum_sets(Z);
        return Z;
    }
}

Node
to_cnf (Node &tree)
{
    return conversion_dfs(tree, '*', '+');
}

Node
to_dnf (Node &tree)
{
    return conversion_dfs(tree, '+', '*');
}

int
main (int argc, char **argv)
{
    Node expr, orig;

    if (argc != 2)
        usage(argv[0]);

    if (strlen(argv[1]) == 0)
        usage(argv[0]);

    set_input(std::string(argv[1]));

    expr = parse_input();
    expr.print_tree();

    expr = to_cnf(expr);

    expr.print_tree();
    std::cout << expr.logical_str() << std::endl;

    return 0;
}
