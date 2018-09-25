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
    Node *parent;
    char value;

    Node (char val) 
        : left(NULL), right(NULL), parent(NULL), value(val) 
    { }

    void
    add_right (Node *n)
    {
        this->right = n;
        n->parent = this;
    }

    void
    add_left (Node *n)
    {
        this->left = n;
        n->parent = this;
    }
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
        s->add_right(n);
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
        s->add_left(n);
        s->add_right(term());
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
        c->add_right(n);
        n = c;
    }

    if (look() == '+') {
        match('+');
        c = new Node('+');
        c->add_left(n);
        c->add_right(expr());
        n = c;
    }
    /* these characters are the start of another expression */
    else if (look() == '(' || look() == '!' || isalpha(look())) {
        c = new Node('*');
        c->add_left(n);
        c->add_right(expr());
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
    if (!n)
        return;
    free_expr(n->left);
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
 *      op                     R
 *    /   \                  /   \
 *   L     R       ->       op   op
 *        / \              / \   / \
 *      RL   RR           L  RL L  RR
 *
 * R becomes the new root node. Since we are distributing over 'op' then that
 * becomes the new children node. Then it is a simple recursion with L staying
 * constant and R collectively being iterated down to a term.
 */
Node *
distribute_iter (char op, Node *L, Node *R, Node *parent)
{
    Node *H, *tmp;

    /* 
     * All iterations of distribute end up here at which point we copy the
     * contents of L and R and make a new Node to hold them.
     */
    if (is_constant(L) && is_constant(R)) {
        H = new Node(op);
        H->add_left(deep_copy(L));
        H->add_right(deep_copy(R));
        return H;
    }

    /* 
     * If the tree has a constant expression on the right, we flip left and
     * right. The algorithm is setup so that it uses right's children.
     */
    if (is_constant(R)) {
        tmp = L;
        L = R;
        R = tmp;
    }

    H = new Node(R->value);
    H->add_left(distribute_iter(op, L, R->left, NULL));
    H->add_right(distribute_iter(op, L, R->right, NULL));
    return H;
}

/*
 * This particular function is setup as a DFS. When it reaches a node with at
 * least one compound statement it can then distribute those terms. This 
 * distribution is done with `distribute_iter` which is setup as a BFS and also
 * allocates a new subtree for the distributed terms.
 */
Node *
distribute (Node *N)
{
    Node *H, *L, *R;

    if (!N)
        return N;

    /* Variables and Constants (e.g. a,b,0,1) cannot be distributed */
    if (isalnum(N->value))
        return N;

    L = distribute(N->left);
    R = distribute(N->right);

    /* Compound statements of variables cannot be distributed, e.g a + b */
    if (is_constant(L) && is_constant(R))
        return N;

    /* distribute when there's at least one compound statement */
    H = distribute_iter(N->value, L, R, NULL);

    /*
     * `distribute_iter' creates new subtrees and thus must be freed if they
     * were each then used for distributing expression.
     */
    if (N->left != L)
        free_expr(L);
    if (N->right != R)
        free_expr(R);

    /* 
     * If we make it this far, distribution had to occur then N's values will
     * have been copied to the leaves of the distributed subtree. Detach `N'
     * from the tree and free only the N subexpression.
     */
    if (N->parent && N->parent->left == N)
        N->parent->left = NULL;
    if (N->parent && N->parent->right == N)
        N->parent->right = NULL;
    free_expr(N);

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
   Node *expr;
   expr = parse_file("input.txt");
   print_expr(expr);
   expr = distribute(expr);
   print_expr(expr);
   free_expr(expr);
   //expr = reduce(expression);
   //expr = factor(expression);
   return 0;
}
