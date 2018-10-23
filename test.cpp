#ifndef TEST_CPP
#define TEST_CPP

#include "Node.hpp"
#include "Parse.hpp"
#include <random>
#include <vector>
#include <algorithm>

Node rand_node (int &stop_chance, std::mt19937_64 &rng);
std::string add_var (std::mt19937_64 &rng);
std::string add_var (std::mt19937_64 &rng);
Node add_sub (int &stop_chance, std::mt19937_64 &rng);
Node add_negation (int &stop_chance, std::mt19937_64 &rng);

/*
 * Generate a new test case. This means randomly generating a new parse tree
 * using the nodes themselves. The idea is that a tree can print its own
 * logical string expression. Therefore, if the parser is working correctly,
 * any randomly generated parse tree can produce its own input to the parser
 * which then could generate its own parse tree.
 * Comparing the two trees is the our test.
 */
bool
generate_test (bool verbose)
{
    std::random_device device;
    std::mt19937_64 rng(device());
    std::string input;
    int stop_chance;
    Node Tree, E;

    stop_chance = 0;
    Tree = rand_node(stop_chance, rng);
    input = Tree.logical_str();
    set_input(input);
    E = parse_input();

    if (!(Tree == E)) {
        printf("Input fails to reproduce itself: '%s'\n", input.c_str());
        if (verbose) {
            Tree.print_tree();
            E.print_tree();
        }
        return false;
    } else if (verbose) {
        printf("%s ok\n", input.c_str());
    }

    return true;
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
add_sub (int &stop_chance, std::mt19937_64 &rng)
{
    return rand_node(stop_chance, rng);
}

Node
add_negation (int &stop_chance, std::mt19937_64 &rng)
{
    Node N('!');
    N.add_reduction(rand_node(stop_chance, rng));
    return N;
}

/* 
 * Recursively generate a tree of nodes. Each 'action' done increases the
 * probability of stopping by 5%. Each action is randomly chosen from a list
 * of actions.
 */
Node
rand_node (int &stop_chance, std::mt19937_64 &rng)
{
    static std::uniform_int_distribution<int> node_choice(0, 1);
    static std::uniform_int_distribution<int> action_choice(0, 100);
    /* 
     * Each choice marks the start of where that choice 'begins' on a number
     * line from 0 to 100 and uses the next choice to know where that choice
     * 'ends'. If a random number 47 was produced, the choice would be 'v'
     * because 0 til 50 is choice 'v'. If 95 was produced, then the choice
     * would be '!'. '0' is a marker and isn't a real choice.
     */
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
        if (rand < stop_chance)
            break;

        rand = action_choice(rng);

        /* get the choice from the list of choices above */
        for (int i = 0; i < 3; i++) {
            if (rand >= choices[i][0] && rand <= choices[i+1][0]) {
                choice = choices[i][1];
                break;
            }
        }

        stop_chance += 5;

        switch (choice) {
            case 'v': 
                N.add_value(add_var(rng));
                break;
            case 's':
                N.add_reduction(add_sub(stop_chance, rng));
                break;
            case '!':
                N.add_child(add_negation(stop_chance, rng));
                break;
        }
    }

    /* A node must have at least two operands */
    while (N.children.size() + N.values.size() < 2)
        N.add_value(add_var(rng));
    return N;
}

void
usage (const char *prog)
{
    fprintf(stderr, "%s [<num-of-tests>] [<verbose=1|0>]\n", prog);
    exit(1);
}

int
main (int argc, char **argv)
{
    unsigned verbosity = false;
    unsigned num_tests = 1000;
    bool all_passed = true;

    if (argc < 2 || argc > 3)
        usage(argv[0]);

    sscanf(argv[1], "%u", &num_tests);
    if (argc > 2)
        sscanf(argv[2], "%u", &verbosity);

    for (unsigned i = 0; i < num_tests; i++) {
        bool passed = generate_test(verbosity);
        if (all_passed && !passed)
            all_passed = false;
    }

    if (all_passed)
        printf("All tests passed. Good job.\n");

    return 0;
}

#endif
