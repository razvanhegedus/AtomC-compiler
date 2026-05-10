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

/**
 * exprMulPrime: (MUL|DIV) exprCast exprMulPrime | epsiolon
 */
int exprMulPrime() {
    Token *startTk = crtTk;

    if(consume(MUL) || consume(DIV)) {
        if(exprCast()) {
            if(exprMulPrime()) return 1;
        }
        tkerr(crtTk, "Missing or invalid operand after '*' or '/'");
    }

    crtTk = startTk;
    return 1;

}

/**
 * exprMul: exprCast exprMulPrime 
 */
int exprMul() {
    Token* startTk = crtTk;

    if(exprCast()){
        if(exprMulPrime()) return 1;
    }

    crtTk = startTk;
    return 0;
}

/**
 * exprADDPrime: (ADD|SUB) exprMul exprADDPrime | epsiolon
 */
int exprAddPrime() {
    Token* startTk = crtTk;
    if(consume(ADD) || consume(SUB)) {
        if(exprMul()) {
            if(exprAddPrime()) return 1;
        }
        tkerr(crtTk, "Missing or invalid expression after addition/subtraction operator");
    }

    crtTk = startTk;
    return 1;
}


/**
 * exprAdd: exprMul exprADDPrime 
 */
int exprAdd() {
    Token* startTk = crtTk;

    if(exprMul()){
        if(exprAddPrime()) return 1;
    }

    crtTk = startTk;
    return 0;
}

/**
 * exprRelPrime: (LESS|LESSEQ|GREATER|GREATEREQ) exprADD exprRelrime | epsiolon
 */
int exprRelPrime() {
    Token* startTk = crtTk;
    if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
        if(exprAdd()) {
            if(exprRelPrime()) return 1;
        }
        tkerr(crtTk, "Expected add expression after relational operator");
    }

    //eps
    crtTk = startTk;
    return 1;
}

/**
 * exprRel: exprAdd exprRelPrime 
 */
int exprRel(){
    Token* startTk = crtTk;

    if(exprAdd()){
        if(exprRelPrime()) return 1;
    }
    crtTk = startTk;
    return 0;
}

/**
 * exprEqPrime: (EQUAL|NOTEQ) exprRel exprEqPrime | epsilon
 */
int exprEqPrime() {
    Token *startTk = crtTk;

    if (consume(EQUAL) || consume(NOTEQ)) {
        if (exprRel()) {
            if (exprEqPrime()) return 1;
        } else {
            tkerr(crtTk, "Missing expression after equality operator");
        }
    }

    crtTk = startTk; // Epsilon path
    return 1;
}

/**
 * exprEq: exprRel exprEqPrime
 */
int exprEq() {
    Token *startTk = crtTk;

    if (exprRel()) {
        if (exprEqPrime()) return 1;
    }

    crtTk = startTk;
    return 0;
}


/**
 * exprAndPrime: AND exprEq exprAndPrime | epsilon
 */
int exprAndPrime() {
    Token *startTk = crtTk;

    if (consume(AND)) {
        if (exprEq()) {
            if (exprAndPrime()) return 1;
        } else {
            tkerr(crtTk, "Missing expression after '&&'");
        }
    }

    crtTk = startTk;
    return 1;
}

/**
 * exprAnd: exprEq exprAndPrime
 */
int exprAnd() {
    Token *startTk = crtTk;

    if (exprEq()) {
        if (exprAndPrime()) return 1;
    }

    crtTk = startTk;
    return 0;
}


/**
 * exprOrPrime: OR exprAnd exprOrPrime | epsilon
 */
int exprOrPrime() {
    Token *startTk = crtTk;

    if (consume(OR)) {
        // COMMITMENT POINT: Found ||
        if (exprAnd()) {
            if (exprOrPrime()) return 1;
        } else {
            tkerr(crtTk, "Missing expression after '||'");
        }
    }

    crtTk = startTk;
    return 1;
}

/**
 * exprOr: exprAnd exprOrPrime
 */
int exprOr() {
    Token *startTk = crtTk;

    if (exprAnd()) {
        if (exprOrPrime()) return 1;
    }

    crtTk = startTk;
    return 0;
}


/**
 * exprAssign: exprUnary ASSIGN exprAssign | exprOr
 */
int exprAssign() {
    Token *startTk = crtTk;

    // We try to match a unary expression followed by an assignment
    if (exprUnary()) {
        if (consume(ASSIGN)) {
            // COMMITMENT POINT: Found '='
            if (exprAssign()) return 1;
            else tkerr(crtTk, "Missing expression after '='");
        }
    }

    // Backtrack and try the second alternative: exprOr
    crtTk = startTk;
    if (exprOr()) return 1;

    crtTk = startTk;
    return 0;
}

/**
 * expr: exprAssign
 */
int expr() {
    // Top-level entry doesn't usually call tkerr; 
    // it returns 0 to let the statement parser handle the error.
    if (exprAssign()) return 1;
    return 0;
}


/**
 * stmCompund: LACC (varDef|stm)* RACC
 */
int stmCompound() {
    Token *startTk = crtTk;
    if(consume(LACC)){
        while(1) {
            if(varDef()){}
            else if(stm()){}
            else {
                break;
            }
        }
        if(consume(RACC)) {return 1;}
        else {
            tkerr(crtTk, "Expected variable definition, statement, or '}'");
        }
    }

    crtTk = startTk;
    return 0;
}


/**
 * stm: stmCompound
 *    | IF LPAR expr RPAR stm (ELSE stm)?
 *    | WHILE LPAR expr RPAR stm
 *    | FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
 *    | BREAK SEMICOLON
 *    | RETURN expr? SEMICOLON
 *    | expr? SEMICOLON
 */
int stm() {
    Token *startTk = crtTk;

    if (stmCompound()) return 1;

    crtTk = startTk;
    if (consume(IF)) {
        if (!consume(LPAR)) tkerr(crtTk, "Expected '(' after 'if'");
        if (!expr()) tkerr(crtTk, "Invalid condition in 'if'");
        if (!consume(RPAR)) tkerr(crtTk, "Expected ')' after 'if' condition");
        if (!stm()) tkerr(crtTk, "Missing statement after 'if'");
        if (consume(ELSE)) {
            if (!stm()) tkerr(crtTk, "Missing statement after 'else'");
        }
        return 1;
    }

    crtTk = startTk;
    if (consume(WHILE)) {
        if (!consume(LPAR)) tkerr(crtTk, "Expected '(' after 'while'");
        if (!expr()) tkerr(crtTk, "Invalid condition in 'while'");
        if (!consume(RPAR)) tkerr(crtTk, "Expected ')' after 'while' condition");
        if (!stm()) tkerr(crtTk, "Missing statement body for 'while' loop");
        return 1;
    }

    crtTk = startTk;
    if (consume(FOR)) {
        if (!consume(LPAR)) tkerr(crtTk, "Expected '(' after 'for'");
        expr(); // Optional init
        if (!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after for-init");
        expr(); // Optional condition
        if (!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after for-condition");
        expr(); // Optional step
        if (!consume(RPAR)) tkerr(crtTk, "Expected ')' after for-header");
        if (!stm()) tkerr(crtTk, "Missing statement body for 'for' loop");
        return 1;
    }

    crtTk = startTk;
    if (consume(BREAK)) {
        if (consume(SEMICOLON)) return 1;
        else tkerr(crtTk, "Missing ';' after 'break'");
    }

    crtTk = startTk;
    if (consume(RETURN)) {
        expr(); // Optional return value
        if (consume(SEMICOLON)) return 1;
        else tkerr(crtTk, "Missing ';' after 'return'");
    }

    crtTk = startTk;
    if (expr()) {
        if (consume(SEMICOLON)) return 1;
        else tkerr(crtTk, "Missing ';' after expression");
    } else if (consume(SEMICOLON)) {
        return 1; 
    }

    crtTk = startTk;
    return 0;
}


/**
 * fnParam: typeBase ID arrayDecl?
 */
int fnParam() {
    Token *startTk = crtTk;

    if (typeBase()) {
        // COMMITMENT POINT: Once we have a type, we REQUIRE an ID.
        if (consume(ID)) {
            // arrayDecl is optional (?), so we just call it.
            // It will consume [CT_INT?] if it's there, or do nothing.
            arrayDecl(); 
            return 1;
        } else {
            tkerr(crtTk, "Expected identifier after type in function parameter");
        }
    }

    crtTk = startTk;
    return 0;
}


/**
 * fnDef : (typeBase | VOID) ID LPAR (fnParam (COMMA fnParam)* )? RPAR stmCompound
 */
int fnDef() {
    Token *startTk = crtTk;

    // 1. Try to match the return type: typeBase OR VOID
    int foundType = 0;
    if (typeBase()) {
        foundType = 1;
    } else {
        crtTk = startTk; // Reset to try VOID
        if (consume(VOID)) {
            foundType = 1;
        }
    }

    if (foundType) {
        if (consume(ID)) {
            if (consume(LPAR)) {
                if (fnParam()) {
                    while (consume(COMMA)) {
                        if (!fnParam()) tkerr(crtTk, "Expected parameter after ','");
                    }
                }

                if (!consume(RPAR)) tkerr(crtTk, "Missing ')' in function signature");
                if (!stmCompound()) tkerr(crtTk, "Missing function body");
                
                return 1; 
            }
        }
    }

    crtTk = startTk;
    return 0;
}


/**
 * arrayDecl : LBRACKET CT_INT? RBRACKET
 */
int arrayDecl(){
    Token *startTk = crtTk;

    if(consume(LBRACKET)) {
        consume(CT_INT);
        if(!consume(RBRACKET)) tkerr(crtTk, "Missing ] in array declaration");
        return 1;
    }

    crtTk = startTk;
    return 0;
}


/**
 * typeBase : INT | DOUBLE | CHAR | STRUCT ID
 */
int typeBase() {
    Token *startTk = crtTk;

    if (consume(INT)) return 1;
    if (consume(DOUBLE)) return 1;
    if (consume(CHAR)) return 1;
    
    if (consume(STRUCT)) {
        if (consume(ID)) {
            return 1;
        } else {
            tkerr(crtTk, "Expected identifier after 'struct'");
        }
    }

    crtTk = startTk;
    return 0;
}


/**
 * varDef: typeBase ID arrayDecl? SEMICOLON
 */
int varDef() {
    Token *startTk = crtTk;

    if (typeBase()) {
        if (consume(ID)) {
            // arrayDecl() is optional (?). It returns 0 or 1, 
            // but we don't error if it's 0.
            arrayDecl(); 

            if (consume(SEMICOLON)) {
                return 1;
            } else {
                tkerr(crtTk, "Missing ';' after variable declaration");
            }
        } else {
            crtTk = startTk;
            return 0;
        }
    }

    crtTk = startTk;
    return 0;
}

/**
 * structDef: STRUCT ID LACC varDef* RACC SEMICOLON
 */
int structDef() {
    Token *startTk = crtTk;

    if (consume(STRUCT)) {
        if (consume(ID)) {
            if (consume(LACC)) {
                // COMMITMENT: We are defining a struct body
                while (varDef()) {
                    // keep consuming variable definitions
                }
                
                if (!consume(RACC)) tkerr(crtTk, "Missing '}' in struct definition");
                if (!consume(SEMICOLON)) tkerr(crtTk, "Missing ';' after struct definition");
                return 1;
            }
        }
    }

    crtTk = startTk;
    return 0;
}

/**
 * unit: (structDef | fnDef | varDef)* END
 */
int unit() {

    while (1) {
        Token *startTk = crtTk;

        if (structDef()) {
            continue; 
        }

        crtTk = startTk;
        if (fnDef()) {
            continue; 
        }

        crtTk = startTk;
        if (varDef()) {
            continue;
        }

        crtTk = startTk;
        break;
    }

    if (consume(END)) {
        return 1;
    } else {
        tkerr(crtTk, "Invalid global declaration or syntax error at top level");
    }

    return 0; 
}