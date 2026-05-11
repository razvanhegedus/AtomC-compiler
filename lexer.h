#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

// 1. Token Codes
enum {
    ID = 1,
    CT_INT,
    CT_REAL,
    CT_CHAR,
    CT_STRING,
    BREAK,
    INT,
    CHAR,
    DOUBLE,
    ELSE,
    FOR,
    IF,
    RETURN,
    STRUCT,
    VOID,
    WHILE,
    COMMA,
    SEMICOLON,
    LPAR,
    RPAR,
    LBRACKET,
    RBRACKET,
    LACC,
    RACC,
    END,
    ADD,
    SUB,
    MUL,
    DIV,
    DOT,
    AND,
    OR,
    NOT,
    ASSIGN,
    EQUAL,
    NOTEQ,
    LESS,
    LESSEQ,
    GREATER,
    GREATEREQ
};

// 2. Token Structure
typedef struct _Token {
    int code;
    union {
        char* text; // used for ID, CT_STRING
        long int i; // CT_INT, CT_CHAR
        double r;   // CT_REAL
    };
    int line; // input file line
    struct _Token* next;
} Token;

// 3. Global Variables (Shared between lexer and parser)
extern const char *tokenNames[];
extern Token *tokens;      // head of the list
extern Token *lastToken;   // tail of the list
extern Token *crtTk;       // parser's current position
extern Token *consumedTk;  // last consumed token for parser
extern char* pCrtCh;
extern int line;

// 4. Function Prototypes
void err(const char *fmt, ...);
void tkerr(const Token *tk,const char *fmt,...);
Token* addTk(int code, int line);
char* createString(const char* start, const char* end);
Token* getNextToken();
void showTokens();
char* loadFile(const char* fileName);
void done();

#endif // LEXER_H