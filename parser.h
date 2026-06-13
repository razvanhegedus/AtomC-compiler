#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "DA.h"
#include "TA.h"

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

int expr(Ret *r);

int exprAssign(Ret *r);
int exprOr(Ret *r);
int exprOrPrime(Ret *r);
int exprAnd(Ret* r);
int exprAndPrime(Ret* r);

int exprEq(Ret* r);
int exprEqPrime(Ret* r);
int exprRel(Ret* r);
int exprRelPrime(Ret* r);

int exprAdd(Ret* r);
int exprAddPrime(Ret* r);
int exprMul(Ret* r);
int exprMulPrime(Ret* r);

int exprCast(Ret* r);
int exprUnary(Ret* r);
int exprPostfix(Ret* r);
int exprPostfixPrime(Ret* r);
int exprPrimary(Ret* r);

#endif