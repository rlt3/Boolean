#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <cerrno>
#include <vector>
#include <string>
#include <set>

static FILE *INPUT_FILE = NULL;
static int LOOKAHEAD;

/*
 * 1 character look ahead
 */
int
look ()
{
    return LOOKAHEAD;
}

/*
 * Arbitrary character look ahead.
 */
int
look_n (int count)
{
    std::vector<int> read;
    /* read all character read including whitespace */
    for (int i = 0; i < count; i++) {
        do {
            read.push_back(getc(INPUT_FILE));
        } while (isspace(read.back()));
    }
    /* and put them back */
    for (int i = (int) read.size() - 1; i >= 0; i--) {
        ungetc(read[i], INPUT_FILE);
    }
    return read.back();
}

int
next ()
{
    do {
        LOOKAHEAD = getc(INPUT_FILE);
    } while (isspace(LOOKAHEAD));
    return LOOKAHEAD;
}

void
match (char c)
{
    if (look() != c) {
        fprintf(stderr, "Expected character '%c' got '%c'\n", c, look());
        exit(1);
    }
    //printf(" %c ", c);
    next();
}

struct Node {
    /*
     * a set of character values.
     * a vector of children.
     * a type.
     *
     * constant expressions would have no children, e.g. a+b+c or abc.
     * otherwise there are children, e.g. a+(bc)+d
     * instead of having an actual node for values, it is simply a character
     * which keeps values very, very simple and makes 'sets' 10x easier to use.
     * that's the goal: use c++'s strengths and stop forcing shit with weird
     * overloading of operators because we have a weird nested type as a set.
     * YOU WILL SEE A DRAMATIC INCREASE IN PRODUCTIVITY THIS WAY
     *
     * <var>  = [A-Za-z01]
     * <sub>  = (<expr>)
     * <or>   = +<var><or> | +<sub> | E
     * <and>  = <var><and> | <sub> | E
     * <expr> = <sub><expr> | <var> | <var><or><expr> | <var><and><expr>
     *
     * In this grammar, every time <expr> is called it creates a new Node. That
     * node exists and is used as long as there is a single compound type being
     * used or a single variable is read, e.g. a+b+c is first read as a 
     * <var><or><expr> where <or> is recursive and thus will simply 'fill' up
     * that single expression node until it cannot. Any call to <expr> will add
     * its return value as a child to the current node it has. This will remove
     * any need for an absorption function immediately after parsing (and
     * possibly in general).
     *
     */
    char type;
    std::set<char> values;
    std::vector<Node> children;

    Node () 
        /* this means 'constant type', i.e a Node with a singular variable */
        : type('+')
    { }

    Node (char type) 
        : type(type)
    { }

    void
    add_child (Node child)
    {
        //if (child.type == '@')
        //    this->add_value(*child.values.begin());
        //else
        this->children.push_back(child);
    }

    void
    add_value (char value)
    {
        this->values.insert(value);
    }
};

void
print_tree (Node &n)
{
    static int tab = 0;

    for (int i = 0; i < tab; i++)
        printf("   ");
    printf("%c\n", n.type);
    tab++;

    for (auto c : n.values) {
        for (int i = 0; i < tab; i++)
            printf("   ");
        printf("%c\n", c);
    }

    for (auto c : n.children) {
        print_tree(c);
        tab--;
    }

    if (tab == 1) {
        tab = 0;
        printf("-------------------------------\n");
    }
}

char var ();
void prod (Node &);
void expr (Node &);

/*
 * var = [A-Za-z01]
 */
char
var ()
{
    char c = look();
    if (!isalnum(c)) {
        fprintf(stderr, "Expected character instead got '%c'\n", c);
        exit(1);
    }
    match(c);
    return c;
}

void
absorb (Node &parent)
{
    std::vector<Node> absorbed_children;
    std::set<char> absorbed_values;
    std::vector<int> nodes_to_remove;

    for (unsigned i = 0; i < parent.children.size(); i++) {
        const auto &child = parent.children[i];

        if (child.children.size() != 1)
            continue;

        if (child.children[0].type != parent.type)
            continue;

        nodes_to_remove.push_back(i);
        absorbed_children.push_back(child.children[0]);
        for (auto v : child.values)
            absorbed_values.insert(v);
    }

    for (auto &i : nodes_to_remove)
        parent.children.erase(parent.children.begin() + i);
    for (auto &c : absorbed_children)
        parent.children.push_back(c);
    for (auto &v : absorbed_values)
        parent.values.insert(v);
}

/*
 * <prod> = (<expr>)<prod> | <var><prod> | <var>
 */
void
prod (Node &N)
{
    while (1) {
        if (look() == '(') {
            match('(');
            Node E('+');
            expr(E);
            absorb(E);
            if (E.children.size() == 1 && E.values.size() == 0) {
                N.add_child(E.children[0]);
            } else {
                N.add_child(E);
            }
            match(')');
        }
        else if (isalnum(look())) {
            N.add_value(var());
        }

        if (!(look() == '(' || isalnum(look())))
            break;
    }
}

/*
* <expr> = <var> | <var>+<expr> | <prod> | <prod>+<expr>
 */
void
expr (Node &N)
{
    while (1) {
        if (isalnum(look()) && (look_n(1) == '+' || look_n(1) == EOF || look_n(1) == ')')) {
            N.add_value(var());
        }
        else {
            Node P('*');
            prod(P);
            if (P.children.size() == 1 && P.values.size() == 0) {
                N.add_child(P.children[0]);
            } else {
                N.add_child(P);
            }
        }

        if (look() != '+')
            break;
        match('+');
    }
}

Node
parse_file (const char *inputfile)
{
    Node N;
    INPUT_FILE = fopen(inputfile, "r");
    if (!INPUT_FILE) {
        perror(strerror(errno));
        exit(1);
    }
    /* set look to whitespace because 'next' loops until no whitespace */
    LOOKAHEAD = ' ';
    next();
    expr(N);
    if (N.children.size() == 1 && N.values.size() == 0)
        N = N.children[0];
    printf("\n");
    fclose(INPUT_FILE);
    return N;
}

int
main (int argc, char **argv)
{
    Node expr = parse_file("input.txt");
    print_tree(expr);
    return 0;
}
