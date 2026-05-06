#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

int consume(int code);

int unit();

int structDef();
int varDef();
int typeBase();
int arrayDecl();
int fnDef();
int fnParam();
int stm();
int stmCompound();

int expr();
int exprAssign();
int exprPrimary();

#endif