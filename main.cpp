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
        : type('@')
    { }

    Node (char type) 
        : type(type)
    { }
};

char var ();
void g_or (Node &);
void g_and (Node &);
Node expr ();

/*
 * var -> [A-Za-z01]
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

/*
 * <sub> = (<expr>)
 */
Node
sub ()
{
    match('(');
    Node N = expr();
    match(')');
    return N;
}

/* <or>   = +<var><or> | +<expr><or> */
void
g_or (Node &N)
{
    match('+');

    /* input at this point: ab */
    if (isalnum(look()) && isalnum(look_n(1))) {
        N.children.push_back(expr());
    }
    /* input at this point: ( */
    else if (look() == '(') {
        N.children.push_back(sub());
    }
    /* input at this point: a */
    else if (isalnum(look())) {
        N.values.insert(var());
    }
                
    if (look() == '+') {
        g_or(N);
    }
}

/* <and>  = <var><var><or><and> | <var><expr><and> */
void
g_and (Node &N)
{
    N.values.insert(var());

    /* input at this point: a+ */
    if (isalnum(look()) && look_n(1) == '+') {
        /* 
         * We make 'and' be left-and-right-associative while the 'or' is 
         * simply left-associative. This lets us parse expressions like:
         * a+bc+d into the tree: (+ ad(* bc)) and not (+ a (* b (+ c d)))
         */
        N.values.insert(var());
        Node Or('+');
        g_or(Or);
        N.children.push_back(Or);
    } 
    /* input at this point: ( */
    else if (look() == '(') {
        N.children.push_back(sub());
    } 
    /* input at this point: a */
    else if (isalnum(look())) {
        N.values.insert(var());
    }

    if (isalnum(look())) {
        g_and(N);
    }
}

/* <expr> = <sub><expr> | <var> | <var><or><expr> | <var><and><expr> */
Node
expr ()
{
    Node N;
    char c = look();

    if (isalnum(c) && look_n(1) == '+') {
        N.type = '+';
        N.values.insert(var());
        g_or(N);
        return N;
    }
    else if (isalnum(c) && (isalnum(look_n(1)) || look_n(1) == '(')) {
        N.type = '*';
        g_and(N);
        return N;
    }
    else {
        N.values.insert(var());
        return N;
    }
}

Node
parse_file (const char *inputfile)
{
    Node n;
    INPUT_FILE = fopen(inputfile, "r");
    if (!INPUT_FILE) {
        perror(strerror(errno));
        exit(1);
    }
    /* set look to whitespace because 'next' loops until no whitespace */
    LOOKAHEAD = ' ';
    next();
    n = expr();
    printf("\n");
    fclose(INPUT_FILE);
    return n;
}

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

int
main (int argc, char **argv)
{
    Node expr = parse_file("input.txt");
    print_tree(expr);
    return 0;
}
