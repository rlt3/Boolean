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

    /* Reduce the child before adding it as a value or child */
    void
    add_reduction (Node child)
    {
        if (child.children.size() == 0 && child.values.size() == 1) {
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

        /* negation just adds the obvious expression negation */
        if (N.type == '!') {
            str += "!(";
            str += this->children[0].logical_str();
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
