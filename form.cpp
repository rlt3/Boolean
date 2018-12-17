#include <iostream>
#include "Node.hpp"
#include "Parse.hpp"

void
usage (char *prog)
{
    fprintf(stderr, "Usage: %s <expression>\n", prog);
    exit(1);
}

bool
children_has_type (const Node &N, const std::string &type)
{
    for (auto &child : N.children)
        if (child.type == type)
            return true;
    return false;
}

void
remove_children_of_type (Node &N, const std::string &type)
{
    for (auto it = N.children.begin(); it != N.children.end();) {
        if (it->type == type)
            it = N.children.erase(it++);
        else
            it++;
    }
}

void
reduce_to_type (Node &N, const std::string &type)
{
    N.children.clear();
    N.type = type;
}

/* a => !a; !a => a */
Node
negate_var (Node N)
{
    if (N.type[0] == '!')
        return Node(std::string(1, N.type[1]));
    else
        return Node("!" + N.type);
}

Node
reduce (Node parent)
{
    std::set<Node> reduced_children;

    if (parent.children.size() == 0)
        return parent;

    if (parent.children.size() > 0) {
        for (auto &child : parent.children)
            reduced_children.insert(reduce(child));
        parent.children.clear();
        for (auto &child : reduced_children)
            parent.add_reduction(child);
    }

    /* a0bc => 0 */
    if (parent.type == "*" && children_has_type(parent, "0")) {
        reduce_to_type(parent, "0");
        goto exit;
    }

    /* a1bc => abc */
    if (parent.type == "*" && children_has_type(parent, "1")) {
        remove_children_of_type(parent, "1");
        goto exit;
    }

    /* a+0+b+c => a+b+c */
    if (parent.type == "+" && children_has_type(parent, "0")) {
        remove_children_of_type(parent, "0");
        goto exit;
    }

    /* a+1+b+c => 1 */
    if (parent.type == "+" && children_has_type(parent, "1")) {
        reduce_to_type(parent, "1");
        goto exit;
    }

    /*
     * For some var in values, if the negated var is contained in values, then
     * we reduce. If we are 'ORing' then node becomes '1', if 'ANDing' then
     * node becomes '1'.
     * a+!a+b+c => 1
     * a!abc => 0
     */
    for (auto &child : parent.children) {
        if (child.is_operator())
            continue;
        if (parent.children.count(negate_var(child)) == 0)
            continue;
        if (parent.type == "+")
            reduce_to_type(parent, "1");
        else
            reduce_to_type(parent, "0");
        break;
    }

exit:
    return parent;
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

Node
minimize_sets (Node &N)
{
    /*
     * For each child of N:
     * Apply any unilateral reduction rules to potentially remove itself.
     * If Node is in wanted form, convert it to other form then back again.
     * Otherwise just convert it to wanted form.
     * Finally find the minimum sets using the containment algorithm below.
     *
     * Think carefully about recursive functions where two functions
     * effectively call each other.
     */
    return N;
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
	bool good_form = false;
    std::set<Node> new_children;
    Node Z(expr_type);
    Node Y(clause_type);

    if (tree.children.size() == 0)
        return tree;

    for (auto &child : tree.children)
        new_children.insert(conversion_dfs(child, expr_type, clause_type));
    tree.children.clear();
    for (auto &child : new_children)
        tree.add_reduction(child);

    //int is_dnf, is_cnf;
    //is_cnf = tree.is_cnf();
    //is_dnf = tree.is_dnf();
    //if (is_cnf && is_dnf)
    //    return tree;

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
		return reduce(tree);
	} else {
		distribute_node(Z, Y, tree.children.begin(), tree.children.end());
		minimum_sets(Z);
		return reduce(Z);
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
    std::cout << expr.logical_str() << std::endl;

    //expr = to_cnf(expr);
    //expr = to_dnf(expr);

    /*
     * The following factors
     * (cdfk!nrs)+(cdfkrsw)+(dfk!nrsv)+(dfkrsvw)
     * into:
     * dfkrs((c!n)+(cw)+(!nv)+(vw))
     * into:
     * dfkrs((c(!n+w))+(v(!n+w)))
     * into:
     * dfkrs(c+v)(!n+w)
     */

    //expr.print_tree();

    return 0;
}
