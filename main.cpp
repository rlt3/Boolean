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
    printf(" %c ", c);
    next();
}

struct Node {
    char type;
    std::set<std::string> values;
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
        this->children.push_back(child);
    }

    void
    add_value (std::string value)
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
        printf("%s\n", c.c_str());
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

std::string var ();
void prod (Node &);
void expr (Node &);

/*
 * var = !?[A-Za-z01]
 */
std::string
var ()
{
    std::string v;

    if (look() == '!') {
        match('!');
        v += '!';
    }

    if (!isalnum(look())) {
        fprintf(stderr, "Expected character instead got '%c'\n", look());
        exit(1);
    }
    v += look();
    match(look());

    return v;
}

void
absorb (Node &parent)
{
    std::vector<Node> absorbed_children;
    std::set<std::string> absorbed_values;
    std::vector<int> nodes_to_remove;

    if (parent.type == '!')
        return;

    for (unsigned i = 0; i < parent.children.size(); i++) {
        const auto &child = parent.children[i];

        if (child.children.size() != 1)
            continue;

        if (child.type == '!')
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
 * <prod> = <expr><prod> | <var><prod> | <var>
 */
void
prod (Node &N)
{
    while (1) {
        if (look() == '(' || (look() == '!' && look_n(1) == '(')) {
            Node E('+');
            expr(E);
            absorb(E);

            if (E.type != '!' && E.children.size() == 1 && E.values.size() == 0) {
                E = E.children[0];
            }

            N.add_child(E);
        }
        else if (look() == '!' || isalnum(look())) {
            N.add_value(var());
        }

        if (!(look() == '(' || look() == '!' || isalnum(look())))
            break;
    }
}

/*
 * <expr> = !?(<expr>) | <var> | <var>+<expr> | <prod> | <prod>+<expr>
 */
void
expr (Node &N)
{
    while (1) {
        if (look() == '(' || (look() == '!' && look_n(1) == '(')) {
            bool is_negated = false;

            if (look() == '!') {
                match('!');
                is_negated = true;
            }

            match('(');
            Node E('+');
            expr(E);

            /* 
             * sub-expression is not isolated and is part of a sum of products,
             * so we call 'expr' to gather all other sums of products and add
             * the current parsed nodes (E) to the other parsed sums.
             */
            if (look() != ')' && (look() == '!' || isalnum(look()))) {
                Node S('+');
                expr(S);
                S.children[0].children.push_back(E.children[0]);
                E = S;
            } 

            match(')');

            if (E.type != '!' && E.children.size() == 1 && E.values.size() == 0) {
                E = E.children[0];
            }

            if (is_negated) {
                Node Neg('!');
                Neg.add_child(E);
                E = Neg;
            }

            N.add_child(E);
        }
        else {
            Node P('*');
            prod(P);
            /* if we have a product of a single child, add child to sum */
            if (P.children.size() == 1 && P.values.size() == 0) {
                N.add_child(P.children[0]);
            }
            /* handles products of a single value, adding it directly to sum */
            else if (P.children.size() == 0 && P.values.size() == 1) {
                N.add_value(*P.values.begin());
            }
            else {
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
    if (N.type != '!' && N.children.size() == 1 && N.values.size() == 0)
        N = N.children[0];
    printf("\n");
    fclose(INPUT_FILE);
    return N;
}

void
print_logical (Node &N)
{
    static int depth = 0;

    for (auto &child : N.values) {
        printf("%s", child.c_str());
        if (N.type == '+' && (N.children.size() > 0 || child != *std::prev(N.values.end())))
            printf("%c", N.type);
    }

    depth++;
    for (unsigned i = 0; i < N.children.size(); i++) {
        if (N.type == N.children[i].type)
            printf("(");
        print_logical(N.children[i]);
        if (N.type == N.children[i].type)
            printf(")");
        if (N.type == '+' && i != N.children.size() - 1)
            printf("%c", N.type);
    }
    depth--;

    if (depth == 0)
        printf("\n");
}

int
main (int argc, char **argv)
{
    Node expr = parse_file("input.txt");
    print_tree(expr);
    //print_logical(expr);
    return 0;
}
