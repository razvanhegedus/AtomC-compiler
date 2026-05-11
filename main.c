#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char* content = loadFile(argv[1]);
    if (!content) {
        fprintf(stderr, "Error: Could not load file %s\n", argv[1]);
        return 1;
    }
    
    pCrtCh = content;
    line = 1;

    Token* tk;
    do {
        tk = getNextToken();
    } while (tk->code != END);
   
    printf("--- Tokens Found ---\n");
    showTokens(); // This will print your ID, CT_INT, CT_STRING, etc.

   

    crtTk = tokens; 

    printf("--- Starting Syntactic Analysis ---\n");
    if (unit()) {
        printf("Success: The code is syntactically correct.\n");
    } else {

        printf("Error: Syntax analysis failed at the top level.\n");
    }

    // 6. Cleanup
    free(content);
    done(); // This should free the linked list of tokens
    
    return 0;
}