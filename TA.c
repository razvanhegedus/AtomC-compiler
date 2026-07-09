#include "TA.h"
#include <stddef.h>

Type createType(int typeBase, int nElements){
    Type t;
    t.typeBase = typeBase;
    t.nElements = nElements;
    t.s = NULL;
    return t;
}

int canBeScalar(Ret *r){
    return r->type.nElements < 0 &&
           r->type.typeBase != TB_STRUCT;
}

int convTo(Type *src, Type *dst){
    if(src->nElements > -1){
        if(dst->nElements > -1){
            if(src->typeBase != dst->typeBase)
                return 0;
        }else{
            return 0;
        }
    }
    else{
        if(dst->nElements > -1)
            return 0;
    }

    switch(src->typeBase){
        case TB_CHAR:
        case TB_INT:
        case TB_DOUBLE:
            switch(dst->typeBase){
                case TB_CHAR:
                case TB_INT:
                case TB_DOUBLE:
                    return 1;
            }
            break;
        case TB_STRUCT:
            if(dst->typeBase == TB_STRUCT){
                if(src->s == dst->s)
                    return 1;
            }
            break;
    }
    return 0;
}

int arithTypeTo(Type *s1, Type *s2, Type *dst){
    if(s1->nElements >= 0 || s2->nElements >= 0)
        return 0;

    if(s1->typeBase == TB_STRUCT ||
       s2->typeBase == TB_STRUCT)
        return 0;

    if(s1->typeBase == TB_DOUBLE ||
       s2->typeBase == TB_DOUBLE){
        *dst = createType(TB_DOUBLE, -1);
    }
    else if(s1->typeBase == TB_INT ||
            s2->typeBase == TB_INT){
        *dst = createType(TB_INT, -1);
    }
    else{
        *dst = createType(TB_CHAR, -1);
    }
    return 1;
}