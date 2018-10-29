#ifndef PARSE_HPP
#define PARSE_HPP

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <cerrno>
#include <cassert>
#include "Node.hpp"

static std::stringstream INPUT;
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
    /* save all characters read including whitespace */
    for (int i = 0; i < count; i++) {
        do {
            read.push_back(INPUT.get());
        } while (isspace(read.back()));
    }
    /* and put them back */
    for (int i = (int) read.size() - 1; i >= 0; i--) {
        INPUT.putback(read[i]);
    }
    return read.back();
}

int
next ()
{
    do {
        LOOKAHEAD = INPUT.get();
    } while (isspace(LOOKAHEAD));
    return LOOKAHEAD;
}

void
unexpected (char c)
{
    fprintf(stderr, "Unexpected character '%c'\n", c);
    exit(1);
}

void
match (char c)
{
    if (look() != c) {
        fprintf(stderr, "Expected character '%c' got '%c'\n", c, look());
        exit(1);
    }
    next();
}

Node var ();
Node sub ();
Node negate ();
Node prod ();
Node expr ();

bool
is_var ()
{
    if (look() == '!')
        return isalpha(look_n(1));
    return isalpha(look());
}

/*
 * atom = [01]
 */
Node
atom ()
{
    int c = look();
    if (!(c == '0' || c == '1')) {
        fprintf(stderr, "Expected atomic 0 or 1 instead got '%c'\n", c);
        exit(1);
    }
    match(c);
    return Node(c);
}

/*
 * var = !?[A-Za-z]
 */
Node
var ()
{
    std::string v;

    if (look() == '!') {
        match('!');
        v += '!';
    }
    if (!is_var()) {
        fprintf(stderr, "Expected character instead got '%c'\n", look());
        exit(1);
    }
    v += look();
    match(look());

    return Node(v);
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
    N.add_reduction(expr());
    match(')');
    assert(N.children.size() == 1);
    return N;
}

/*
 * <prod> = <negate><prod> | <sub><prod> | <var><prod> | <atom><prod> | E
 */
Node
prod ()
{
    Node N('*');

    while (1) {
        if (look() == '!' && look_n(1) == '(')
            N.add_child(negate());
        else if (look() == '(')
            N.add_reduction(sub());
        else if (is_var())
            N.add_child(var());
        else if (look() == '0' || look() == '1')
            N.add_child(atom());
        else
            unexpected(look());

        /* a prod cannot begin with any of these */
        if (look() == '+' || look() == ')' || look() == EOF)
            break;
    }

    return N;
}

/*
 * <expr> = <prod><expr> | <prod>+<expr> | E
 */
Node
expr ()
{
    /* use a sentinel value not in language to mark 'unset' */
    Node N("@");

    do {
        N.add_reduction(prod());

        /* 
         * Anytime we find an disjunction ('+') then we make sure N has type
         * '+'. Note that a disjunction can overwrite a conjunction but not
         * vice-versa.
         */
        if (look() == '+') {
            N.type = '+';
            match('+');
        } else if (N.type == "@") {
            N.type = '*';
        }

        /* these tokens end an expr or a sub expr */
        if (look() == EOF || look() == ')')
            break;
    } while (look() != EOF);

    /* 
     * if we have an expression of a single child, e.g. a or (b) or ((c+d))
     * then make that child the root and return it
     */
    if (N.children.size() == 1)
        N = *N.children.begin();

    return N;
}

Node
parse_input ()
{
    /* set look to whitespace because 'next' loops until no whitespace */
    LOOKAHEAD = ' ';
    next();
    return expr();
}

void
set_input (std::string input)
{
    INPUT.clear();
    INPUT.str(input);
}

#endif
