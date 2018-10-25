#ifndef NODE_HPP
#define NODE_HPP

#include <vector>
#include <string>
#include <set>

struct Node {
    char type;
    std::set<std::string> values;
    std::vector<Node> children;

    Node () 
        : type('+')
    { }

    Node (char type) 
        : type(type)
    { }

    int
    max_depth (const int depth) const
    {
        int max = depth;
        for (auto &child : this->children)
            max = std::max(child.max_depth(depth + 1), depth);
        return max;
    }

    int
    depth () const
    {
        return max_depth(0);
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
        if (this->values != other.values)
            return false;
        if (this->children.size() != other.children.size())
            return false;
        for (unsigned i = 0; i < this->children.size(); i++)
            if (!(this->children[i] == other.children[i]))
                return false;
        return true;
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
        if (child.type == '!') {
            this->add_child(child);
        } else if (child.type == this->type) {
            for (auto &grandchild : child.children)
                this->add_child(grandchild);
            for (auto &val : child.values)
                this->add_value(val);
        } else if (child.children.size() == 0 && child.values.size() == 1) {
            this->add_value(*child.values.begin());
        } else if (child.children.size() == 1 && child.values.size() == 0) {
            this->add_child(child.children[0]);
        } else {
            this->add_child(child);
        }
    }

    void
    add_child (Node child)
    {
        this->children.push_back(child);
    }

    void
    add_value (std::string value)
    {
        this->values.insert(value);
    }

    void
    print_tree () const
    {
        static int tab = 0;
        const Node &N = *this;

        for (int i = 0; i < tab; i++)
            printf("   ");
        printf("%c\n", N.type);
        tab++;

        for (auto c : N.values) {
            for (int i = 0; i < tab; i++)
                printf("   ");
            printf("%s\n", c.c_str());
        }

        for (auto c : N.children) {
            c.print_tree();
            tab--;
        }

        if (tab == 1) {
            tab = 0;
            printf("-------------------------------\n");
        }
    }

    std::string
    logical_str () const
    {
        std::string str;
        const Node &N = *this;

        if (N.type == '0' || N.type == '1') {
            return std::string(1, N.type);
        }

        /* negation just adds the obvious expression negation */
        if (N.type == '!') {
            str += "!(";
            if (N.children.size() == 1) {
                str += N.children[0].logical_str();
            } else {
                str += *N.values.begin();
            }
            str += ")";
            return str;
        }

        for (auto &var : N.values) {
            str += var;
            /* always add the plus even at the end if there's children to print */
            if (N.type == '+' && (N.children.size() > 0 || var != *std::prev(N.values.end())))
                str += N.type;
        }

        for (unsigned i = 0; i < N.children.size(); i++) {
            str += "(";
            str += N.children[i].logical_str();
            str += ")";
            if (N.type == '+' && i != N.children.size() - 1)
                str += N.type;
        }

        return str;
    }
};

#endif
