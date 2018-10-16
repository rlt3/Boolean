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

int
main (int argc, char **argv)
{
    Node expr = parse_file("input.txt");
    print_tree(expr);
    return 0;
}
