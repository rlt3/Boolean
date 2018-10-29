#ifndef NODE_HPP
#define NODE_HPP

#include <vector>
#include <string>
#include <set>

struct Node {
    std::string type;
    std::set<Node> children;

    Node () 
        : type("+")
    { }

    Node (const char c)
        : type(std::string(1, c))
    { }

    Node (std::string type) 
        : type(type)
    { }

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

    /*
     * Do a string comparison of each tree's logical string for sets so that
     * ordering will always be deterministic between sets of similar but not
     * equal nodes.
     */
    bool
    operator< (const Node &other) const
    {
        return this->logical_str() < other.logical_str();
    }

    bool
    operator!= (const Node &other) const
    {
        return !(*this == other);
    }

    bool
    operator== (const Node &other) const
    {
        return this->logical_str() == other.logical_str();
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
        std::string str;
        const Node &N = *this;

        if (!this->is_operator()) {
            return N.type;
        }

        /* negation just adds the obvious expression negation */
        if (N.type == "!") {
            return str + "!(" + N.children.begin()->logical_str() + ")";
        }

        for (auto &child : N.children) {
            if (child.is_operator())
                str += "(";
            str += child.logical_str();
            if (child.is_operator())
                str += ")";
            if (N.type == "+" && child != *std::prev(N.children.end()))
                str += N.type;
        }

        return str;
    }
};

#endif
