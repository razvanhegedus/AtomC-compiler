#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DA.h"
#include "TA.h"

Symbols* symTable;
int crtDepth = 0;
Symbol* owner;
int crtGlobalMemorySize = 0; 

int allocInGlobalMemory(int size) {
    int currentOffset = crtGlobalMemorySize;
    crtGlobalMemorySize += size;
    return currentOffset;
}

void initSymbols(Symbols *symbols) {
    symbols->begin = NULL;
    symbols->end = NULL;  
    symbols->after = NULL; 
}

Symbol *newSymbol(const char *name, int kind) {
    Symbol *s;
    SAFEALLOC(s, Symbol); 
    s->name = name; 
    s->kind = kind;
    s->depth = crtDepth; 

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
    if (list->end == list->after) { 
        int count = list->end - list->begin;
        int n = count * 2;
        if (n == 0) n = 1; 
        
        list->begin = (Symbol **)realloc(list->begin, n * sizeof(Symbol *));
        if (list->begin == NULL) err("not enough memory for symbol table"); 
        
        list->end = list->begin + count;
        list->after = list->begin + n;
    }
    *list->end++ = s; 
    return s;
}

Symbol *findSymbolInDomain(Symbols *list, const char *name) {
    if (list == NULL || list->begin == list->end) return NULL;
    
    int count = list->end - list->begin;
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(list->begin[i]->name, name) == 0) {
            return list->begin[i]; 
        }
    }
    return NULL; 
}

Symbol *findSymbol(const char *name) {
    if (symTable == NULL || symTable->begin == symTable->end) return NULL;
    
    int count = symTable->end - symTable->begin;
    for (int i = count - 1; i >= 0; i--) {
        if (strcmp(symTable->begin[i]->name, name) == 0) {
            return symTable->begin[i]; 
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
    *d = *s; 
    return d;
}

int typeSize(Type *t) {
    int baseSize = 4; 
    
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
    
    if (s->kind == SK_STRUCT) {
        int mCount = s->structMembers.end - s->structMembers.begin;
        for (int i = 0; i < mCount; i++) {
            printSymbol(s->structMembers.begin[i], indent + 1);
        }
    } 
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