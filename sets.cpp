#include <cstdio>
#include <string>
#include <set>
#include <algorithm>

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
            return this->children.size() < other.children.size();
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
    intersect (Node &other)
    {
        std::set<Node> S;
        set_intersection(this->children.begin(), this->children.end(),
                         other.children.begin(), other.children.end(),
                         std::inserter(S, S.begin()));
        return S;
    }

    bool
    has_intersection (Node &other)
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
    contains (Node &other)
    {
        return (std::includes(this->children.begin(), this->children.end(),
                              other.children.begin(), other.children.end()));
    }
};

int
main ()
{
    Node Sub(AND);
    Sub.add_var('x');
    Sub.add_var('y');

    Node SubSub(OR);
    SubSub.add_var('w');
    SubSub.add_var('z');
    Sub.add_sub(SubSub);

    Node A(OR);
    A.add_var('a');
    A.add_var('b');
    A.add_sub(Sub);

    Node B(OR);
    B.add_var('d');
    B.add_var('e');
    B.add_sub(Sub);

    A.print();
    B.print();

    if (A.has_intersection(B)) {
        printf("has intersection: ");
        Node I;
        I.children = A.intersect(B);
        I.type = A.type;
        I.print();
    }

    Node Container;
    Container.add_sub(Sub);
    if (A.contains(Container)) {
        printf("A contains Sub!\n");
    }
    
    return 0;
}
