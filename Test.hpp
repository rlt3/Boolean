#ifndef TEST_HPP
#define TEST_HPP

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
        int fail_chance = 0;
        Tree = rand_node(fail_chance, rng);
        Tree.print_tree();
        Tree.print_logical();
    }

protected:
    Node Tree;

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
            if (rand < fail_chance) {
                /* 
                 * A node must have at least two operands. This will catch all
                 * potential states a node can be that will *not* satisfy
                 * having a node with operands.
                 */
                if (N.children.size() == 0 && N.values.size() == 0)
                    N.add_value(add_var(rng));
                if (N.children.size() == 1 && N.values.size() == 0)
                    N.add_value(add_var(rng));
                if (N.children.size() == 0 && N.values.size() == 1)
                    N.add_value(add_var(rng));
                break;
            }

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
                default:
                    fprintf(stderr, "Error, generated: %d\n", rand);
                    exit(1);
            }
        }

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
