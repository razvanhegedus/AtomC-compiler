#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "DA.h"


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
    showTokens(); // This will print your ID, CT_INT, CT_STRING, etc.

   

    crtTk = tokens; 

    printf("--- Starting Syntactic & Domain Analysis ---\n");

    // 2. Initialize the Global Domain State
    symTable = (Symbols*)malloc(sizeof(Symbols));
    if (symTable == NULL) {
        fprintf(stderr, "Fatal error: Not enough memory for global symbol table.\n");
        return EXIT_FAILURE;
    }
    initSymbols(symTable);
    
    crtDepth = 0;
    owner = NULL;
    crtGlobalMemorySize = 0; // Reset global memory tracking allocation offset

    // 3. Invoke the Parser
    if (unit()) {
        printf("Compilation successful!\n");
        printf("Total Global Memory Allocated: %d bytes\n", crtGlobalMemorySize);
        printSymbolTable();
    } else {
        printf("Compilation failed due to syntax or domain errors.\n");
    }

    // 4. Memory Cleanup
    // Free the dynamic array inside the symbol table container
    if (symTable->begin) {
        // Optional loop: You could loop through and free each individual Symbol* inside 
        // the array here if you want a perfect 0-byte leak profile.
        free(symTable->begin);
    }
    free(symTable);

    return EXIT_SUCCESS;
}