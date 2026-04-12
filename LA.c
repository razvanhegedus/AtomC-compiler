#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

enum{
    ID = 1,
    CT_INT,
    CT_REAL,
    CT_CHAR,
    CT_STRING,
    BREAK,
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

typedef struct _Token
{
    int code;
    union{
        char* text; //used for ID, CT_STRING
        long int i; //CT_INT, CT_CHAR
        double r; //CT_REAL
    };
    int line; //input file line
    struct _Token* next;
}Token;

Token* lastToken = NULL; //tail of the list
Token* tokens = NULL; //head of the lsit

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
char* pCrtCh;
int line;

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
                break;

            case 1:
                if(isalpha(ch) || ch == '_') 
                {
                    pCrtCh++; //consume the character
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
                    err("Unterminated string constant at line %d", line);
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
        }
    }
}


int main() {
    return 0;
}