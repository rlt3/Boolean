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

struct Node;
void print_tree(Node &N);

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

    void
    add_child_value (Node child)
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
Node sub ();
Node negate ();
Node prod ();
Node expr ();

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

/*
 * <negate> = !(<expr>)
 */
Node
negate ()
{
    Node N('!');
    match('!');
    match('(');
    N.add_child(expr());
    match(')');
    return N;
}

/*
 * <prod> = <negate><prod> | <sub><prod> | <var><prod> | E
 */
Node
prod ()
{
    Node N('*');

    while (1) {
        if (look() == '!' && look_n(1) == '(')
            N.add_child(negate());
        else if (look() == '(')
            N.add_child(sub());
        else if (isalnum(look()) || (look() == '!' && isalnum(look_n(1))))
            N.add_value(var());

        if (look() == '+' || look() == ')' || look() == EOF)
            break;
    }

    return N;
}

/*
 * <expr> = <prod> | <prod>+<expr>
 */
Node
expr ()
{
    /* use a sentinel value not in language to mark 'unset' */
    Node N('@');

    while (look() != EOF) {
        Node P = prod();

        N.add_child_value(P);

        /* 
         * Anytime we find an disjunction ('+') then we make sure N has that
         * type. Note that a disjunction can overwrite a conjunction but not
         * vice-versa.
         */
        if (look() == '+') {
            N.type = '+';
            match('+');
        } else if (N.type == '@') {
            N.type = '*';
        }

        if (look() == EOF || look() == ')')
            break;
    }

    /* 
     * if we have an expression of a single child, e.g. a or (b) or ((c+d))
     * then make that child the root and return it
     */
    if (N.children.size() == 1 && N.values.size() == 0)
        N = N.children[0];

    return N;
}

Node
parse_file (const char *inputfile)
{
    INPUT_FILE = fopen(inputfile, "r");
    if (!INPUT_FILE) {
        perror(strerror(errno));
        exit(1);
    }
    /* set look to whitespace because 'next' loops until no whitespace */
    LOOKAHEAD = ' ';
    next();
    Node N = expr();
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

    if (N.type == '!') {
        printf("!(");
        depth++;
        print_logical(N.children[0]);
        depth--;
        printf(")");
        goto exit;
    }

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

exit:
    if (depth == 0)
        printf("\n");
}

int
main (int argc, char **argv)
{
    Node expr = parse_file("input.txt");
    print_tree(expr);
    print_logical(expr);
    return 0;
}
