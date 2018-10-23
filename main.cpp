#include "Node.hpp"
#include "Parse.hpp"

void
usage (char *prog)
{
    fprintf(stderr, "Usage: %s <expression>\n", prog);
    exit(1);
}

/*
 * Negate the children of the given node.
 */
void
negate_children (Node &N)
{
    std::vector<Node> new_children;
    for (auto &child : N.children) {
        Node neg('!');
        neg.add_child(child);
        new_children.push_back(neg);
    }
    N.children = new_children;
}

/*
 * Negate the values of the given node.
 */
void
negate_values (Node &N)
{
    std::set<std::string> new_values;
    for (auto &val : N.values) {
        /* if the value begins with a '!', then remove it, e.g. !!a => a */
        if (val[0] == '!') {
            new_values.insert(std::string(1, val[1]));
        }
        /* otherwise add '!' to the expression, e.g. !a */
        else {
            std::string nval = "!";
            nval += val[0];
            new_values.insert(nval);
        }
    }
    N.values = new_values;
}

/*
 * Apply any negation found in the given expression tree.
 */
Node
negate (Node &parent)
{
    std::vector<Node> negated_children;

    /* if the node isn't a negation, recurse down the tree */
    if (parent.type != '!')
        goto recurse;

    if (parent.children.size() == 1) {
        Node &child = parent.children[0];

        /* 
        * Since the negation of a negation is effectively canceling-out any
        * real work, we can skip ahead in the tree here to the child of the
        * second negation.
        */
        if (child.type == '!') {
            if (child.children.size() == 1) {
                return negate(child.children[0]);
            } else {
                child.type = '*';
                return child;
            }
        }

        switch (child.type) {
          case '+': child.type = '*'; break;
          case '*': child.type = '+'; break;
        }

        negate_children(child);
        negate_values(child);
        parent = child;
    }
    /* there is no child to negate but there is a single child */
    else {
        parent.type = '*';
        negate_values(parent);
    }

recurse:
    if (parent.children.size() == 0)
        return parent;

    for (auto &child : parent.children)
        negated_children.push_back(negate(child));
    /* 
     * We do this so we can cleanly add by a reduction. And we add by reduction
     * because the parsing allows negated nodes to have a single value in place
     * of a child.
     */
    parent.children.clear();
    for (auto &child : negated_children)
        parent.add_reduction(child);

    return parent;
}

int
main (int argc, char **argv)
{
    if (argc != 2)
        usage(argv[0]);

    if (strlen(argv[1]) == 0)
        usage(argv[0]);

    set_input(std::string(argv[1]));

    Node expr = parse_input();
    expr.print_tree();

    expr = negate(expr);
    expr.print_tree();

    std::cout << expr.logical_str() << std::endl;

    return 0;
}
