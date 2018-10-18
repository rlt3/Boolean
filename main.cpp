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
    char value;
    std::set<Node> children;

    Node () 
        : value(0)
    { }

    Node (char val) 
        : value(val)
    { }

    void
    add_child (Node n)
    {
        children.insert(n);
    }

    bool
    operator== (const Node &other) const
    {
        return (this->value == other.value && this->children == other.children);
    }

    bool
    operator< (const Node &other) const
    {
        /* 
         * std::set determines equality by comparing two elements such that:
         * !(a < b) && !(b < a). Therefore, false will be returned for both
         * statements to produce equality. Conversely, two true's will say that
         * the two elements are NOT equal (and therefore put both in the set).
         *
         * If there are two operator values (*,+,!) and their children sets are
         * not the same size, return true. The sizes are equal, set a 
         * Node-by-Node comparison of the sets. Any mismatching elements will
         * cause this to return true. Otherwise false.
         *
         * In the case for two variable symbols, simply compare them.
         */

        if ((value == '*' && value == other.value) || 
            (value == '+' && value == other.value) ||
            (value == '!' && value == other.value))
        {
            if (children.size() != other.children.size())
                return true;
            return !(children == other.children);
        }

        return (this->value < other.value);
    }
};

/*
 * Combine like expressions as much as possible. For example, (ab)cd could
 * simply be abcd.
 */
Node
absorb (Node parent)
{
    std::set<Node> C;

    if (parent.children.empty())
        return parent;

    /* hold all the flattened children's nodes */
    for (auto child : parent.children)
        C.insert(absorb(child));

    /* 
     * for all those flattened children, if any match the parent's value, it
     * can be absorbed by the parent. Otherwise, it is added as another child.
     */
    parent.children.clear();
    for (auto child : C) {
        if (child.value == parent.value) {
            for (auto c : child.children)
                parent.children.insert(c);
        } else {
            parent.children.insert(child);
        }
    }

    return parent;
}

/*
 * var -> !?[A-Za-z01]
 */
Node
var ()
{
    bool negate = false;
    Node n, s;

    if (look() == '!') {
        match('!');
        negate = true;
    }

    if (!isalnum(look())) {
        fprintf(stderr, "Expected character instead got '%c'\n", look());
        exit(1);
    }
    n = Node(look());
    match(look());

    if (negate) {
        s = Node('!');
        s.add_child(n);
        return s;
    }

    return n;
}

/*
 * term -> <var><term> | <var>
 */
Node
term ()
{
    Node s, n = var();

    if (isalpha(look()) || (look() == '!' && look_n(1) != '(')) {
        s = Node('*');
        s.add_child(n);
        s.add_child(term());
        return s;
    }

    return n;
}

/*
 * <value> -> !?[A-Za-z01]
 * <term>  -> !?(<term>) | <value> | <value>+<term> | <value><term>
 * <expr>  -> !?(<expr>) | <term> | <term>+<expr> | <term><expr>
 *
 * I think the extra layer of <term> helps automatically group expressions in
 * a humanly logical format. For example, in the expression: 'a(b) + c + d' I
 * would expect the 'a(b)' to be reduced to 'ab' first and then or'd with the
 * other expressions. Instead what happens is that a is and'd with the or of
 * a, b, c which seems incorrect. The absorption on the final <term> should
 * force 'a(b)' to be 'ab' before doing the or later.
 */

/*
 * expr  -> !?(<expr>) | <term> | <term><expr> | <term>+<expr>
 */
Node
expr ()
{
    bool negate = false;
    Node n, c;

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
        if (negate) {
            c = Node('!');
            c.add_child(n);
            n = c;
        }
    } else {
        n = term();
    }

    if (look() == '+') {
        match('+');
        c = Node('+');
        c.add_child(n);
        c.add_child(expr());
        n = c;
    }
    /* these characters are the start of another expression */
    else if (look() == '(' || look() == '!' || isalnum(look())) {
        c = Node('*');
        c.add_child(n);
        c.add_child(expr());
        n = c;
    }
    /* if this isn't the end of an expression or end of input, error */
    else if (!(look() == ')' || look() == EOF)) {
        fprintf(stderr, "Unexpected '%c'\n", look());
        exit(1);
    }

    return absorb(n);
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
    printf("%c\n", n.value);
    tab++;

    for (auto c : n.children) {
        print_tree(c);
        tab--;
    }

    if (tab == 1) {
        tab = 0;
        printf("-------------------------------\n");
    }
}

/*
 * Recursively negate all terms in the expression tree.
 */
Node
apply_negation (Node N)
{
    return N;
}

/*
 * Distribute all atomic children to the compound children. Non-recursive, so
 * it only distributes the given Node's children once.
 */
Node
distribute (Node N)
{
    /* 
     * Distributing from the left to the right. The left (L) will contain only
     * atomic nodes that will be distributed to all compound right (R)
     * children.
     */
    std::set<Node> L;
    std::set<Node> R;
    char compound_type;

    /* if the Node is outright atomic, we cannot distribute */
    if (isalnum(N.value))
        return N;

    /* 
     * N can never be a negated compound expression because `apply_negation`
     * applies any negations recursively until there are only negated atomic
     * values, e.g. !(a+b) => !a!b
     */
    if (N.value == '!')
        return N;

    /*
     * Because of absorption, all compound statements will be the 'opposite' of
     * the current root's value.
     */
    switch (N.value) {
        case '*': compound_type = '+'; break;
        case '+': compound_type = '*'; break;
    }

    /* gather atomic and compound children into L and R respectively */
    for (auto c : N.children) {
        /* 
         * If value is not the compound type, it is an atomic Node. 
         * Negated nodes ('!') will always be atomic, see above.
         */
        if (c.value != compound_type)
            L.insert(c);
        else
            R.insert(c);
    }

    /* if all children are constant, we do not need to distribute */
    if (L.size() == N.children.size())
        return N;

    N.children.clear();

    /*
     * For each Node r in R create a new node q with r's value. Loop through
     * r's children and create a new Node c with N's value where c's children
     * are L and the current child of r. c is then added as a child of q.
     */
    for (auto r : R) {
        Node q(r.value);
        for (auto rc : r.children) {
            Node child(N.value);
            child.children = L;
            child.children.insert(rc);
            q.children.insert(child);
        }
        N.children.insert(q);
    }

    /* If the is only one child in N, that child becomes the new root. */
    if (N.children.size() == 1)
        return *N.children.begin();

    return N;
}

/*
 * Recursively reduce all reducable terms in the expression tree.
 */
Node
reduce (Node N)
{
    return N;
}

/*
 * Recursively factor out all redundant terms in the expression tree.
 */
Node
factor (Node N)
{
    bool all_atomic;
    Node F;
    std::set<Node> C;
    std::set<Node> R;

    all_atomic = true;

    /* if there are no children, there is nothing to factor */
    if (N.children.empty())
        return N;

    for (auto c : N.children) {
        if (all_atomic && (c.value == '*' || c.value == '+'))
            all_atomic = false;
        C.insert(factor(c));
    }
    
    /* if all the children are atomic we cannot factor anything out */
    if (all_atomic)
        return N;

    /* 
     * F is the first child in C. We use it as the guinea pig. If there are any
     * values in all of the children in C, it will also be in F.
     */
    F = *C.begin();
    /* for each child in F */
    for (auto fc : F.children) {
        bool in_all = true;

        /* loop through each Node in C after the first */
        for (auto it = std::next(C.begin()); it != C.end(); it++) {
            bool in_child = false;
            /* test if fc is in the children of that Node */
            for (auto c : (*it).children)
                if (c == fc)
                    in_child = true;
            /* 
             * if fc isn't in one Node of C, it cannot be in all Nodes of C, so
             * break and try a new fc
             */
            if (!in_child) {
                in_all = false;
                break;
            }
        }

        /* if fc is found in all children of C, add it to the redundant set */
        if (in_all)
            R.insert(fc);
    }

    char compound_type;
    switch (N.value) {
        case '+': 
            compound_type = '+';
            N.value = '*';
            break;
        case '*':
            compound_type = '*';
            N.value = '+';
            break;
    }

    /* 
     * R now contains the redundant terms in the children of C. So, for each
     * Node in C, we erase from its children each redundant term and then add
     * those terms to the factored node.
     * 
     */
    printf("BEFORE\n");
    N.children = C;
    print_tree(N);
    N.children.clear();
    F = Node(compound_type);
    for (auto c : C) {
        for (auto r : R)
            c.children.erase(r);
        for (auto b : c.children)
            F.children.insert(b);
    }
    for (auto r : R)
        N.children.insert(r);
    N.children.insert(F);
    printf("AFTER\n");
    print_tree(N);
    printf("\n\n");
    return N;
}

int
main (int argc, char **argv)
{
    Node simp;
    Node expr;
    
    expr = apply_negation(parse_file("input.txt"));
    print_tree(expr);

    simp = distribute(expr);
    print_tree(simp);

    simp = factor(simp); printf("SIMP:\n");
    print_tree(simp);

    //while (1) {
    //    simp = distribute(expr);
    //    if (expr == simp)
    //        simp = factor(expr);
    //    simp = reduce(simp)
    //    simp = factor(simp)
    //    if (expr == simp)
    //        break;
    //}

    return 0;
}
