#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "DA.h"
#include "vm.h"

extern int crtGlobalMemorySize;

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
    showTokens(); 

    crtTk = tokens; 

    printf("--- Starting Syntactic & Domain Analysis ---\n");

    symTable = (Symbols*)malloc(sizeof(Symbols));
    if (symTable == NULL) {
        fprintf(stderr, "Fatal error: Not enough memory for global symbol table.\n");
        return EXIT_FAILURE;
    }
    initSymbols(symTable);
    
    crtDepth = 0;
    owner = NULL;
    crtGlobalMemorySize = 0; 

    addExtFuncs(); 

    if (unit()) {
        printf("Compilation successful!\n");
        printf("Total Global Memory Allocated: %d bytes\n", crtGlobalMemorySize);
        printSymbolTable();

        printf("\n--- Generare Instructiuni VM (Test) ---\n");
        mvTest(); 
        
        printf("\n--- Start Executie VM ---\n");
        run(instructions);
        printf("\n--- Executie VM Terminata cu Succes ---\n");

    } else {
        printf("Compilation failed due to syntax or domain errors.\n");
    }

    if (symTable->begin) {
        free(symTable->begin);
    }
    free(symTable);

    return EXIT_SUCCESS;
}