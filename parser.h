#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "DA.h"

int consume(int code);

int stm();
int stmCompound(int newDomain);

int fnDef();
int fnParam();
int varDef();
int structDef();
int typeBase(Type *t);
int arrayDecl(Type *t);

int unit();

int expr();

int exprAssign();
int exprOr();
int exprOrPrime();
int exprAnd();
int exprAndPrime();

int exprEq();
int exprEqPrime();
int exprRel();
int exprRelPrime();

int exprAdd();
int exprAddPrime();
int exprMul();
int exprMulPrime();

int exprCast();
int exprUnary();
int exprPostfix();
int exprPostfixPrime();
int exprPrimary();

#endif