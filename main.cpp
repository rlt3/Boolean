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
        Node child = parent.children[0];

        /* 
        * Since the negation of a negation is effectively canceling-out any
        * real work, we can skip ahead in the tree here to the child of the
        * second negation.
        */
        if (child.type == '!') {
            /* 
             * assignment to parent here lets us not have to assign return of
             * negate for the entire tree.
             */
            if (child.children.size() == 1) {
                parent = negate(child.children[0]);
            } else {
                child.type = '*';
                parent = child;
            }
        } else {
            switch (child.type) {
              case '+': child.type = '*'; break;
              case '*': child.type = '+'; break;
            }

            negate_children(child);
            negate_values(child);
            parent = child;
        }
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
 * Distribute the given values over the node with the given operation.
 */
Node
distribute_values (Node &parent, std::set<std::string> &values, const char op)
{
    /*
     * Since we are distributing values *over* some operation, we create a
     * new node that represents that operation for each child. That child
     * is added as a child to that node.
     *
     * vals: [a],
     *      node: b+c => ab+ac
     *      node: b+cd+e(f+g) => ab+acd+ae(f+g)
     *
     * append values to values of each child of N
     * append values to values of N itself
     */
    std::vector<Node> dist_children;

    for (auto &val : parent.values) {
        Node N = Node(op);
        N.values = values;
        N.add_value(val);
        dist_children.push_back(N);
    }

    for (auto &child : parent.children) {
        Node N = Node(op);
        N.values = values;
        N.add_reduction(child);
        dist_children.push_back(N);
    }

    parent.values.clear();
    parent.children.clear();
    for (auto &child : dist_children)
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

    /*
     * If there is more than one child, the parent node stays intact and all
     * children gain the distributed values separately.
     */
    if (parent.children.size() > 1) {
        for (auto &child : parent.children)
            distribute_values(child, parent.values, parent.type);
        /* parent has no values now because they have been distributed */
        parent.values.clear();
    }
    /*
     * For a single child, the parent becomes the new distributed child. This
     * is where distributing can yield a different node type, e.g. a(b+c)
     * yields a parent type '+' ab+ac.
     */
    else {
        parent = distribute_values(parent.children[0], parent.values, parent.type);
    }

    return parent;
}

bool
children_has_type (Node &N, const char type)
{
    for (auto &child : N.children)
        if (child.type == type)
            return true;
    return false;
}

void
remove_children_of_type (Node &N, const char type)
{
    for (auto it = N.children.begin(); it != N.children.end();) {
        if (it->type == type)
            it = N.children.erase(it++);
        else
            it++;
    }
}

void
reduce_to_type (Node &N, const char type)
{
    N.values.clear();
    N.children.clear();
    N.type = type;
}

/* a => !a; !a => a */
std::string
negate_var (std::string value)
{
    if (value[0] == '!')
        return std::string(1, value[1]);
    return "!" + value;
}

Node
reduce (Node &parent)
{
    std::vector<Node> reduced_children;

    if (parent.children.size() > 0) {
        for (auto &child : parent.children)
            reduced_children.push_back(reduce(child));
        parent.children.clear();
        for (auto &child : reduced_children)
            parent.add_reduction(child);
    }

    /* a0bc => 0 */
    if (parent.type == '*' && children_has_type(parent, '0')) {
        reduce_to_type(parent, '0');
    }

    /* a1bc => abc */
    if (parent.type == '*' && children_has_type(parent, '1')) {
        remove_children_of_type(parent, '1');
    }

    /* a+0+b+c => a+b+c */
    if (parent.type == '+' && children_has_type(parent, '0')) {
        remove_children_of_type(parent, '0');
    }

    /* a+1+b+c => 1 */
    if (parent.type == '+' && children_has_type(parent, '1')) {
        reduce_to_type(parent, '1');
    }

    /* 
     * For some var in values, if the negated var is contained in values, then
     * we reduce. If we are 'ORing' then node becomes '1', if 'ANDing' then
     * node becomes '1'.
     * a+!a+b+c => 1
     * a!abc => 0
     */
    for (auto &value : parent.values) {
        if (!parent.values.count(negate_var(value)))
            continue;
        if (parent.type == '+')
            reduce_to_type(parent, '1');
        else
            reduce_to_type(parent, '0');
        break;
    }

    if (parent.children.size() == 1 && parent.values.size() == 0)
        parent = parent.children[0];

    return parent;
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

    negate(expr);
    printf("Negated\n");
    expr.print_tree();

    reduce(expr);
    printf("Reduced\n");
    expr.print_tree();

    /*
     * (!b)&&((d||m||q)||((!t&&d))||((!a&&!p&&c&&q)&&(m||!l))||((r&&v)&&((a||v))))
     * !b(d+m+q+(!td)+(!a!pcq(m+!l))+(rv(a+v)))
     * => (!bd)+(!bm)+(!bq)+(!brv)
     *
     * (!b)||(!(((b)||((!s&&o)&&((a||h))))&&((y)||((!q&&c&&h&&q&&z)&&((e||z))&&((q||r))&&(!((h||n)))&&((!t||i))))))
     * !b+(!((b+(!so(a+h)))(y+(!qchqz(e+z)(q+r)(!(h+n))(!t+i)))))
     * => !b+!y
     *
     * At first I didn't recurse all the way down the tree to distribute because
     * it would be an infinite recursion. Now, I have methods to stop that recursion
     * because we only distribute when they are children to distribute with.
     * The two expression above represent the flaw with the current method of
     * distribution which doesn't work really at all.
     * Negate the expression, distribute all terms to their depth then reduce.
     */

    int max_depth;
    int depth;
    depth = 0;
    max_depth = expr.depth() + 1;
    while (depth < max_depth) {
        distribute(expr, 0, depth);
        printf("Dist Depth %d\n", depth);
        expr.print_tree();
        orig = expr;
        expr = reduce(expr);
        if (orig != expr) {
            depth = 0;
            max_depth = expr.depth() + 1;
        } else {
            depth++;
        }
    }

    std::cout << expr.logical_str() << std::endl;
    std::cout << expr.string() << std::endl;

    return 0;
}
