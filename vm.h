#ifndef VM_H
#define VM_H

#define STACK_SIZE (32 * 1024)
#define GLOBAL_SIZE (32 * 1024)

//enum for the opcode of each instruction
enum {
    O_ADD_C, O_ADD_D, O_ADD_I, 
    O_SUB_D, O_SUB_I, O_MUL_D, O_MUL_I, O_DIV_D, O_DIV_I,
    O_CALL, O_CALLEXT, 
    O_CAST_I_D, O_CAST_D_I, O_CAST_C_I,
    O_DROP, 
    O_ENTER, 
    O_EQ_D, O_EQ_I, O_LESS_D, O_LESS_I,
    O_HALT, 
    O_INSERT, 
    O_JT_I, O_JF_I, O_JMP,
    O_LOAD, 
    O_OFFSET, 
    O_PUSHFPADDR, O_PUSHCT_A, O_PUSHCT_I, O_PUSHCT_C, O_PUSHCT_D,
    O_RET, 
    O_STORE
};

//represents a single instruction
typedef struct _Instr {
    int opcode;
    union {
        long int i;
        double d;
        void *addr;
    } args[2];
    struct _Instr *last, *next;
} Instr;

//double linked list that stores the instructions
extern Instr *instructions;
extern Instr *lastInstruction;

void *allocGlobal(int size);

//stack operations
void pushd(double d);
double popd();
void pusha(void *a);
void *popa();
void pushi(long int i);
long int popi();
void pushc(char c);
char popc();

// Functions to build and manipulate the instruction list 
Instr *createInstr(int opcode);
void insertInstrAfter(Instr *after, Instr *i);
Instr *addInstr(int opcode);
Instr *addInstrAfter(Instr *after, int opcode);
Instr *addInstrA(int opcode, void *addr);
Instr *addInstrI(int opcode, long int val);
Instr *addInstrII(int opcode, long int val1, long int val2);
void deleteInstructionsAfter(Instr *start);

// The main loop that evaluates the instructions
void run(Instr *IP);
void mvTest();

#endif 