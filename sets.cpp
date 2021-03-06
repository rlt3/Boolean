#include <cstdio>
#include <string>
#include <set>
#include <algorithm>
#include <cassert>

typedef enum NodeType {
    AND, OR, NOT, VAR, NVAR, TRUE, FALSE
} NodeType;

class Node {
public:
    NodeType type;
    char val;
    std::set<Node> children;

    Node () : type(FALSE), val('\0') { }
    Node (char var, bool negated) : type(negated ? NVAR : VAR), val(var) { }
    Node (NodeType op) : type(op), val('\0') { }
    Node (int constant) : type(constant > 0 ? TRUE : FALSE), val(constant) { }

    bool
    is_operator () const
    {
        return (type == AND || type == OR || type == NOT);
    }

    bool
    operator< (const Node &other) const
    {
        /* Put variables before compound expressions */
        if (!this->is_operator() && other.is_operator())
            return true;
        if (this->is_operator() && !other.is_operator())
            return false;

        /* If two compound expressions */
        if (this->is_operator() && other.is_operator()) {
            /* NOTs come before everything */
            if (this->type == NOT && other.type != NOT)
                return true;
            if (this->type != NOT && other.type == NOT)
                return false;

            /* sort AND's first */
            if (this->type == AND && other.type == OR)
                return true;
            /* then the OR's */
            if (this->type == OR && other.type == AND)
                return false;

            /* and if they're the same, sort by size */
            if (this->children.size() < other.children.size())
                return true;
            if (this->children.size() > other.children.size())
                return false;

            /* if they are the same size, recurse until we find a difference */
            auto it1 = this->children.begin();
            auto it2 = other.children.begin();
            for (; it1 != this->children.end(); it1++, it2++) {
                if (*it1 < *it2)
                    return true;
                else if (*it2 < *it1)
                    return false;
            }

            return false;
        }

        /* past this point, 'this' and 'other' are variable Nodes */

        if (this->val == other.val) {
            /* If equal vars, negation is ordered first */
            if (this->type == NVAR && other.type == VAR)
                return true;
            if (this->type == VAR && other.type == NVAR)
                return false;
            /* this makes sure equal variables are considered equal */
            if (this->type == NVAR && other.type == NVAR)
                return false;
        }

        /* in all other cases sort by variable name */
        return this->val < other.val;
    }

    bool
    operator!= (const Node &other) const
    {
        return !(*this == other);
    }

    bool
    operator== (const Node &other) const
    {
        if (this->type != other.type)
            return false;
        if (this->val != other.val)
            return false;
        if (this->children != other.children)
            return false;
        return true;
    }

    void
    print () const
    {
        printf("%s\n", this->to_str().c_str());
    }

    std::string
    to_str () const
    {
        std::string S;

        if (this->is_operator()) {
            const Node &end = *std::prev(this->children.end());
            S += "(";
            for (const auto &N : this->children) {
                S += N.to_str();
                if (this->type == OR && N != end)
                    S += "+";
            }
            S += ")";
        } else {
            if (this->type == NVAR)
                S += "!";
            S += this->val;
        }
        return S;
    }

    void
    add_var (char var)
    {
        this->children.insert(Node(var, false));
    }

    /* negated var */
    void
    add_nvar (char var)
    {
        this->children.insert(Node(var, true));
    }

    void
    add_sub (Node sub)
    {
        this->children.insert(sub);
    }

    std::set<Node>
    intersect (Node &other) const
    {
        std::set<Node> S;
        set_intersection(this->children.begin(), this->children.end(),
                         other.children.begin(), other.children.end(),
                         std::inserter(S, S.begin()));
        return S;
    }

    bool
    has_intersection (Node &other) const
    {
        return !(this->intersect(other).empty());
    }

    /*
     * This has a weird interaction... since this effectively determines
     * whether or not a Node contains other nodes, specifying whether a Node
     * contains another single Node will *not* work with this method. One would
     * need to add that single Node to a container node (to form a set
     * container) to use this method correctly. Otherwise, the 'other.children'
     * will be empty (or at least contain nodes unexpected).
     */
    bool
    has_subset (Node &other) const
    {
        return (std::includes(this->children.begin(), this->children.end(),
                              other.children.begin(), other.children.end()));
    }

    bool
    contains_type (const NodeType type) const
    {
        if (this->type == type)
            return true;
        for (auto &child : this->children)
            if (child.type == type)
                return true;
        for (auto &child : this->children)
            if (child.contains_type(type))
                return true;
        return false;
    }

    bool
    is_cnf () const
    {
        for (auto &child : this->children)
            if (child.contains_type(AND))
                return false;
        return true;
    }

    bool
    is_dnf () const
    {
        for (auto &child : this->children)
            if (child.contains_type(OR))
                return false;
        return true;
    }

    /*
     * Distribute the children of this Node over its operator.
     */
    Node
    distribute ()
    {
        assert(this->is_operator());
        return Node();
    }
};

/*
 * A + (B * C * ...) => (A+B)*(A+B)*(A+...)*...
 */
Node
convert_to_cnf (Node &A, Node &B)
{
    Node CNF(AND);

    for (auto &C : B.children) {
        Node child(OR);
        child.add_sub(A);
        child.add_sub(C);
        CNF.add_sub(child);
    }

    return CNF;
}

Node
to_cnf (Node &N)
{
    Node ret(AND);

    assert(N.is_operator());

    if (N.is_cnf())
        return N;

    //if (N.children.size() < 2)
    //    return N;

    if (N.type == AND) {
        for (auto C : N.children)
            ret.add_sub(to_cnf(C));
    }
    else {
    }

    return ret;
}

int
main ()
{
    Node c1('z', false);
    Node c2(AND);
    c2.add_var('a');
    c2.add_var('b');

    Node E = convert_to_cnf(c1, c2);
    E.print();

    //Node Tree(OR);

    //Node L(AND);
    //L.add_var('a');
    //L.add_var('b');

    //Node R(AND);
    //R.add_var('x');
    //R.add_var('y');

    //Node RR(OR);
    //RR.add_var('w');
    //RR.add_var('z');

    //R.add_sub(RR);
    //Tree.add_var('c');
    //Tree.add_sub(R);
    //Tree.add_sub(L);

    //Tree.print();
    
    return 0;
}
