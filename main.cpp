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

    if (isalpha(look()) || look() == '!') {
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
print_expr (Node *n)
{
    static int tab = 0;

    if (tab == 0) {
        printf("%p:\n", (void*) n);
    }

    for (int i = 0; i < tab; i++)
        printf("   ");
    printf("%c\n", n->value);
    tab++;

    if (n->left) {
        print_expr(n->left);
        tab--;
    }
    if (n->right) {
        print_expr(n->right);
        tab--;
    }
    if (tab == 1) {
        tab = 0;
        printf("-------------------------------\n");
    }
}

void
free_expr (Node *n)
{
    if (n->left)
        free_expr(n->left);
    if (n->right)
        free_expr(n->right);
    delete n;
}

bool
is_type (Node *n, const std::string types)
{
    if (!n)
        return false;
    for (const auto &c : types)
        if (n->value == c)
            return true;
    return false;
}

void
set_recurse (Node *n, std::set<char> &s)
{
    if (!n)
        return;
    set_recurse(n->left, s);
    set_recurse(n->right, s);
    if (isalnum(n->value))
        s.insert(n->value);
}

std::set<char>
get_set (Node *n)
{
    std::set<char> s;
    set_recurse(n, s);
    return s;
}

bool
is_constant (Node *n)
{
    if (!n)
        return true;
    if (isalnum(n->value))
        return true;
    if (n->value == '!')
        return is_constant(n->right);
    return false;
}

Node*
deep_copy (Node *n)
{
    if (!n)
        return n;

    Node *C = new Node(n->value);
    C->left = deep_copy(n->left);
    C->right = deep_copy(n->right);
    return C;
}

/*
 *      N                      R.op
 *    /   \                  /   \
 *   L     R       ->       N.op  N.op
 *        / \              / \   / \
 *      RL   RR           L  RL L  RR
 *
 * For each child of R, we make a new Node whose operation if the operation
 * of L, and put that child of R on the right of that node and L on the
 * left of that Node.
 * The new head of the entire tree should have the operation of R.
 */
Node*
distribute (Node *N)
{
    /* head, left, right */
    Node *H, *L, *R;

    print_expr(N);

    if (!N)
        return N;

    if (!(N->left || N->right))
        return N;

    if (is_constant(N->left) && is_constant(N->right))
        return N;

    /* 
     * if the tree has a constant expr on the right, we flip left and right so
     * the algorithm can handle that case.
     */
    if (is_constant(N->right)) {
        L = N->right;
        R = N->left;
    } else {
        R = N->right;
        L = N->left;
    }

    H = new Node(R->value);

    if (R->left) {
        H->left = new Node(N->value);
        H->left->left = deep_copy(L);
        H->left->right = deep_copy(R->left);
        H->left = distribute(H->left);
    }

    if (R->right) {
        H->right = new Node(N->value);
        H->right->left = deep_copy(L);
        H->right->right = deep_copy(R->right);
        H->right = distribute(H->right);
    }

    return H;
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
   Node *expr, *dist;
   expr = parse_file("input.txt");
   //Node *L = deep_copy(expr->left);
   //Node *R = deep_copy(expr->right);
   //free_expr(L);
   //free_expr(R);
   //free_expr(expr);
   dist = distribute(expr);
   print_expr(dist);
   free_expr(expr);
   free_expr(dist);
   //expr = reduce(expression);
   //expr = factor(expression);
   return 0;
}
