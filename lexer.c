#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>  
#include <stdarg.h> 

char* pCrtCh;
int line = 1;


void err(const char *fmt,...)
{
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"lexical error at line %d: ", line);
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

void tkerr(const Token *tk,const char *fmt,...)
{
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error in line %d: ",tk->line);
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");



const char *tokenNames[] = {
    "", "ID", "CT_INT", "CT_REAL", "CT_CHAR", "CT_STRING", 
    "BREAK", "CHAR", "DOUBLE", "ELSE", "FOR", "IF", "RETURN", "STRUCT", "VOID", "WHILE",
    "COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC",
    "END", "ADD", "SUB", "MUL", "DIV", "DOT", "AND", "OR", "NOT", "ASSIGN", 
    "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ"
};


Token* lastToken = NULL; //tail of the list
Token* tokens = NULL; //head of the lsit

Token* crtTk = NULL;
Token* consumedTk = NULL;

Token* addTk(int code, int line)
{
    Token* tk;
    SAFEALLOC(tk, Token);
    tk->code = code;
    tk->line = line;
    tk->next = NULL;
    if(lastToken)
    {
        lastToken->next = tk;
    }
    else
    {
        tokens = tk;
    }
    lastToken = tk;
    return tk;
}

char* createString(const char* start, const char* end)
{
    int len = end - start;
    char* str = (char*)malloc(len + 1);
    
    if (str == NULL) {
        err("not enough memory for createString");
    }
    
    memcpy(str, start, len);
    
    str[len] = '\0';
    
    return str;
}

Token* getNextToken()
{
    int state = 0, nCh;
    char ch;
    const char *pStartCh;
    Token* tk;

    while(1)
    {
        ch = *pCrtCh;
        switch(state)
        {
            case 0: //start state
                if(isalpha(ch) || ch == '_')  //for ID
                {
                    pStartCh = pCrtCh; //memorize beginning of the ID
                    pCrtCh++; //consume the character
                    state = 1;
                }
                else if(ch==' ' || ch == '\r' || ch == '\t') //consume the character
                {
                    pCrtCh++;
                }
                else if(ch == '\n') // update line
                {
                    line++;
                    pCrtCh++;
                }
                else if(ch == '\0' || ch == EOF) //end of input string
                {
                    return addTk(END, line);
                }
                else if(ch == ',')
                {
                    pCrtCh++;
                    return addTk(COMMA, line);
                }
                else if(ch == ';')
                {
                    pCrtCh++;
                    return addTk(SEMICOLON, line);
                }
                else if(ch == '(')
                {
                    pCrtCh++;
                    return addTk(LPAR, line);
                }
                else if(ch == ')')
                {
                    pCrtCh++;
                    return addTk(RPAR, line);
                }
                else if(ch == '[')
                {
                    pCrtCh++;
                    return addTk(LBRACKET, line);
                }
                else if(ch == ']')
                {
                    pCrtCh++;
                    return addTk(RBRACKET, line);
                }
                else if(ch == '{')
                {
                    pCrtCh++;
                    return addTk(LACC, line);
                }
                else if(ch == '}')
                {
                    pCrtCh++;
                    return addTk(RACC, line);
                }
                //Operators
                else if(ch == '+')
                {
                    pCrtCh++;
                    return addTk(ADD, line);
                    
                }
                else if(ch == '-')
                {
                    pCrtCh++;
                    return addTk(SUB, line);
                }
                else if(ch == '*')
                {
                    pCrtCh++;
                    return addTk(MUL, line);
                }
                else if(ch == '/')
                {
                    pCrtCh++;
                    state = 6; // state to check for DIV, COMMENT or multi line comment
                }
                else if(ch == '=') //for equals or assign
                {
                    pCrtCh++;
                    state = 3; //state to check equals or assign
                }
                else if(ch == '.')
                {
                    pCrtCh++;
                    return addTk(DOT, line);
                }
                else if(ch == '&'){
                    pCrtCh++;
                    state = 10; //check for the next character 
                }
                else if(ch == '|'){
                    pCrtCh++;
                    state = 11;
                }
                else if(ch == '!')
                {
                    pCrtCh++;
                    state = 12; //state to check for not or noteq
                }
                else if(ch == '<')
                {
                    pCrtCh++;
                    state = 15; //state to cehck for less or lesseq
                }
                else if(ch == '>')
                {
                    pCrtCh++;
                    state = 18; //state to check for greater or greatereq
                }
                else if(ch == '\''){ 
                    pCrtCh++;
                    state = 21; //state for char verificiation
                }
                else if(ch == '\"')
                {
                    pCrtCh++;
                    pStartCh = pCrtCh;
                    state = 24;
                }
                //int and real
                else if(ch == '0')
                {
                    pStartCh = pCrtCh;
                    pCrtCh++;
                    state = 26; //check for zero, hex, octal 
                }
                else if(isdigit(ch) && ch != '0')
                {
                    pStartCh = pCrtCh;
                    pCrtCh++;
                    state = 33;
                }
                break;

            case 1:
                if(isalpha(ch) || ch == '_') 
                {
                    pCrtCh++;
                }
                else state = 2;
                break;

            case 2:
                nCh = pCrtCh - pStartCh; //id length
                //keywotd test
                if (nCh == 5 && !memcmp(pStartCh, "break", 5)) tk = addTk(BREAK, line);
                else if (nCh == 4 && !memcmp(pStartCh, "char", 4)) tk = addTk(CHAR, line);
                else if (nCh == 6 && !memcmp(pStartCh, "double", 6)) tk = addTk(DOUBLE, line);
                else if (nCh == 4 && !memcmp(pStartCh, "else", 4)) tk = addTk(ELSE, line);
                else if (nCh == 3 && !memcmp(pStartCh, "for", 3)) tk = addTk(FOR, line);
                else if (nCh == 2 && !memcmp(pStartCh, "if", 2)) tk = addTk(IF, line);
                else if (nCh == 6 && !memcmp(pStartCh, "return", 6)) tk = addTk(RETURN, line);
                else if (nCh == 6 && !memcmp(pStartCh, "struct", 6)) tk = addTk(STRUCT, line);
                else if (nCh == 4 && !memcmp(pStartCh, "void", 4)) tk = addTk(VOID, line);
                else if (nCh == 5 && !memcmp(pStartCh, "while", 5)) tk = addTk(WHILE, line);
                
                // 2. Default case: it's a user-defined Identifier
                else {
                    tk = addTk(ID, line);
                    tk->text = createString(pStartCh, pCrtCh);
                }

                return tk; 

            case 3:
                if(ch == '=')
                {
                    pCrtCh++;
                    state = 4;
                }
                else state = 5;
                break;
            case 4:
                return addTk(EQUAL, line);

            case 5:
                return addTk(ASSIGN, line);

            case 6:
                if(ch == '/')
                {
                    pCrtCh++;
                    state = 7;
                }
                else if(ch == '*')
                {
                    pCrtCh++;
                    state = 8;
                }
                else{
                    return addTk(DIV, line);
                }
                break;

            case 7:
                if(ch != '\n' && ch != '\r' && ch != '\0' && ch != EOF) pCrtCh++;
                else state = 0;
                break;

            case 8:
                if(ch == '*') 
                {
                    pCrtCh++;
                    state = 9; //will check for a / 
                }
                else if(ch == '\n') 
                {
                    line++;    
                    pCrtCh++;
                } 
                else if(ch == '\0' || ch == EOF) 
                {
                    err("Unterminated comment");
                } 
                else 
                {
                    pCrtCh++;
                }
                break;

            case 9:
                if(ch == '/')
                {
                    pCrtCh++;
                    state = 0;
                }
                else if(ch == '*') //stay here, may have **/
                {
                    pCrtCh++;
                }
                else
                {
                    pCrtCh++;
                    state = 8;
                }
                break;

            case 10: //check for second &
                if(ch == '&')
                {
                    pCrtCh++;
                    return addTk(AND, line);
                }
                else{
                    err("invalid character: expected '&&'");
                }
                break;

            case 11:
                if(ch == '|')
                {
                    pCrtCh++;
                    return addTk(OR, line);
                }
                else{
                    err("invalid character: expected '||'");
                }
                break;

            case 12:
                if(ch == '='){
                    pCrtCh++;
                    state = 13;
                }
                else{
                    state = 14;
                }
                break;

            case 13:
                return addTk(NOTEQ, line);

            case 14:
                return addTk(NOT, line);
            
            case 15: // Just saw '<'
                if(ch == '='){
                    pCrtCh++;
                    state = 16;
                }
                else{
                    state = 17;
                }
                break;
                
            case 16:
                return addTk(LESSEQ, line);

            case 17:
                return addTk(LESS, line);

            case 18: // Just saw '>'
                if(ch == '='){
                    pCrtCh++;
                    state = 19;
                }
                else{
                    state = 20;
                }
                break;

            case 19:
                return addTk(GREATEREQ, line);

            case 20:
                return addTk(GREATER, line);
            
            case 21:
                if(ch != '\'' && ch != '\n' && ch != '\r' && ch != '\0' && ch != EOF)
                {
                    pCrtCh++;
                    state = 22;
                }
                else
                {
                    err("Invalid or empty character constant");
                }
                break;

            case 22:
                if(ch == '\'')
                {
                    pCrtCh++;
                    state = 23;
                }
                else
                {
                    err("Missing closing single quote");
                }
                break;

            case 23:
                tk = addTk(CT_CHAR, line);
                tk->i = *(pCrtCh - 2);
                return tk;
            case 24:
                if (ch == '\"') { 
                    state = 25;
                } 
                else if (ch == '\n' || ch == '\r' || ch == '\0' || ch == EOF) {
                    err("Unterminated string constant ");
                } 
                else {
                    pCrtCh++;
                    // Stay in state 24
                }
                break;
            case 25:
                tk = addTk(CT_STRING, line);
                tk->text = createString(pStartCh, pCrtCh);
                pCrtCh++;
                return tk;
            case 26: //seen 0
                if(ch == 'x' || ch == 'X'){ //hex
                    pCrtCh++;
                    state = 28;
                }
                else if (isdigit(ch) && ch != '8' && ch != '9') 
                { 
                    pCrtCh++; state = 30; //octal
                }
                else if(ch == '.')
                {
                    pCrtCh++; state = 36; //Real case (eg:0.5) 
                }
                else
                {
                    state = 32; //just zero
                }
                break;
            case 28: // HEX LOOP: After '0x'
                if (isxdigit(ch)) 
                { 
                    pCrtCh++; 
                } 
                else 
                {
                    if (pCrtCh - pStartCh <= 2)
                    { 
                        err("Invalid hex: expected hex digits after 0x");
                    }
                    state = 29; // Final HEX state
                }
                break;

            case 29: //final hex state
                tk = addTk(CT_INT, line);
                char *sHex = createString(pStartCh, pCrtCh);
                tk->i = strtol(sHex, NULL, 16);
                free(sHex);
                return tk;

            case 30:
                if(isdigit(ch) && ch != '8' && ch != '9')
                {
                    pCrtCh++;
                }
                else 
                {
                    if(pCrtCh - pStartCh <= 1)
                    {
                        err("Invalid octal: expected octal digits after 0");
                    }
                    state = 31; //final octal state
                }
                break;
                
            case 31: //final octal state
                tk = addTk(CT_INT, line);
                char* sOct = createString(pStartCh, pCrtCh);
                tk->i = strtol(sOct, NULL, 8);
                free(sOct);
                return tk;
                
            case 32: //final 0 state
                tk = addTk(CT_INT, line);
                tk->i = 0;
                return tk;
            
            case 33: //base 10 int saw 1-9
                if (isdigit(ch)) { pCrtCh++; } //stay here
                else if (ch == '.') { pCrtCh++; state = 36; } // Real path
                else if (ch == 'e' || ch == 'E') { pCrtCh++; state = 39; } // Exponent path
                else if(ch == ';' || ch == ',' || ch == ' ' || ch == '+' || ch == '-' || ch == '/' || ch == '*'){ state = 34; } // Final base10 int
                else {err("Invalid integer");}
                break;

            case 34: //final base10 state
                tk = addTk(CT_INT, line);
                char *sInt = createString(pStartCh, pCrtCh);
                tk->i = strtol(sInt, NULL, 10); 
                free(sInt);
                return tk;
                
            case 36: //seen dot after a 0 or base 10
                if(isdigit(ch))
                {
                    pCrtCh++;
                    state = 37;
                }
                else{
                    err("Real number errorr:Need to see a digit after a dot");
                }
                break;

            case 37: //seen .digit
                if(isdigit(ch))
                {
                    pCrtCh++;
                }
                else if(ch == 'e' || ch == 'E')
                {
                    pCrtCh++;
                    state = 39;
                }
                else{
                    state = 38; //final real state 
                }
                break;

            case 38: //final real state
                tk = addTk(CT_REAL, line);
                char *sReal = createString(pStartCh, pCrtCh);
                tk->r = atof(sReal); 
                free(sReal);
                return tk;

            case 39: //seen digits.digits and an e or digits and an e
                if(ch == '-' || ch == '+')
                {
                    pCrtCh++;
                    state = 40;
                }
                else if(isdigit(ch))
                {
                    pCrtCh++;
                    state = 41;
                }
                else
                {
                    err("Wrong format real number");
                }
                break;

            case 40:  //saw sign
                if(isdigit(ch))
                {
                    pCrtCh++;
                    state = 42;
                }
                else
                {
                    err("Wrong format for exponent number");
                }
                break;

            case 41: //xx.xxExx
                if(isdigit(ch))
                {
                    pCrtCh++;
                }
                else
                {
                    state = 38; //final real state
                }
                break;

            case 42: //seen xx.xxE+/- or xxE+/- and at least a digit
                if(isdigit(ch))
                {
                    pCrtCh++;
                }
                else
                {
                    state = 38; //final real state
                }
                break;
                
        }
    }
}

void showTokens() {
    Token *tk = tokens;
    printf("\nLexical Analysis: Tokens List\n");
    
    while (tk != NULL) {
        printf("Line %d | %-12s | ", tk->line, tokenNames[tk->code]);

        switch (tk->code) {
            case ID:
                printf("Value: %s", tk->text);
                break;
            case CT_INT:
                printf("Value: %ld", tk->i);
                break;
            case CT_REAL:
                printf("Value: %g", tk->r);
                break;
            case CT_CHAR:
                printf("Value: '%c'", (char)tk->i);
                break;
            case CT_STRING:
                printf("Value: \"%s\"", tk->text);
                break;
            default:
                break;
        }
        printf("\n");
        tk = tk->next;
    }
}

char* loadFile(const char* fileName) {
    FILE* f = fopen(fileName, "rb");
    if (!f) {
        printf("Error: Could not open file %s\n", fileName);
        exit(1);
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) err("not enough memory");
    
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    
    fclose(f);
    return buffer;
}

void done() {
    Token *tk = tokens;
    while (tk != NULL) {
        Token *nextTk = tk->next; 
        
        if (tk->code == ID || tk->code == CT_STRING) {
            if (tk->text != NULL) {
                free(tk->text);
            }
        }
        
        free(tk); 
        tk = nextTk;
    }
    
    tokens = NULL;
    lastToken = NULL;
}

