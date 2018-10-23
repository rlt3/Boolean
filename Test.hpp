#ifndef TEST_HPP
#define TEST_HPP

#include "Parse.hpp"
#include <random>
#include <vector>
#include <algorithm>

class Test {
public:
    Test ()
    { }

    void
    generate ()
    {
        std::random_device device;
        std::mt19937_64 rng(device());
        std::string input;
        int fail_chance;
        Node Tree, E;

        for (int i = 0; i < 1000; i++) {
            fail_chance = 0;

            Tree = rand_node(fail_chance, rng);
            input = Tree.logical_str();
            INPUT.clear();
            INPUT.str(input);
            E = parse_input();

            if (!(Tree == E)) {
                fprintf(stderr, "%d: Input fails to reproduce itself: '%s'\n", i, input.c_str());
                Tree.print_tree();
                E.print_tree();
                exit(1);
            }
        }
    }

protected:
    /* 
     * Recursively generate a tree of nodes. Each 'action' done increases the
     * probability of failure by 5%. Each action is randomly chosen from a list
     * of actions.
     */
    Node
    rand_node (int &fail_chance, std::mt19937_64 &rng)
    {
        static std::uniform_int_distribution<int> node_choice(0, 1);
        static std::uniform_int_distribution<int> action_choice(0, 100);
        static int choices[4][2] = {
            {   0, 'v' },
            {  50, 's' },
            {  90, '!' },
            { 100, '0' }
        };

        Node N;
        switch (node_choice(rng)) {
            case 0:  N = Node('+'); break;
            default: N = Node('*'); break;
        }

        char choice;
        int rand;
        
        while (1) {
            rand = action_choice(rng);
            if (rand < fail_chance)
                break;

            rand = action_choice(rng);

            for (int i = 0; i < 3; i++) {
                if (rand >= choices[i][0] && rand <= choices[i+1][0]) {
                    choice = choices[i][1];
                    break;
                }
            }

            fail_chance += 5;

            switch (choice) {
                case 'v': 
                    N.add_value(add_var(rng));
                    break;
                case 's':
                    N.add_child(add_sub(fail_chance, rng));
                    break;
                case '!':
                    N.add_child(add_negation(fail_chance, rng));
                    break;
            }
        }

        /* A node must have at least two operands */
        while (N.children.size() + N.values.size() < 2)
            N.add_value(add_var(rng));
        return N;
    }

    std::string
    add_var (std::mt19937_64 &rng)
    {
        static std::uniform_int_distribution<int> negated_choice(0, 100);
        /* range from a-z in ascii */
        static std::uniform_int_distribution<int> char_choice(97, 122);
        std::string var;

        if (negated_choice(rng) > 80)
            var += '!';

        var += static_cast<char>(char_choice(rng));
        return var;

    }

    Node
    add_sub (int &fail_chance, std::mt19937_64 &rng)
    {
        return rand_node(fail_chance, rng);
    }

    Node
    add_negation (int &fail_chance, std::mt19937_64 &rng)
    {
        Node N('!');
        N.add_child(rand_node(fail_chance, rng));
        return N;
    }
};

#endif
