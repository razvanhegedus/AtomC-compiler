#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "DA.h"
#include "TA.h"
#include "vm.h"


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


Symbols* symTable;
int crtDepth = 0;
Symbol* owner;
int crtGlobalMemorySize = 0; // Tracks total bytes allocated in the global segment




int allocInGlobalMemory(int size) {
    int currentOffset = crtGlobalMemorySize;
    crtGlobalMemorySize += size;
    return currentOffset;
}

void initSymbols(Symbols *symbols) {
    symbols->begin = NULL;  // The beginning of the symbols 
    symbols->end = NULL;    // The position after the last symbol 
    symbols->after = NULL;  // The position after the allocated space 
}


Symbol *newSymbol(const char *name, int kind) {
    Symbol *s;
    SAFEALLOC(s, Symbol); 
    s->name = name; 
    s->kind = kind;
    s->depth = crtDepth; // Current nesting depth 


    if (kind == SK_PARAM) {
        s->mem = MEM_ARG; 
    } else if (crtDepth == 0) {
        s->mem = MEM_GLOBAL; 
    } else {
        s->mem = MEM_LOCAL; 
    }
    if (kind == SK_STRUCT) {
        initSymbols(&s->structMembers); 
    } else if (kind == SK_FN) {
        initSymbols(&s->fn.params); 
        initSymbols(&s->fn.locals); 
    }
    return s;
}


Symbol *addSymbolToDomain(Symbols *list, Symbol *s) {
    if (list->end == list->after) { // Create more room if full 
        int count = list->end - list->begin;
        int n = count * 2;
        if (n == 0) n = 1; // Initial case 
        
        list->begin = (Symbol **)realloc(list->begin, n * sizeof(Symbol *));
        if (list->begin == NULL) err("not enough memory for symbol table"); 
        
        list->end = list->begin + count;
        list->after = list->begin + n;
    }
    *list->end++ = s; // Add the symbol to the list 
    return s;
}


Symbol *findSymbolInDomain(Symbols *list, const char *name) {
    if (list == NULL || list->begin == list->end) return NULL;
    
    int count = list->end - list->begin;
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(list->begin[i]->name, name) == 0) {
            return list->begin[i]; // Return the most recent definition
        }
    }
    return NULL; 
}

Symbol *findSymbol(const char *name) {
    if (symTable == NULL || symTable->begin == symTable->end) return NULL;
    
    int count = symTable->end - symTable->begin;
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(symTable->begin[i]->name, name) == 0) {
            return symTable->begin[i]; // Return the most recent definition 
        }
    }
    return NULL; 
}

void pushDomain() {
    crtDepth++;
}


void dropDomain() {
    while (symTable->end > symTable->begin && (*(symTable->end - 1))->depth == crtDepth) {
        symTable->end--;
    }
    
    crtDepth--;
}


void addSymbolToList(Symbols *list, Symbol *s) {
    addSymbolToDomain(list, s);
}

int symbolsLen(Symbols *list) {
    return list->end - list->begin;
}

Symbol *dupSymbol(Symbol *s) {
    Symbol *d;
    SAFEALLOC(d, Symbol);
    *d = *s; // Copy all fields (shallow copy)
    return d;
}

int typeSize(Type *t) {
    int baseSize = 4; // Default size for primitives (int, double, char, pointers)
    
    if (t->typeBase == TB_STRUCT && t->s != NULL) {
        baseSize = 0;
        Symbols *members = &t->s->structMembers;
        int count = members->end - members->begin;
        for (int i = 0; i < count; i++) {
            baseSize += typeSize(&members->begin[i]->type);
        }
    }
    
    if (t->nElements >= 0) {
        if (t->nElements == 0) {
            return 4;
        }
        return t->nElements * baseSize;
    }
    
    return baseSize;
}



const char* kindToString(int kind) {
    switch (kind) {
        case SK_VAR:    return "VAR";
        case SK_FN:     return "FN";
        case SK_STRUCT: return "STRUCT";
        case SK_PARAM:  return "PARAM";
        default:        return "UNKNOWN";
    }
}

const char* memToString(int mem) {
    switch (mem) {
        case MEM_GLOBAL: return "GLOBAL";
        case MEM_ARG:    return "ARG";
        case MEM_LOCAL:  return "LOCAL";
        default:         return "NONE";
    }
}

void printType(Type *t) {
    switch (t->typeBase) {
        case TB_INT:    printf("int"); break;
        case TB_DOUBLE: printf("double"); break;
        case TB_CHAR:   printf("char"); break;
        case TB_VOID:   printf("void"); break;
        case TB_STRUCT: 
            if (t->s) printf("struct %s", t->s->name);
            else printf("struct unknown");
            break;
        default: printf("unknown"); break;
    }
    
    if (t->nElements == 0) {
        printf("[]");
    } else if (t->nElements > 0) {
        printf("[%d]", t->nElements);
    }
}

Symbol *requireSymbol(Symbols *list, const char *name) {
    Symbol *s = findSymbolInDomain(list, name);
    
    if (s == NULL) {
        err("Eroare critica: Simbolul '%s' lipseste din tabela de simboluri!", name);
    }
    
    return s;
}

void printSymbol(Symbol *s, int indent) {
    for (int i = 0; i < indent; i++) printf("    ");
    
    printf("%-15s | cls: %-6s | mem: %-7s | type: ", 
           s->name, kindToString(s->kind), memToString(s->mem));
    printType(&s->type);
    printf("\n");
    
    // Dacă este structură, afișăm recursiv membrii acesteia
    if (s->kind == SK_STRUCT) {
        int mCount = s->structMembers.end - s->structMembers.begin;
        for (int i = 0; i < mCount; i++) {
            printSymbol(s->structMembers.begin[i], indent + 1);
        }
    } 
    // Dacă este funcție, afișăm parametrii și variabilele locale
    else if (s->kind == SK_FN) {
        int pCount = s->fn.params.end - s->fn.params.begin;
        for (int i = 0; i < pCount; i++) {
            printSymbol(s->fn.params.begin[i], indent + 1);
        }
        int lCount = s->fn.locals.end - s->fn.locals.begin;
        for (int i = 0; i < lCount; i++) {
            printSymbol(s->fn.locals.begin[i], indent + 1);
        }
    }
}

void printSymbolTable() {
    if (!symTable || symTable->begin == symTable->end) {
        printf("\nTabela de simboluri este goala.\n");
        return;
    }
    
    printf("\n==================== Symbols Table ====================\n");
    int count = symTable->end - symTable->begin;
    for (int i = 0; i < count; i++) {
        printSymbol(symTable->begin[i], 0);
    }
    printf("=============================================================\n");
}



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
int exprPrimary(Ret *r){
    Token *startTk = crtTk;
    Token *tkName = crtTk;

    if(consume(ID)){
        Symbol *s = findSymbol(tkName->text);
        if(!s) tkerr(crtTk, "undefined id: %s", tkName->text);

        if(consume(LPAR)){
            /* function call */
            if(s->kind != SK_FN) tkerr(crtTk, "only a function can be called");

            int nParams = symbolsLen(&s->fn.params);
            int curParam = 0;
            Ret rArg;

            if(expr(&rArg)){
                if(curParam >= nParams) tkerr(crtTk, "too many arguments in function call");
                Symbol *param = s->fn.params.begin[curParam];
                if(!convTo(&rArg.type, &param->type))
                    tkerr(crtTk, "in call, cannot convert the argument type to the parameter type");
                curParam++;

                while(consume(COMMA)){
                    if(!expr(&rArg)) tkerr(crtTk, "Expected expression after ','");
                    if(curParam >= nParams) tkerr(crtTk, "too many arguments in function call");
                    param = s->fn.params.begin[curParam];
                    if(!convTo(&rArg.type, &param->type))
                        tkerr(crtTk, "in call, cannot convert the argument type to the parameter type");
                    curParam++;
                }
            }
            if(!consume(RPAR)) tkerr(crtTk, "Missing ')' in function call");
            if(curParam < nParams) tkerr(crtTk, "too few arguments in function call");

            *r = (Ret){ s->type, 0, 1, {.i=0} };
        } else {
            /* plain id */
            if(s->kind == SK_FN) tkerr(crtTk, "a function can only be called");
            *r = (Ret){ s->type, 1, s->type.nElements >= 0, {.i=0} };
        }
        return 1;
    }

    if(consume(CT_INT)){    *r = (Ret){ {TB_INT,    NULL, -1}, 0, 1, {.i=0} }; return 1; }
    if(consume(CT_REAL)){   *r = (Ret){ {TB_DOUBLE, NULL, -1}, 0, 1, {.i=0} }; return 1; }
    if(consume(CT_CHAR)){   *r = (Ret){ {TB_CHAR,   NULL, -1}, 0, 1, {.i=0} }; return 1; }
    if(consume(CT_STRING)){ *r = (Ret){ {TB_CHAR,   NULL,  0}, 0, 1, {.i=0} }; return 1; }

    if(consume(LPAR)){
        if(!expr(r)) tkerr(crtTk, "Expected expression after '('");
        if(!consume(RPAR)) tkerr(crtTk, "Missing ')' after expression");
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
int exprPostfixPrime(Ret *r){
    if(consume(LBRACKET)){
        Ret idx;
        if(!expr(&idx)) tkerr(crtTk, "Expected expression inside '[]'");
        if(!consume(RBRACKET)) tkerr(crtTk, "Missing ']'");

        if(r->type.nElements < 0) tkerr(crtTk, "only an array can be indexed");
        Type tInt = { TB_INT, NULL, -1 };
        if(!convTo(&idx.type, &tInt)) tkerr(crtTk, "the index is not convertible to int");
        r->type.nElements = -1;
        r->lval = 1;
        r->ct = 0;

        return exprPostfixPrime(r);
    }
    if(consume(DOT)){
        Token *tkName = crtTk;
        if(!consume(ID)) tkerr(crtTk, "Expected identifier after '.'");

        if(r->type.typeBase != TB_STRUCT)
            tkerr(crtTk, "a field can only be selected from a struct");
        Symbol *m = findSymbolInDomain(&r->type.s->structMembers, tkName->text);
        if(!m) tkerr(crtTk, "the structure %s does not have a field %s",
                     r->type.s->name, tkName->text);
        *r = (Ret){ m->type, 1, m->type.nElements >= 0, {.i=0} };

        return exprPostfixPrime(r);
    }
    return 1; /* epsilon */
}


//exprPrimary exprPostfixPrime
int exprPostfix(Ret *r){
    Token *startTk = crtTk;
    if(exprPrimary(r)){
        if(exprPostfixPrime(r)) return 1;
    }
    crtTk = startTk;
    return 0;
}

/**
 * exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
 */
int exprUnary(Ret *r){
    Token *startTk = crtTk;
    if(consume(SUB) || consume(NOT)){
        if(!exprUnary(r)) tkerr(crtTk, "Missing unary expression after operator");
        if(!canBeScalar(r)) tkerr(crtTk, "unary - must have a scalar operand");
        r->lval = 0;
        r->ct = 1;
        return 1;
    }
    if(exprPostfix(r)) return 1;
    crtTk = startTk;
    return 0;
}

/**
 * exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
 */
int exprCast(Ret *r){
    Token *startTk = crtTk;
    if(consume(LPAR)){
        Type t; Ret op;
        if(typeBase(&t)){
            arrayDecl(&t);
            if(!consume(RPAR)) tkerr(crtTk, "Missing ')' after type in cast");
            if(!exprCast(&op)) tkerr(crtTk, "Missing expression after cast");

            if(t.typeBase == TB_STRUCT) tkerr(crtTk, "cannot convert to a struct type");
            if(op.type.typeBase == TB_STRUCT) tkerr(crtTk, "cannot convert a struct");
            if(op.type.nElements >= 0 && t.nElements < 0)
                tkerr(crtTk, "an array can be converted only to another array");
            if(op.type.nElements < 0 && t.nElements >= 0)
                tkerr(crtTk, "a scalar can be converted only to another scalar");

            *r = (Ret){ t, 0, 1, {.i=0} };
            return 1;
        }
        crtTk = startTk;
    }
    if(exprUnary(r)) return 1;
    crtTk = startTk;
    return 0;
}

/**
 * exprMulPrime: (MUL|DIV) exprCast exprMulPrime | epsiolon
 */
int exprMulPrime(Ret *r){
    if(consume(MUL) || consume(DIV)){
        Ret right;
        if(!exprCast(&right)) tkerr(crtTk, "Missing or invalid operand after '*' or '/'");
        Type tDst;
        if(!arithTypeTo(&r->type, &right.type, &tDst))
            tkerr(crtTk, "invalid operand type for * or /");
        *r = (Ret){ tDst, 0, 1, {.i=0} };
        return exprMulPrime(r);
    }
    return 1;
}

/**
 * exprMul: exprCast exprMulPrime 
 */
int exprMul(Ret *r){
    Token *startTk = crtTk;
    if(exprCast(r)){ if(exprMulPrime(r)) return 1; }
    crtTk = startTk;
    return 0;
}

/**
 * exprADDPrime: (ADD|SUB) exprMul exprADDPrime | epsiolon
 */
int exprAddPrime(Ret *r){
    if(consume(ADD) || consume(SUB)){
        Ret right;
        if(!exprMul(&right)) tkerr(crtTk, "Missing or invalid expression after addition/subtraction operator");
        Type tDst;
        if(!arithTypeTo(&r->type, &right.type, &tDst))
            tkerr(crtTk, "invalid operand type for + or -");
        *r = (Ret){ tDst, 0, 1, {.i=0} };
        return exprAddPrime(r);
    }
    return 1;
}
/**
 * exprAdd: exprMul exprADDPrime 
 */
int exprAdd(Ret *r){
    Token *startTk = crtTk;
    if(exprMul(r)){ if(exprAddPrime(r)) return 1; }
    crtTk = startTk;
    return 0;
}



/**
 * exprRelPrime: (LESS|LESSEQ|GREATER|GREATEREQ) exprADD exprRelrime | epsiolon
 */
int exprRelPrime(Ret *r){
    if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
        Ret right;
        if(!exprAdd(&right)) tkerr(crtTk, "Expected add expression after relational operator");
        Type tDst;
        if(!arithTypeTo(&r->type, &right.type, &tDst))
            tkerr(crtTk, "invalid operand type for <, <=, >, >=");
        *r = (Ret){ {TB_INT, NULL, -1}, 0, 1, {.i=0} };
        return exprRelPrime(r);
    }
    return 1;
}

/**
 * exprRel: exprAdd exprRelPrime 
 */
int exprRel(Ret *r){
    Token *startTk = crtTk;
    if(exprAdd(r)){ if(exprRelPrime(r)) return 1; }
    crtTk = startTk;
    return 0;
}


/**
 * exprEqPrime: (EQUAL|NOTEQ) exprRel exprEqPrime | epsilon
 */
int exprEqPrime(Ret *r){
    if(consume(EQUAL) || consume(NOTEQ)){
        Ret right;
        if(!exprRel(&right)) tkerr(crtTk, "Missing expression after equality operator");
        Type tDst;
        if(!arithTypeTo(&r->type, &right.type, &tDst))
            tkerr(crtTk, "invalid operand type for == or !=");
        *r = (Ret){ {TB_INT, NULL, -1}, 0, 1, {.i=0} };
        return exprEqPrime(r);
    }
    return 1;
}
/**
 * exprEq: exprRel exprEqPrime
 */
int exprEq(Ret *r){
    Token *startTk = crtTk;
    if(exprRel(r)){ if(exprEqPrime(r)) return 1; }
    crtTk = startTk;
    return 0;
}



/**
 * exprAndPrime: AND exprEq exprAndPrime | epsilon
 */
int exprAndPrime(Ret *r){
    if(consume(AND)){
        Ret right;
        if(!exprEq(&right)) tkerr(crtTk, "Missing expression after '&&'");
        Type tDst;
        if(!arithTypeTo(&r->type, &right.type, &tDst))
            tkerr(crtTk, "invalid operand type for &&");
        *r = (Ret){ {TB_INT, NULL, -1}, 0, 1, {.i=0} };
        return exprAndPrime(r);
    }
    return 1;
}

/**
 * exprAnd: exprEq exprAndPrime
 */
int exprAnd(Ret *r){
    Token *startTk = crtTk;
    if(exprEq(r)){ if(exprAndPrime(r)) return 1; }
    crtTk = startTk;
    return 0;
}



/**
 * exprOrPrime: OR exprAnd { semantic_actions } exprOrPrime | ε
 */
int exprOrPrime(Ret *r) {
    if (consume(OR)) {
        Ret right;
        
        if (exprAnd(&right)) {
            Type tDst;

            if (!arithTypeTo(&r->type, &right.type, &tDst)) {
                tkerr(crtTk, "invalid operand type for ||");
            }

            *r = (Ret){
                {TB_INT, NULL, -1}, // Type: int, no symbol, non-array
                0,                  // lval: false (0)
                1,                  // ct: true (1)
                {.i = 0}            // CtVal: initialize the union's integer field
            };

            if (exprOrPrime(r)) {
                return 1;
            }
            return 0; 
        } else {
            tkerr(crtTk, "Missing expression after '||'");
            return 0;
        }
    }

    // ε (epsilon) case: No OR token found, which is a valid exit condition.
    return 1;
}


/**
 * exprOr: exprAnd exprOrPrime
 */
int exprOr(Ret *r) {
    Token *startTk = crtTk;

    if (exprAnd(r)) {
        if (exprOrPrime(r)) {
            return 1;
        }
    }

    crtTk = startTk;
    return 0;
}

/**
 * exprAssign: exprUnary ASSIGN exprAssign | exprOr
 */

int exprAssign(Ret *r) {

    Token *startTk = crtTk;

    Ret rDst;

    // exprUnary ASSIGN exprAssign
    if (exprUnary(&rDst)) {

        if (consume(ASSIGN)) {

            // commitment point
            if (exprAssign(r)) {

                // verificări semantice

                if (!rDst.lval)
                    tkerr(crtTk,
                        "the assign destination must be a left-value");

                if (rDst.ct)
                    tkerr(crtTk,
                        "the assign destination cannot be constant");

                if (!canBeScalar(&rDst))
                    tkerr(crtTk,
                        "the assign destination must be scalar");

                if (!canBeScalar(r))
                    tkerr(crtTk,
                        "the assign source must be scalar");

                if (!convTo(&r->type, &rDst.type))
                    tkerr(crtTk,
                        "the assign source cannot be converted to destination");

                // rezultatul asignării
                r->lval = 0;
                r->ct = 1;

                return 1;
            }

            tkerr(crtTk,
                "Missing expression after '='");
        }
    }

    // backtrack
    crtTk = startTk;

    // exprOr
    if (exprOr(r))
        return 1;

    crtTk = startTk;
    return 0;
}

/**
 * expr: exprAssign
 */
int expr(Ret *r) {

    if (exprAssign(r))
        return 1;

    return 0;
}

/**
 * stmCompund: LACC (varDef|stm)* RACC
 */
int stmCompound(int newDomain) {
    Token *startTk = crtTk;
    if (consume(LACC)) {
        if (newDomain) pushDomain();
        while (1) {
            if (varDef()) {}
            else if (stm()) {}
            else break;
        }
        if (consume(RACC)) {
            if (newDomain) dropDomain();
            return 1;
        }
        tkerr(crtTk, "Expected variable definition, statement, or '}'");
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
int stm(){
    Token *startTk = crtTk;
    Ret rInit, rCond, rStep, rExpr;

    if(stmCompound(1)) return 1;
    crtTk = startTk;

    if(consume(IF)){
        if(!consume(LPAR)) tkerr(crtTk, "Expected '(' after 'if'");
        if(!expr(&rCond)) tkerr(crtTk, "Invalid condition in 'if'");
        if(!canBeScalar(&rCond)) tkerr(crtTk, "the if condition must be a scalar value");
        if(!consume(RPAR)) tkerr(crtTk, "Expected ')' after if condition");
        if(!stm()) tkerr(crtTk, "Missing statement after if");
        if(consume(ELSE)){
            if(!stm()) tkerr(crtTk, "Missing statement after else");
        }
        return 1;
    }
    crtTk = startTk;

    if(consume(WHILE)){
        if(!consume(LPAR)) tkerr(crtTk, "Expected '(' after while");
        if(!expr(&rCond)) tkerr(crtTk, "Invalid while condition");
        if(!canBeScalar(&rCond)) tkerr(crtTk, "the while condition must be a scalar value");
        if(!consume(RPAR)) tkerr(crtTk, "Expected ')' after while condition");
        if(!stm()) tkerr(crtTk, "Missing while body");
        return 1;
    }
    crtTk = startTk;

    if(consume(FOR)){
        if(!consume(LPAR)) tkerr(crtTk, "Expected '(' after for");
        expr(&rInit);
        if(!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after for init");
        if(expr(&rCond)){
            if(!canBeScalar(&rCond)) tkerr(crtTk, "the for condition must be a scalar value");
        }
        if(!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after for condition");
        expr(&rStep);
        if(!consume(RPAR)) tkerr(crtTk, "Expected ')' after for");
        if(!stm()) tkerr(crtTk, "Missing for body");
        return 1;
    }
    crtTk = startTk;

    if(consume(BREAK)){
        if(!consume(SEMICOLON)) tkerr(crtTk, "Missing ';' after break");
        return 1;
    }
    crtTk = startTk;

    if(consume(RETURN)){
        if(expr(&rExpr)){
            if(owner->type.typeBase == TB_VOID)
                tkerr(crtTk, "a void function cannot return a value");
            if(!canBeScalar(&rExpr))
                tkerr(crtTk, "the return value must be a scalar value");
            if(!convTo(&rExpr.type, &owner->type))
                tkerr(crtTk, "cannot convert the return expression type to the function return type");
        } else {
            if(owner->type.typeBase != TB_VOID)
                tkerr(crtTk, "a non-void function must return a value");
        }
        if(!consume(SEMICOLON)) tkerr(crtTk, "Missing ';' after return");
        return 1;
    }

    if(expr(&rExpr)){
        if(!consume(SEMICOLON)) tkerr(crtTk, "Missing ';' after expression");
        return 1;
    } else if(consume(SEMICOLON)) return 1;

    crtTk = startTk;
    return 0;
}


/**
 * fnParam: typeBase ID arrayDecl?
 * fnParam: {Type t;} typeBase[&t] ID[tkName]
(arrayDecl[&t] {t.n=0;} )?
{
Symbol *param=findSymbolInDomain(symTable,tkName->text);
if(param)tkerr(iTk,"symbol redefinition: %s",tkName->text);
param=newSymbol(tkName->text,SK_PARAM);
param->type=t;
param->paramIdx=symbolsLen(owner->fn.params);
// parametrul este adaugat atat la domeniul curent, cat si la parametrii fn
addSymbolToDomain(symTable,param);
addSymbolToList(&owner->fn.params,dupSymbol(param));
}
 */
int fnParam(){
    Token *startTk = crtTk;
    Type t;
    if(typeBase(&t)){
        Token *tkName = crtTk;
        if(consume(ID)){
            if(arrayDecl(&t)) t.nElements = 0;
            Symbol *param = findSymbolInDomain(symTable, tkName->text);
            if(param) tkerr(crtTk, "symbol redefinition: %s", tkName->text);
            param = newSymbol(tkName->text, SK_PARAM);
            param->type = t;
            param->owner = owner;
            param->paramIdx = symbolsLen(&owner->fn.params);
            addSymbolToDomain(symTable, param);
            addSymbolToList(&owner->fn.params, dupSymbol(param));
            return 1;
        }
        tkerr(crtTk, "Expected identifier after type in function parameter");
    }
    crtTk = startTk;
    return 0;
}

/**
 * fnDef : (typeBase | VOID) ID LPAR (fnParam (COMMA fnParam)* )? RPAR stmCompound
 */
/**
 * fnDef : (typeBase | VOID) ID LPAR (fnParam (COMMA fnParam)* )? RPAR stmCompound[false]
 */
int fnDef(){
    Token *startTk = crtTk;
    Type t;
    int foundType = 0;
    if(typeBase(&t)) foundType = 1;
    else {
        crtTk = startTk;
        if(consume(VOID)){
            t.typeBase = TB_VOID; t.s = NULL; t.nElements = -1;
            foundType = 1;
        }
    }
    if(foundType){
        Token *tkName = crtTk;
        if(consume(ID)){
            if(consume(LPAR)){

                Symbol *fn = findSymbolInDomain(symTable, tkName->text);
                if(fn) tkerr(crtTk, "symbol redefinition: %s", tkName->text);
                fn = newSymbol(tkName->text, SK_FN);
                fn->type = t;
                addSymbolToDomain(symTable, fn);
                Symbol *savedOwner = owner;
                owner = fn;
                pushDomain(); 

                if(fnParam()){
                    while(consume(COMMA))
                        if(!fnParam()) tkerr(crtTk, "Expected parameter after ','");
                }
                if(!consume(RPAR)) tkerr(crtTk, "Missing ')' in function signature");

                if(!stmCompound(0)) tkerr(crtTk, "Missing function body");

                dropDomain();
                owner = savedOwner;
                return 1;
            }
        }
    }
    crtTk = startTk;
    return 0;
}

/**
 * arrayDecl : LBRACKET CT_INT? RBRACKET
 * 
 * 
 *arrayDecl[inout Type *t]: LBRACKET
( CT_INT[tkSize] {t->n=tkSize->i;} | {t->n=0;} )
RBRACKET
 */
int arrayDecl(Type* t){
    Token *startTk = crtTk;

    if(consume(LBRACKET)) {
        Token* tkSize = crtTk;
        if(consume(CT_INT)) {
            t->nElements = tkSize->i;
        }
        else{
            t->nElements = 0;
        }
        if(!consume(RBRACKET)) tkerr(crtTk, "Missing ] in array declaration");
        return 1;
    }

    crtTk = startTk;
    return 0;
}



/**
 * typeBase : INT | DOUBLE | CHAR | STRUCT ID
 */
int typeBase(Type *t){
    Token *startTk = crtTk;
    t->nElements = -1;
    t->s = NULL;
    if(consume(INT))   { t->typeBase = TB_INT;    return 1; }
    if(consume(DOUBLE)){ t->typeBase = TB_DOUBLE; return 1; }
    if(consume(CHAR))  { t->typeBase = TB_CHAR;   return 1; }
    if(consume(STRUCT)){
        Token *tkName = crtTk;
        if(consume(ID)){
            t->typeBase = TB_STRUCT;
            t->s = findSymbol(tkName->text);
            if(!t->s) tkerr(crtTk, "structura nedefinita: %s", tkName->text);
            return 1;
        }
        tkerr(crtTk, "Expected identifier after 'struct'");
    }
    crtTk = startTk;
    return 0;
}


/**
 * varDef: typeBase ID arrayDecl? (ASSIGN expr)? SEMICOLON
 */
int varDef(){
    Token *startTk = crtTk;
    Type t;
    if(typeBase(&t)){
        Token *tkName = crtTk;
        if(consume(ID)){
            // Check if it's an array definition
            if(arrayDecl(&t)){
                if(t.nElements == 0)
                    tkerr(crtTk, "a vector variable must have a specified dimension");
            }
            // Optional assignment with type checks
            if(consume(ASSIGN)){
                Ret rInit;
                if(!expr(&rInit)) tkerr(crtTk, "Expected expression after '=' in declaration");
                if(!canBeScalar(&rInit))
                    tkerr(crtTk, "the variable initializer must be a scalar");
                if(!convTo(&rInit.type, &t))
                    tkerr(crtTk, "the initializer cannot be converted to the variable type");
            }
            if(consume(SEMICOLON)){


                Symbol *var = findSymbolInDomain(symTable, tkName->text);
                if(var) tkerr(crtTk, "symbol redefinition: %s", tkName->text);

                // 2. Instantiate and attach properties
                var = newSymbol(tkName->text, SK_VAR);
                var->type = t;
                var->owner = owner;

                // 3. Register within the top-level processing table
                addSymbolToDomain(symTable, var);

                // 4. Compute layout offsets depending on lexical environment
                if(owner){
                    switch(owner->kind){
                        case SK_FN:
                            // Local variable inside a function body
                            var->varIdx = symbolsLen(&owner->fn.locals);
                            addSymbolToList(&owner->fn.locals, dupSymbol(var));
                            break;
                        case SK_STRUCT:
                            // Layout member field inside a structure definition
                            var->varIdx = typeSize(&owner->type);
                            addSymbolToList(&owner->structMembers, dupSymbol(var));
                            break;
                    }
                } else {
                    var->varIdx = allocInGlobalMemory(typeSize(&t));
                }
                // =========================================================
                return 1;
            }
            tkerr(crtTk, "Missing ';' after variable declaration");
        }
        crtTk = startTk;
    }
    return 0;
}
/**
 * structDef: STRUCT ID LACC varDef* RACC SEMICOLON
 */
int structDef(){
    Token *startTk = crtTk;
    if(consume(STRUCT)){
        Token *tkName = crtTk;
        if(consume(ID)){
            if(consume(LACC)){
                Symbol *s = findSymbolInDomain(symTable, tkName->text);
                if(s) tkerr(crtTk, "symbol redefinition: %s", tkName->text);
                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
                s->type.typeBase = TB_STRUCT;
                s->type.s = s;
                s->type.nElements = -1;
                pushDomain();
                Symbol *savedOwner = owner;
                owner = s;
                while(varDef()){}
                if(!consume(RACC)) tkerr(crtTk, "Missing '}' in struct definition");
                owner = savedOwner;
                dropDomain();
                if(!consume(SEMICOLON)) tkerr(crtTk, "Missing ';' after struct definition");
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

Symbol *addExtFunc(const char *name, Type type, void *addr) {
    Symbol *s = newSymbol(name, SK_FN); 
    s->type = type;
    
    s->addr = addr; 
    
    initSymbols(&s->fn.params); 
    addSymbolToDomain(symTable, s); 
    
    return s;
}


Symbol *addFuncArg(Symbol *func, const char *name, Type type) {
    Symbol *a = newSymbol(name, SK_PARAM); 
    a->type = type;
    
    addSymbolToDomain(&func->fn.params, a); 
    
    return a;
}

extern void put_i();

void addExtFuncs() {
    Symbol *s;

    s = addExtFunc("put_i", createType(TB_VOID, -1), put_i);
    addFuncArg(s, "i", createType(TB_INT, -1));


    s = addExtFunc("put_s", createType(TB_VOID, -1), NULL);
    addFuncArg(s, "s", createType(TB_CHAR, 0));

    s = addExtFunc("get_i", createType(TB_INT, -1), NULL);
    
}