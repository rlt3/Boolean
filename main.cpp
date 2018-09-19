#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <cerrno>
#include <vector>

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
    Node *left;
    Node *right;
    char value;
    Node (char val) : left(NULL), right(NULL), value(val) { }
};

/*
 * Var -> !?[A-Za-z]
 */
Node*
var ()
{
    bool negate = false;
    Node *n, *s;

    if (look() == '!') {
        match('!');
        negate = true;
    }

    if (!isalpha(look())) {
        fprintf(stderr, "Expected character instead got '%c'\n", look());
        exit(1);
    }
    n = new Node(look());
    match(look());

    if (negate) {
        s = new Node('!');
        s->right = n;
        n = s;
    }

    return n;
}

/*
 * Term -> VarTerm | Var
 */
Node*
term ()
{
    Node *s, *n = var();

    if (isalpha(look())) {
        s = new Node('*');
        s->left = n;
        s->right = term();
        n = s;
    }

    return n;
}

/*
 * Expr -> !Expr | (Expr) | Expr + Expr | ExprExpr | Term
 */
Node*
expr ()
{
    bool negate = false;
    Node *n, *c;

    /* 
     * An expression can only be negated if it is wrapped in parentheses.
     * Otherwise, it means we are negating a single term, e.g. !(ab) vs. !ab
     */
    if (look() == '!' && look_n(1) == '(') {
        match('!');
        negate = true;
    }

    if (look() == '(') {
        match('(');
        n = expr();
        match(')');
    } else {
        n = term();
    }

    if (negate) {
        c = new Node('!');
        c->right = n;
        n = c;
    }

    if (look() == '+') {
        match('+');
        c = new Node('+');
        c->left = n;
        c->right = expr();
        n = c;
    }
    /* these characters are the start of another expression */
    else if (look() == '(' || look() == '!' || isalpha(look())) {
        c = new Node('*');
        c->left = n;
        c->right = expr();
        n = c;
    }
    /* if this isn't the end of an expression or end of input, error */
    else if (!(look() == ')' || look() == EOF)) {
        fprintf(stderr, "Unexpected '%c'\n", look());
        exit(1);
    }

    return n;
}

Node*
parse_file (const char *inputfile)
{
    Node *n;
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
free_expr (Node *n)
{
    static int tab = 0;

    for (int i = 0; i < tab; i++)
        printf("   ");
    printf("%c\n", n->value);
    tab++;

    if (n->left) {
        free_expr(n->left);
        tab--;
    }
    if (n->right) {
        free_expr(n->right);
        tab--;
    }
    delete n;
}

/*
 * The first step: distribute all terms of any subexpressions so that only the
 * operators only have single terms as operands, e.g.:
 *      !a(ab + c) -> !aa * ab + !ac
 *      (ab)(cd) -> ac * ad * bc * bd
 */
Node*
distribute (Node *n)
{
    return n;
}

/*
 * Apply reduction rules such as !aa = 0, !a + a = 1, a0 = 0, etc.
 */
Node*
reduce (Node *n)
{
    return n;
}

/*
 * The reverse of `distribute': factor out all redundant terms.
 */
Node*
factor (Node *n)
{
    return n;
}

int
main (int argc, char **argv)
{
    Node *expression = parse_file("input.txt");
    expression = distribute(expression);
    expression = reduce(expression);
    expression = factor(expression);
    free_expr(expression);
    return 0;
}
