#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char* content = loadFile(argv[1]);
    
    pCrtCh = content;
    line = 1;

    Token* tk;
    do {
        tk = getNextToken();
    } while (tk->code != END);

    showTokens();

    free(content);
    done();
    
    return 0;
}