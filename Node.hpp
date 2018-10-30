#ifndef NODE_HPP
#define NODE_HPP

#include <vector>
#include <string>
#include <set>

struct Node {
    std::string type;
    std::set<Node> children;
    std::string logical;

    Node () 
        : type("+")
    { }

    Node (const char c)
        : type(std::string(1, c))
    {
        this->logical = this->logical_str();
    }

    Node (std::string type) 
        : type(type)
    {
        this->logical = this->logical_str();
    }

    Node (const Node &other) 
        : type(other.type)
        , children(other.children)
        , logical(other.logical)
    { }

    std::set<std::string>
    values () const
    {
        std::set<std::string> S;

        for (auto &child : this->children) {
            if (!child.is_operator()) {
                S.insert(child.type);
            } else {
                std::set<std::string> vals = child.values();
                for (auto &v : vals)
                    S.insert(v);
            }
        }
        return S;
    }

    bool
    contains (const std::string type) const
    {
        if (this->type == type)
            return true;
        for (auto &child : this->children)
            if (child.type == type)
                return true;
        for (auto &child : this->children)
            if (child.contains(type))
                return true;
        return false;
    }

    bool
    is_cnf () const
    {
        for (auto &child : this->children)
            if (child.contains("*"))
                return false;
        return true;
    }

    bool
    is_dnf () const
    {
        for (auto &child : this->children)
            if (child.contains("+"))
                return false;
        return true;
    }


    bool
    is_operator () const
    {
        return (type == "+" || type == "*" || type == "!");
    }

    Node&
    operator= (const Node &other)
    {
        this->type = other.type;
        this->children = other.children;
        this->logical = other.logical;
        return *this;
    }

    /*
     * Do a string comparison of each tree's logical string for sets so that
     * ordering will always be deterministic between sets of similar but not
     * equal nodes.
     * This sorts variables by their value first and then by their negation
     * second.
     */
    bool
    operator< (const Node &other) const
    {
        bool a_negated, b_negated;
        std::string A = this->logical;
        std::string B = other.logical;

        /* Put variables before compound expressions */
        if (!this->is_operator() && other.is_operator())
            return true;
        if (this->is_operator() && !other.is_operator())
            return false;
        /* If two compound expressions, the smaller ones go first */
        if (this->is_operator() && other.is_operator())
            return A < B;

        a_negated = b_negated = false;

        if (A[0] == '!') {
            a_negated = true;
            A = std::string(1, A[1]);
        }
        if (B[0] == '!') {
            b_negated = true;
            B = std::string(1, B[1]);
        }

        /* If equal, negation is ordered first */
        if (A == B && a_negated && !b_negated)
            return true;
        if (A == B && !a_negated && b_negated)
            return false;
        /* this makes sure equal variables are considered equal */
        if (A == B && a_negated && b_negated)
            return false;

        /* in all other cases sort by variable name */
        return A < B;
    }

    bool
    operator!= (const Node &other) const
    {
        return !(*this == other);
    }

    bool
    operator== (const Node &other) const
    {
        return this->logical == other.logical;
    }

    /* 
     * If the given child has just a single operand (a single child or a single
     * value), then that child can be added to the parent directly. Never do
     * this for a negated expression because all negated expressions have a 
     * single child.
     * If the given child is the same type as the parent, then we can directly
     * add all its children and values to the parent.
     */
    void
    add_reduction (Node child)
    {
        if (!child.is_operator() || child.type == "!") {
            this->add_child(child);
        } else if (child.type == this->type) {
            for (auto &grandchild : child.children)
                this->add_reduction(grandchild);
        } else if (child.children.size() == 1) {
            this->add_reduction(*child.children.begin());
        } else {
            this->add_child(child);
        }
    }

    void
    add_child (Node child)
    {
        this->children.insert(child);
        this->logical = this->logical_str();
    }

    void
    print_tree () const
    {
        static int tab = 0;
        const Node &N = *this;

        for (int i = 0; i < tab; i++)
            printf("   ");

        printf("%s\n", N.type.c_str());

        tab++;
        for (auto c : N.children)
            c.print_tree();
        tab--;

        if (tab == 0)
            printf("-------------------------------\n");
    }

    std::string
    logical_str () const
    {
        return logical_str(false);
    }

    std::string
    logical_str (const bool print_prod) const
    {
        std::string str;
        const Node &N = *this;

        if (!this->is_operator()) {
            return N.type;
        }

        /* negation just adds the obvious expression negation */
        if (N.type == "!") {
            return str + "!(" + N.children.begin()->logical_str(print_prod) + ")";
        }

        for (auto &child : N.children) {
            if (child.is_operator())
                str += "(";
            str += child.logical_str(print_prod);
            if (child.is_operator())
                str += ")";
            if (N.type == "+" && child != *std::prev(N.children.end()))
                str += N.type;
            if (print_prod && N.type == "*" && child != *std::prev(N.children.end()))
                str += N.type;
        }

        return str;
    }
};

#endif
