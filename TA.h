#ifndef TA_H
#define TA_H

#include "DA.h"

typedef union{
    long int i;
    double d;
    const char *str;
} CtVal;

typedef struct{
    Type type;
    int lval;
    int ct;
    CtVal ctVal;
} Ret;

Type createType(int typeBase, int nElements);

int convTo(Type *src, Type *dst);

int canBeScalar(Ret *r);

int arithTypeTo(Type *s1, Type *s2, Type *dst);

#endif