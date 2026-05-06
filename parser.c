#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

int consume(int code)
{
    if(crtTk->code==code)
    {
        consumedTk=crtTk;
        crtTk=crtTk->next;
        return 1;
    }
    return 0;
}

/**
 * exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
 *            | CT_INT | CT_REAL | CT_CHAR | CT_STRING
 *            | LPAR expr RPAR
 */
int exprPrimary(){
    Token* startTk = crtTk;

    // Branch 1: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
    if (consume(ID)) 
    {
        if (consume(LPAR)) 
        {
            if (expr()) 
            {
                while (consume(COMMA)) 
                {
                    if (!expr()) tkerr(crtTk, "Expected expression after ','");
                }
            }
            if (!consume(RPAR)) tkerr(crtTk, "Missing ')' in function call");
        }
        return 1;
    }

    if (consume(CT_INT) || consume(CT_REAL) || consume(CT_CHAR) || consume(CT_STRING)) {
        return 1;
    }

    if (consume(LPAR)) {
        if (!expr()) tkerr(crtTk, "Expected expression after '('");
        if (!consume(RPAR)) tkerr(crtTk, "Missing ')' after expression");
        return 1;
    }

    crtTk = startTk; 
    return 0;
}


/**
 * exprPostfixPrime: LBRACKET expr RBRACKET exprPostfixPrime
 *                 | DOT ID exprPostfixPrime
 *                 | epsilon
 */
int exprPostfixPrime(){
    Token *startTk = crtTk;

    // LBRACKET expr RBRACKET exprPostfixPrime
    if (consume(LBRACKET)) {
        if (expr()) {
            if (consume(RBRACKET)) 
            {
                if (exprPostfixPrime()) return 1;
            } 
            else tkerr(crtTk, "Missing ']' ");
        } 
        else tkerr(crtTk, "Expected expression inside '[]");
    }

    // DOT ID exprPostfixPrime
    if (consume(DOT)) {
        if (consume(ID)) {
            if (exprPostfixPrime()) return 1;
        } else tkerr(crtTk, "Expected identifier after '.'");
    }

    // epsilon
    crtTk = startTk; 
    return 1;
}


//exprPrimary exprPostfixPrime
int exprPostfix(){
    Token *startTk = crtTk;

    if(exprPrimary()){
        if(exprPostfixPrime()) return 1;
    }

    crtTk = startTk;
    return 0;
}


    /**
 * exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
 */
int exprUnary() {
    Token *startTk = crtTk;

    if (consume(SUB) || consume(NOT)) {

        if (exprUnary()) {
            return 1;
        } else {
            tkerr(crtTk, "Missing unary expression after operator");
        }
    }

    if (exprPostfix()) {
        return 1;
    }

    crtTk = startTk; 
    return 0;
}

/**
 * exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
 */
int exprCast() {
    Token *startTk = crtTk;

    // Possibility 1: ( typeBase arrayDecl? ) exprCast
    if (consume(LPAR)) {
        if (typeBase()) {
            arrayDecl(); // Optional, so we don't check return value for error
            if (consume(RPAR)) {
                if (exprCast()) {
                    return 1;
                } else tkerr(crtTk, "Missing expression after cast");
            } else tkerr(crtTk, "Missing ')' after type in cast");
        }

        crtTk = startTk;
    }

    // Possibility 2: exprUnary
    if (exprUnary()) {
        return 1;
    }

    crtTk = startTk;
    return 0;
}
