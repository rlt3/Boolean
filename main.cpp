#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <cerrno>

/*
 * Var    -> [A-Za-z]
 * Term   -> VarTerm
 *         | Var
 * Clause -> Term
 *         | Term + Clause
 *         | (Clause)
 *         | !Clause
 * Expr   -> Clause
 *         | Clause + Expr
 *         | ClauseExpr
 */

static FILE *INPUT_FILE = NULL;
static int LOOKAHEAD;

int
look ()
{
    return LOOKAHEAD;
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

char
var ()
{
    int var = look();
    if (!isalpha(var)) {
        fprintf(stderr, "Expected character instead got '%c'\n", look());
        exit(1);
    }
    match(var);
    return var;
}

/*
 * Term -> VarTerm | Var
 */
void
term ()
{
    var();
    if (isalpha(look()))
        term();
}

/*
 * Clause -> (Clause) | !Clause | Term + Clause | Term
 */
void
clause ()
{
    if (look() == '(') {
        match('(');
        clause();
        match(')');
    }
    else if (look() == '!') {
        match('!');
        clause();
    } else {
        term();
        if (look() == '+') {
            match('+');
            clause();
        }
    }
}

/* 
 * Expr -> Clause | Clause + Expr | ClauseExpr
 */
void
expr ()
{
    clause();
    if (look() == '+') {
        match('+');
        expr();
    } else if (look() != EOF) {
        expr();
    }
    else {
        printf("\n");
    }
}

void
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
    expr();
    fclose(INPUT_FILE);
}

int
main (int argc, char **argv)
{
    parse_file("input.txt");
    return 0;
}
