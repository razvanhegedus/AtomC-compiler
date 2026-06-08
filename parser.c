#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "DA.h"

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
    // Safety check: if the list pointer is NULL or the list is empty, return immediately
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
    // Safety check: if the global symbol table is NULL or empty, return immediately
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
    if (list->end == list->after) {
        int count = list->end - list->begin;
        int n = count * 2;
        if (n == 0) n = 1;
        
        list->begin = (Symbol **)realloc(list->begin, n * sizeof(Symbol *));
        if (list->begin == NULL) err("not enough memory for symbol list");
        
        list->end = list->begin + count;
        list->after = list->begin + n;
    }
    *list->end++ = s;
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
    
    // If the type is a structure, calculate its size by summing up its members
    if (t->typeBase == TB_STRUCT && t->s != NULL) {
        baseSize = 0;
        Symbols *members = &t->s->structMembers;
        int count = members->end - members->begin;
        for (int i = 0; i < count; i++) {
            baseSize += typeSize(&members->begin[i]->type);
        }
    }
    
    if (t->nElements >= 0) {
        // nElements == 0 indicates an unsized array parameter (pointer size = 4 bytes)
        if (t->nElements == 0) {
            return 4;
        }
        return t->nElements * baseSize;
    }
    
    return baseSize;
}



// Funcții auxiliare pentru conversia enum-urilor în text
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

// Afișează tipul unui simbol (inclusiv dacă este structură sau vector)
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
    
    // Afișare dimensiune vector dacă este cazul
    if (t->nElements == 0) {
        printf("[]");
    } else if (t->nElements > 0) {
        printf("[%d]", t->nElements);
    }
}

// Afișează recursiv un simbol și sub-simbolurile sale (membri struct sau variabile locale)
void printSymbol(Symbol *s, int indent) {
    // Generăm indentarea pentru un aspect ierarhic curat
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
    
    printf("\n==================== TABELA DE SIMBOLURI ====================\n");
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
    Type t;
    // Possibility 1: ( typeBase arrayDecl? ) exprCast
    if (consume(LPAR)) {
        if (typeBase(&t)) {
            arrayDecl(&t); // Optional, so we don't check return value for error
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
int stmCompound(int newDomain) {
    Token *startTk = crtTk;
    if(consume(LACC)){
        if (newDomain) {
            pushDomain(); 
        }
        while(1) {
            if(varDef()){}
            else if(stm()){}
            else {
                break;
            }
        }
        if(consume(RACC)) {
            if (newDomain) {
                dropDomain(); 
            }
            return 1;
        }
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

    if (stmCompound(1)) return 1;

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
int fnParam() {
    Token *startTk = crtTk;
    Type t;

    if (typeBase(&t)) {
        // COMMITMENT POINT: Once we have a type, we REQUIRE an ID.
        Token *tkName = crtTk;
        if (consume(ID)) {

            if(arrayDecl(&t))
            {
                t.nElements = 0;
            }
            Symbol *param=findSymbolInDomain(symTable,tkName->text);
            if(param)tkerr(crtTk,"symbol redefinition: %s",tkName->text);
            param=newSymbol(tkName->text,SK_PARAM);
            param->type = t;
            param->owner = owner;
            param->paramIdx=symbolsLen(&owner->fn.params);
            addSymbolToDomain(symTable,param);
            addSymbolToList(&owner->fn.params,dupSymbol(param));
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
/**
 * fnDef : (typeBase | VOID) ID LPAR (fnParam (COMMA fnParam)* )? RPAR stmCompound[false]
 */
int fnDef() {
    Token *startTk = crtTk;
    Type t;

    // 1. Determine return type: typeBase OR VOID
    int foundType = 0;
    if (typeBase(&t)) {
        foundType = 1;
    } else {
        crtTk = startTk; // Reset to attempt to match VOID
        if (consume(VOID)) {
            t.typeBase = TB_VOID;
            t.s = NULL;
            t.nElements = -1;
            foundType = 1;
        }
    }

    if (foundType) {
        Token *tkName = crtTk; // Capture function name token
        if (consume(ID)) {
            if (consume(LPAR)) {
                // =========================================================
                // DOMAIN ANALYSIS: Entering Function Definition
                // =========================================================
                Symbol *fn = findSymbolInDomain(symTable, tkName->text);
                if (fn) tkerr(crtTk, "symbol redefinition: %s", tkName->text);
                
                fn = newSymbol(tkName->text, SK_FN);
                fn->type = t;
                addSymbolToDomain(symTable, fn);
                
                Symbol *savedOwner = owner;
                owner = fn;
                pushDomain(); // Local function scope scope starts immediately after '('
                // =========================================================

                // 2. Parse arguments
                if (fnParam()) {
                    while (consume(COMMA)) {
                        if (!fnParam()) tkerr(crtTk, "Expected parameter after ','");
                    }
                }

                if (!consume(RPAR)) tkerr(crtTk, "Missing ')' in function signature");
                
                // 3. Parse function body
                // Pass '0' (false) so stmCompound knows NOT to push an extra subdomain!
                if (!stmCompound(0)) tkerr(crtTk, "Missing function body");
                
                // =========================================================
                // DOMAIN ANALYSIS: Exiting Function Definition (Where text cut off)
                // =========================================================
                dropDomain();
                owner = savedOwner; // Restore the outer processing context safely
                // =========================================================
                
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
int typeBase(Type *t) {
    Token *startTk = crtTk;

    t->nElements = -1; 
    t->s = NULL;

    if (consume(INT)) { t->typeBase = TB_INT; return 1; } 
    if (consume(DOUBLE)) { t->typeBase = TB_DOUBLE; return 1; }
    if (consume(CHAR)) { t->typeBase = TB_CHAR; return 1; } 
    
    if (consume(STRUCT)) {
        Token *tkName = crtTk;
        if (consume(ID)) {
            t->typeBase = TB_STRUCT; 
            t->s = findSymbol(tkName->text); 
            if (!t->s) tkerr(crtTk, "structura nedefinita: %s", tkName->text);
            return 1;
        } else {
            tkerr(crtTk, "Expected identifier after 'struct'");
        }
    }

    crtTk = startTk;
    return 0;
}


/**
 * varDef: typeBase ID arrayDecl? (ASSIGN expr)? SEMICOLON
 */
int varDef() {
    Token *startTk = crtTk;
    Type t; 

    if (typeBase(&t)) {
        Token *tkName = crtTk; // Capture identifier token before consuming it
        
        if (consume(ID)) {
            // Check if it's an array definition
            if (arrayDecl(&t)) { 
                if (t.nElements == 0) { 
                    tkerr(crtTk, "a vector variable must have a specified dimension"); 
                }
            }

            // Optional assignment
            if (consume(ASSIGN)) {
                if (!expr()) tkerr(crtTk, "Expected expression after '=' in declaration");
            }

            if (consume(SEMICOLON)) {
                // =========================================================
                // DOMAIN ANALYSIS: Variable Processing
                // =========================================================
                
                // 1. Verify symbol name uniqueness within current local scope
                Symbol *var = findSymbolInDomain(symTable, tkName->text); 
                if (var) tkerr(crtTk, "symbol redefinition: %s", tkName->text); 
                
                // 2. Instantiate and attach properties
                var = newSymbol(tkName->text, SK_VAR); 
                var->type = t; 
                var->owner = owner;
                
                // 3. Register within the top-level processing table
                addSymbolToDomain(symTable, var); 

                // 4. Compute layout offsets depending on lexical environment
                if (owner) { 
                    switch (owner->kind) { 
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
                    // =========================================================
                    // THE FINAL CHECK: Global Scope Allocation
                    // =========================================================
                    var->varIdx = allocInGlobalMemory(typeSize(&t));
                }
                // =========================================================
                
                return 1;
            } else {
                tkerr(crtTk, "Missing ';' after variable declaration");
            }
        }
        crtTk = startTk;
    }
    return 0;
}
/**
 * structDef: STRUCT ID LACC varDef* RACC SEMICOLON
 */
int structDef() {
    Token *startTk = crtTk;

    if (consume(STRUCT)) {
        Token *tkName = crtTk; // Capture the ID token before consuming it
        
        if (consume(ID)) {
            if (consume(LACC)) {

                Symbol *s = findSymbolInDomain(symTable, tkName->text);
                if (s) {
                    tkerr(crtTk, "symbol redefinition: %s", tkName->text);
                }
                

                s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));

                s->type.typeBase = TB_STRUCT; 
                s->type.s = s;                
                s->type.nElements = -1;       
                
                pushDomain();
     
                owner = s;

                while (varDef()) {
                }
                if (!consume(RACC)) tkerr(crtTk, "Missing '}' in struct definition");

                owner = NULL; 
                dropDomain();       
                
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