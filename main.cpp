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
deep_copy (Node *n, Node* parent)
{
    if (!n)
        return n;

    Node *C = new Node(n->value);
    C->parent = parent;
    C->left = deep_copy(n->left, C);
    C->right = deep_copy(n->right, C);
    return C;
}

void
print_tree (Node *n)
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
        print_tree(n->left);
        tab--;
    }
    if (n->right) {
        print_tree(n->right);
        tab--;
    }
    if (tab == 1) {
        tab = 0;
        printf("-------------------------------\n");
    }
}

void
print_logical (Node *n)
{
    int is_expr = false;

    if (!n)
        return;

    if (!is_constant(n))
        is_expr = true;

    if (!is_expr)
        printf("%c", n->value);

    if (is_expr)
        printf("(");

    if (n->left)
        print_logical(n->left);

    if (is_expr) {
        if (n->value == '+')
            printf(" || ");
        else if (n->value == '*')
            printf(" && ");
        else
            printf("!");
    }

    if (n->right)
        print_logical(n->right);

    if (is_expr)
        printf(")");

    if (!n->parent)
        printf("\n");
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
distribute_iter (char op, Node *L, Node *R)
{
    Node *H, *tmp;

    /* 
     * All iterations of distribute end up here at which point we copy the
     * contents of L and R and make a new Node to hold them.
     */
    if (is_constant(L) && is_constant(R)) {
        H = new Node(op);
        H->add_left(deep_copy(L, H));
        H->add_right(deep_copy(R, H));
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
    H->add_left(distribute_iter(op, L, R->left));
    H->add_right(distribute_iter(op, L, R->right));
    return H;
}

/*
 * We use this function as a guard for `distribute_iter` which will always 
 * allocate memory for the new tree. If no distribution occurs then no memory
 * should be allocated or free'd.
 */
Node *
distribute (Node *N)
{
    if (!N)
        return N;

    /* Variables and Constants (e.g. a,b,0,1) cannot be distributed */
    if (isalnum(N->value))
        return N;

    /* Compound statements of variables cannot be distributed, e.g a + b */
    if (is_constant(N->left) && is_constant(N->right))
        return N;

    /* distribute when there's at least one compound statement */
    return distribute_iter(N->value, N->left, N->right);
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
    Node *expr, *simp;

    //bool are_equal;
    //while (1) {
    //    simp = distribute(expr);
    //    /* 
    //     * No distribution happened if no memory was allocated, thus no need to
    //     * free any expressions here.
    //     */
    //    if (simp == expr)
    //       break;
    //    simp = reduce(simp);
    //    simp = factor(simp);
    //    /* if `simp` factors back into `expr' then it's in most-simple terms */
    //    are_equal = expr_equal(expr, simp);
    //    /* `expr' must be free'd regardless if loop is done or not */
    //    free_expr(expr);
    //    expr = simp;
    //    if (are_equal)
    //        break;
    //}

    expr = parse_file("input.txt");
    print_tree(expr);
    simp = distribute(expr);
    print_tree(simp);
    print_logical(simp);
    free_expr(expr);
    free_expr(simp);

    return 0;
}
