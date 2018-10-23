#include "Node.hpp"
#include "Parse.hpp"

void
usage (char *prog)
{
    fprintf(stderr, "Usage: %s <expression>\n", prog);
    exit(1);
}

int
main (int argc, char **argv)
{
    if (argc != 2)
        usage(argv[0]);

    if (strlen(argv[1]) == 0)
        usage(argv[0]);

    set_input(std::string(argv[1]));

    Node expr = parse_input();
    expr.print_tree();
    std::cout << expr.logical_str() << std::endl;

    return 0;
}
