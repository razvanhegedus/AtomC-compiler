#ifndef DA_H
#define DA_H

#include "lexer.h"

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };
enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };
enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

struct _Symbol;
typedef struct _Symbol Symbol;

typedef struct {
    int typeBase;  // TB_* 
    Symbol *s;     // struct definition for TB_STRUCT 
    int nElements; // >0 array of size, 0=unsized, <0 non-array 
} Type;


typedef struct _Symbol {
    const char *name;
    int cls;         
    int mem;         
    Type type;
    int depth;       
    union {
        Symbols args;    
        Symbols members; 
    };
    Symbol *owner;    // Points to the parent Function or Struct 
    int varIdx;       // The index or offset within that owner [cite: 35, 40]
} Symbol;


typedef struct {
    Symbol **begin; 
    Symbol **end;   
    Symbol **after; 
} Symbols;


extern Symbols symbols;  // The main Symbol Table 
extern int crtDepth;     // Current nesting depth 
extern Symbol *owner;


void initSymbols(Symbols *symbols);
Symbol *addSymbol(Symbols *symbols, const char *name, int cls);
Symbol *findSymbol(Symbols *symbols, const char *name);

// Domain Logic (From PDF) 
void pushDomain();
void dropDomain();
Symbol *findSymbolInDomain(Symbols *symbols, const char *name);
Symbol *newSymbol(const char *name, int cls);

// Helper functions for list management [cite: 2]
void addSymbolToList(Symbols *list, Symbol *s);
int symbolsLen(Symbols *list);
Symbol *dupSymbol(Symbol *s);


#endif
