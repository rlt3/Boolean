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

    if (!isalnum(look())) {
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

    if (isalpha(look()) || (look() == '!' && look_n(1) != '(')) {
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
expr_delete (Node *n)
{
    if (!n)
        return;
    expr_delete(n->left);
    expr_delete(n->right);
    delete n;
}

/*
 * Deepy copy another expression, i.e. the entire tree.
 */
Node*
expr_copy (Node *n, Node* parent)
{
    if (!n)
        return n;

    Node *C = new Node(n->value);
    C->parent = parent;
    C->left = expr_copy(n->left, C);
    C->right = expr_copy(n->right, C);
    return C;
}

/*
 * Returns whether expression is a constant expression.
 */
bool
expr_constant (Node *n)
{
    if (!n)
        return true;
    if (isalnum(n->value))
        return true;
    if (n->value == '!')
        return expr_constant(n->right);
    return false;
}

/*
 * Compare two expressions for equality.
 */
bool
expr_cmp (Node *A, Node *B)
{
    /* if both are null they're equal */
    if (A == NULL && B == NULL)
        return 1;
    /* if one is null and the other isn't */
    if ((A == NULL) ^ (B == NULL))
        return 0;
    if (A->value != B->value)
        return 0;
    /* if we're here they're equal but both branches need to be equal too */
    return 1 && expr_cmp(A->left, B->left) && expr_cmp(A->right, B->right);
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

    if (!expr_constant(n))
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
 * Recurse downwards in a BFS manner. For some negated expression, the negation
 * is distributed to the expression's leaves. The root negation is removed and
 * the new root is either OR or AND for AND or OR respectively.
 *
 *    !             f(N)
 *     \            / \
 *      N     ->   !   !
 *     / \          \   \
 *    A   B          A   B
 */
Node *
de_morgans_laws (Node *N)
{
    /*
     * Useful tests: 
     *      !(pq) => !p+!q
     *      !(p+q) => !p!q
     *      !!!p => !p
     *      !!p => p
     */

    if (!N)
        return N;

    if (N->value == '!' && !expr_constant(N)) {
        /* give root node the opposite operator of N (in ascii drawing) */
        switch (N->right->value) {
            case '*': N->value = '+'; break;
            case '+': N->value = '*'; break;
        }
        /* N becomes negated and a new left node of root needs to be created */
        N->right->value = '!';
        N->add_left(new Node('!'));
        /* Simply move A to the new left node */
        N->left->add_right(N->right->left);
        /* And remove A from right negation */
        N->right->left = NULL;
    }

    N->left = de_morgans_laws(N->left);
    N->right = de_morgans_laws(N->right);

    return N;
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
    if (expr_constant(L) && expr_constant(R)) {
        H = new Node(op);
        H->add_left(expr_copy(L, H));
        H->add_right(expr_copy(R, H));
        return H;
    }

    /* 
     * If the tree has a constant expression on the right, we flip left and
     * right. The algorithm is setup so that it uses right's children.
     */
    if (expr_constant(R)) {
        tmp = L;
        L = R;
        R = tmp;
    }

    H = new Node(R->value);
    H->add_left(distribute_iter(op, L, R->left));
    H->add_right(distribute_iter(op, L, R->right));
    return H;
}

Node *
distribute (Node *N)
{
    return distribute_iter(N->value, N->left, N->right);
}

/*
 * If reducing to some value, let N be just that value.
 */
Node *
reduce_to (Node *N, const char val)
{
    N->value = val;
    expr_delete(N->left);
    expr_delete(N->right);
    N->right = NULL;
    N->left = NULL;
    return N;
}

/*
 * If reducing to another node V, remove N's presence totally.
 */
Node *
reduce_to (Node *N, Node *V)
{
    /* delete the other node we're not using */
    if (N->left != V)
        expr_delete(N->left);
    if (N->right != V)
        expr_delete(N->right);
    V->parent = NULL;
    delete N;
    return V;
}

/*
 * Apply reduction rules such as !aa = 0, !a + a = 1, a0 = 0, etc.
 *
 *       N
 *     /   \    ->   N
 *    L     R
 */
Node*
reduce (Node *N)
{
    Node *L, *R;

    if (!N)
        return N;

    if (!(N->left && N->right))
        return N;

    N->add_left(reduce(N->left));
    N->add_right(reduce(N->right));
    L = N->left;
    R = N->right;

    /* if both expressions are not constant */
    if (!(expr_constant(L) || expr_constant(R)))
        return N;

    /* !aa = 0 */
    if (N->value == '*' && L->value == '!' && L->right->value == R->value)
        return reduce_to(N, '0');

    /* a!a = 0 */
    if (N->value == '*' && R->value == '!' && R->right->value == L->value)
        return reduce_to(N, '0');
    
    /* a0 = 0 || 0a = 0 */
    if (N->value == '*' && (L->value == '0' || R->value == '0'))
        return reduce_to(N, '0');

    /* 1a = a */
    if (N->value == '*' && L->value == '1')
        return reduce_to(N, R);

    /* a1 = a */
    if (N->value == '*' && R->value == '1')
        return reduce_to(N, L);

    /* aa = 1 */
    if (N->value == '*' && expr_cmp(L, R))
        return reduce_to(N, '1');

    /* a+a = a */
    if (N->value == '+' && expr_cmp(L, R))
        return reduce_to(N, L);

    /* 0+a = a */
    if (N->value == '+' && L->value == '0')
        return reduce_to(N, R);

    /* a+0 = a */
    if (N->value == '+' && R->value == '0')
        return reduce_to(N, L);

    /* 1+a = 1 || a+1 = 1 */
    if (N->value == '+' && (L->value == '1' || R->value == '1'))
        return reduce_to(N, '1');

    return N;
}

/*
 * The inverse of `distribute': factor out all redundant terms.
 *
 * This recurses in a DFS way to find N without two constant children.
 * From node N, we look at the left and right children L and R. The common
 * term T in both L and R are named LT and RT respectively. The leftover or
 * uncommon terms are LL and RR (because there's one in L and one in R).
 * We then combine LL and RR onto N and make a new Node to hold LT and N.
 *
 *       N                 F
 *     /   \             /   \
 *    L     R     ->    T     N
 *   / \   / \               / \
 *  LL LT RT  RR            LL  RR
 *
 */
Node*
factor (Node *N)
{
    /* 
     * I use this macro because I hate redundancy. It simply assigns the common
     * term and uncommon terms to the following variables.  Each variable in
     * the macro holds 'left' or 'right' and is placed literally.
     */
    Node *F, *LT, *RT, *LL, *RR;
#define set_nodes(lt,rt,ll,rr) \
    LT = N->left->lt;  \
    RT = N->right->rt; \
    LL = N->left->ll;  \
    RR = N->right->rr

    /* cannot factor a constant expression (including N == NULL) */
    if (expr_constant(N))
        return N;

    N->left = factor(N->left);
    N->right = factor(N->right);

    if (!(N->left || N->right))
        return N;

    /* reductions were applied, so we cannot factor two constant expressions */
    if (expr_constant(N->left) && expr_constant(N->right))
        return N;

    /*
     * This compares each node of N->left and N->right to see if any
     * expressions are the same and if they are and sets the appropriate
     * expressions to LT, RT, etc. If there are no expressions that are the
     * same, there can be no factoring.
     */
    if (expr_cmp(N->left->left, N->right->left)) {
        set_nodes(left, left, right, right);
    } else if (expr_cmp(N->left->left, N->right->right)) {
        set_nodes(left, right, right, left);
    } else if (expr_cmp(N->left->right, N->right->left)) {
        set_nodes(right, left, left, right);
    } else if (expr_cmp(N->left->right, N->right->right)) {
        set_nodes(right, right, left, left);
    } else {
        return N;
    }

    /* F will have the value of distributed nodes L and R */
    F = new Node(N->left->value);

    /* we always use the left term, so we remove the extra right one */
    expr_delete(RT);

    /* delete the N's left and right so it can hold our uncommon LL and RR */
    delete N->left;
    delete N->right;
    N->add_left(LL);
    N->add_right(RR);

    /* and we combine our common T with N */
    F->add_left(LT);
    F->add_right(N);

    return F;
}

int
main (int argc, char **argv)
{
    Node *expr, *simp;
    bool are_equal;

    expr = parse_file("input.txt");
    expr = de_morgans_laws(expr);

    while (1) {
        simp = distribute(expr);
        simp = reduce(simp);
        simp = factor(simp);
        /* if `simp` factors back into `expr' then it's in most-simple terms */
        are_equal = expr_cmp(expr, simp);
        /* `expr' must be free'd regardless if loop is done or not */
        expr_delete(expr);
        expr = simp;
        if (are_equal)
            break;
    }

    print_tree(expr);
    print_logical(expr);
    expr_delete(expr);

    return 0;
}
