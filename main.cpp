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

/*
 * This distributes nodes at a certain depth and only does a single
 * distribution of terms, i.e. it is not a recursive distributor. For example,
 * if `start_depth' is 0, then the root node of the tree given will be
 * distributed. If `start_depth` is 1, then the children of the root node will
 * be distributed, etc.
 */
Node
distribute (Node &parent, const int depth, const int start_depth)
{
    std::vector<Node> dist_children;

    if (depth < start_depth) {
        for (auto &child : parent.children)
            dist_children.push_back(distribute(child, depth + 1, start_depth));
        parent.children.clear();
        for (auto child : dist_children)
            parent.add_reduction(child);
        return parent;
    }

    /* nothing to distribute */
    if (parent.children.size() == 0 || parent.values.size() == 0)
        return parent;

    /* Distribute multiple children so we can reduce to Nodes with one child */
    if (parent.children.size() > 1) {
        /* 
         * Since we are distributing values *over* some operation, we create a
         * new node that represents that operation for each child. That child
         * is added as a child to that node.
         */
        for (auto &child : parent.children) {
            Node N = Node(parent.type);
            N.values = parent.values;
            N.add_child(child);
            dist_children.push_back(N);
        }
    }
    /* 
     * Distributing over a single child is the last operation before a node
     * becomes 'undistributable' which is the goal of the algorithm.
     */
    else {
        Node &child = parent.children[0];
        for (auto &val : child.values) {
            Node N = Node(parent.type);
            N.values = parent.values;
            N.add_value(val);
            dist_children.push_back(N);
        }
    }

    /* parent has no values now because they have been distributed */
    parent.values.clear();

    /* and parent gets a new type and the distribute children */
    switch (parent.type) {
        case '+': parent.type = '*'; break;
        case '*': parent.type = '+'; break;
    }
    parent.children = dist_children;

    return parent;
}

int
main (int argc, char **argv)
{
    Node expr, dist;

    if (argc != 2)
        usage(argv[0]);

    if (strlen(argv[1]) == 0)
        usage(argv[0]);

    set_input(std::string(argv[1]));

    expr = parse_input();
    negate(expr);
    expr.print_tree();

    //for (int depth = 0 ;; depth++) {
    //    /* 
    //     * expr is passed by reference, so it actually updates expr. saving
    //     * expr to dist preserves it so we can compare later.
    //     */
    //    dist = expr;
    //    distribute(expr, 0, depth);
    //    if (expr == dist)
    //        break;
    //    expr.print_tree();
    //}

    distribute(expr, 0, 1);
    expr.print_tree();

    std::cout << expr.logical_str() << std::endl;

    return 0;
}
