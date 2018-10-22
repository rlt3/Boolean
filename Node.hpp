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
    print_tree ()
    {
        static int tab = 0;
        Node &N = *this;

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

    void
    print_logical ()
    {
        static int depth = 0;
        unsigned last_child;
        Node &N = *this;

        /* negation just adds the obvious expression negation */
        if (N.type == '!') {
            printf("!(");
            depth++;
            this->children[0].print_logical();
            depth--;
            printf(")");
            goto exit;
        }

        for (auto &var : N.values) {
            printf("%s", var.c_str());
            /* always add the plus even at the end if there's children to print */
            if (N.type == '+' && (N.children.size() > 0 || var != *std::prev(N.values.end())))
                printf("%c", N.type);
        }

        depth++;

        last_child = N.children.size() - 1;
        for (unsigned i = 0; i < N.children.size(); i++) {
            /* if child's next or prev is same type it needs parentheses */
            if ((i < last_child && N.children[i].type == N.children[i+1].type) ||
                (i > 0 && N.children[i].type == N.children[i-1].type)) {
                printf("(");
                N.children[i].print_logical();
                printf(")");
            }
            /* otherwise just print it normally */
            else {
                N.children[i].print_logical();
            }
            if (N.type == '+' && i != last_child)
                printf("%c", N.type);
        }
        depth--;

    exit:
        if (depth == 0)
            printf("\n");
    }
};

#endif
