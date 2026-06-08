#ifndef DA_H
#define DA_H

#include "lexer.h"

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };
enum { SK_VAR, SK_FN, SK_STRUCT, SK_PARAM };
enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

struct _Symbol;
typedef struct _Symbol Symbol;

typedef struct {
    int typeBase;  
    Symbol *s;     
    int nElements; // >0 array of size, 0=unsized, <0 non-array 
} Type;


typedef struct {
    Symbol **begin; 
    Symbol **end;   
    Symbol **after; 
} Symbols;


typedef struct _Symbol {
    const char *name;
    int kind;         
    int mem;         
    Type type;
    int depth;       
    union {
        struct {
            Symbols params;
            Symbols locals;
        } fn;                
        Symbols structMembers; 
    };
    Symbol *owner;    // Points to the parent Function or Struct 
    union {
        int varIdx;   
        int paramIdx; 
    };      
} Symbol;



extern Symbols* symTable;  // The main Symbol Table 
extern int crtDepth;     // Current nesting depth 
extern Symbol *owner;


void initSymbols(Symbols *symbols);
Symbol *newSymbol(const char *name, int kind);
Symbol *addSymbolToDomain(Symbols *list, Symbol *s);
Symbol *findSymbolInDomain(Symbols *list, const char *name);
Symbol *findSymbol(const char *name); // Căutare globală în TS

void pushDomain();
void dropDomain();

void addSymbolToList(Symbols *list, Symbol *s);
int symbolsLen(Symbols *list);
Symbol *dupSymbol(Symbol *s);
int typeSize(Type *t); 
int allocInGlobalMemory(int size);

const char* kindToString(int kind);
const char* memToString(int mem);
void printType(Type *t);
void printSymbol(Symbol *s, int indent);
void printSymbolTable();




#endif
