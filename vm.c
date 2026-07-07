#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "vm.h"

char stack[STACK_SIZE];
char *SP = stack; 
char *stackAfter = stack + STACK_SIZE;

char globals[GLOBAL_SIZE];
int nGlobals = 0;

Instr *instructions = NULL;    
Instr *lastInstruction = NULL;

void pushd(double d)
{
    if(SP+sizeof(double)>stackAfter)err("out of stack");
    *(double*)SP=d;
    SP+=sizeof(double);
}

double popd()
{
    SP-=sizeof(double);
    if(SP<stack)err("not enough stack bytes for popd");
    return *(double*)SP;
}

void pusha(void *a)
{
    if(SP+sizeof(void*)>stackAfter)err("out of stack");
    *(void**)SP=a;
    SP+=sizeof(void*);
}

void *popa()
{
    SP-=sizeof(void*);
    if(SP<stack)err("not enough stack bytes for popa");
    return *(void**)SP;
}

void pushi(long int i)
{
    if(SP+sizeof(long int)>stackAfter)err("out of stack");
    *(long int*)SP=i;
    SP+=sizeof(long int);
}

long int popi()
{
    SP-=sizeof(long int);
    if(SP<stack)err("not enough stack bytes for popi");
    return *(long int*)SP;
}

void pushc(char c)
{
    if(SP+sizeof(char)>stackAfter)err("out of stack");
    *SP=c;
    SP+=sizeof(char);
}

char popc()
{
    SP-=sizeof(char);
    if(SP<stack)err("not enough stack bytes for popc");
    return *SP;
}

Instr *createInstr(int opcode)
{
    Instr *i;
    SAFEALLOC(i,Instr)
    i->opcode=opcode;
    return i;
}

void insertInstrAfter(Instr *after,Instr *i)
{
    i->next=after->next;
    i->last=after;
    after->next=i;
    if(i->next==NULL)lastInstruction=i;
}

Instr *addInstr(int opcode)
{
    Instr *i=createInstr(opcode);
    i->next=NULL;
    i->last=lastInstruction;
    if(lastInstruction)
    {
        lastInstruction->next=i;
    }
    else{instructions=i;}
    lastInstruction=i;
    return i;
}

Instr *addInstrAfter(Instr *after,int opcode)
{
    Instr *i=createInstr(opcode);
    insertInstrAfter(after,i);
    return i;
}

Instr *addInstrA(int opcode,void *addr)
{
    Instr *i = addInstr(opcode);
    i->args[0].addr = addr;
    return i;
}

Instr *addInstrI(int opcode,long int val){
    Instr *i = addInstr(opcode);
    i->args[0].i = val;
    return i;
}

Instr *addInstrII(int opcode,long int val1,long int val2) 
{
    Instr *i = addInstr(opcode);
    i->args[0].i = val1;
    i->args[1].i = val2;
    return i;
}

void deleteInstructionsAfter(Instr *start)
{
    if (start == NULL) return;

    Instr *cur = start->next;
    
    while (cur != NULL) 
    {
        Instr *to_delete = cur; 
        cur = cur->next;       
        free(to_delete);        
    }

    start->next = NULL;        
    lastInstruction = start;
}

void *allocGlobal(int size)
{
    void *p=globals+nGlobals;
    if(nGlobals+size>GLOBAL_SIZE)err("insufficient globals space");
    nGlobals+=size;
    return p;
}


void run(Instr *IP)
{
    long int iVal1, iVal2;
    double dVal1, dVal2;
    char cVal1, cVal2;
    char *aVal1;
    char *FP = 0, *oldSP; 
    
    SP = stack; 

    while(1){
        printf("%p/%d\t", IP, (int)(SP - stack)); 
        
        switch(IP->opcode){
            case O_CALL:
                aVal1 = IP->args[0].addr;
                printf("CALL\t%p\n", aVal1);
                pusha(IP->next);
                IP = (Instr*)aVal1;
                break;
                
            case O_CALLEXT:
                printf("CALLEXT\t%p\n", IP->args[0].addr);
                (*(void(*)())IP->args[0].addr)();
                IP = IP->next;
                break;
                
            case O_CAST_I_D:
                iVal1 = popi();
                dVal1 = (double)iVal1;
                printf("CAST_I_D\t(%ld -> %g)\n", iVal1, dVal1);
                pushd(dVal1);
                IP = IP->next;
                break;
                
            case O_DROP:
                iVal1 = IP->args[0].i;
                printf("DROP\t%ld\n", iVal1);
                if(SP - iVal1 < stack) err("not enough stack bytes");
                SP -= iVal1;
                IP = IP->next;
                break;
                
            case O_ENTER:
                iVal1 = IP->args[0].i;
                printf("ENTER\t%ld\n", iVal1);
                pusha(FP);
                FP = SP;
                SP += iVal1;
                IP = IP->next;
                break;
                
            case O_EQ_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("EQ_D\t(%g==%g -> %d)\n", dVal2, dVal1, dVal2 == dVal1);
                pushi(dVal2 == dVal1);
                IP = IP->next;
                break;
            
            case O_LESS_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("LESS_D\t(%g==%g -> %d)\n", dVal2, dVal1, dVal2 < dVal1);
                pushi(dVal2 < dVal1);
                IP = IP->next;
                break;

            case O_EQ_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("EQ_I\t(%ld==%ld -> %d)\n", iVal2, iVal1, iVal2 == iVal1);
                pushi(iVal2 == iVal1);
                IP = IP->next;
                break;
                
            case O_LESS_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("LESS_I\t(%ld==%ld -> %d)\n", iVal2, iVal1, iVal2 < iVal1);
                pushi(iVal2 < iVal1);
                IP = IP->next;
                break;

            
            case O_HALT:
                printf("HALT\n");
                return;
                
            case O_INSERT:
                iVal1 = IP->args[0].i; // iDst
                iVal2 = IP->args[1].i; // nBytes
                printf("INSERT\t%ld %ld\n", iVal1, iVal2);
                if(SP + iVal2 > stackAfter) err("out of stack");
                memmove(SP - iVal1 + iVal2, SP - iVal1, iVal1); // make room
                memmove(SP - iVal1, SP + iVal2, iVal2);         // dup
                SP += iVal2;
                IP = IP->next;
                break;
                
            case O_JT_I:
                iVal1 = popi();
                printf("JT\t%p\t(%ld)\n", IP->args[0].addr, iVal1);
                IP = iVal1 ? IP->args[0].addr : IP->next;
                break;

            case O_JF_I:
                iVal1 = popi();
                printf("JF\t%p\t(%ld)\n", IP->args[0].addr, iVal1);
                IP = iVal1 ? IP->next : IP->args[0].addr;
                break;

            case O_JMP:
                printf("JMP\t%p\n", IP->args[0].addr);
                IP = IP->args[0].addr;
                break;
                
            case O_LOAD:
                iVal1 = IP->args[0].i;
                aVal1 = popa();
                printf("LOAD\t%ld\t(%p)\n", iVal1, aVal1);
                if(SP + iVal1 > stackAfter) err("out of stack");
                memcpy(SP, aVal1, iVal1);
                SP += iVal1;
                IP = IP->next;
                break;
                
            case O_OFFSET:
                iVal1 = popi();
                aVal1 = popa();
                printf("OFFSET\t(%p+%ld -> %p)\n", aVal1, iVal1, aVal1 + iVal1);
                pusha(aVal1 + iVal1);
                IP = IP->next;
                break;
                
            case O_PUSHFPADDR:
                iVal1 = IP->args[0].i;
                printf("PUSHFPADDR\t%ld\t(%p)\n", iVal1, FP + iVal1);
                pusha(FP + iVal1);
                IP = IP->next;
                break;
                
            case O_PUSHCT_A:
                aVal1 = IP->args[0].addr;
                printf("PUSHCT_A\t%p\n", aVal1);
                pusha(aVal1);
                IP = IP->next;
                break;

            case O_PUSHCT_I:
                iVal1 = IP->args[0].i;
                printf("PUSHCT_I\t%ld\n", iVal1);
                pushi(iVal1);
                IP = IP->next;
                break;

            case O_PUSHCT_D:
                dVal1 = IP->args[0].d;
                printf("PUSHCT_D\t%g\n", dVal1);
                pushi(dVal1);
                IP = IP->next;
                break;

            case O_PUSHCT_C:
                cVal1 = (char)IP->args[0].i;  
                printf("PUSHCT_C\t'%c'\n", cVal1);
                pushc(cVal1);               
                IP = IP->next;
                break;
                
            case O_RET:
                iVal1 = IP->args[0].i; // sizeArgs
                iVal2 = IP->args[1].i; // sizeof(retType)
                printf("RET\t%ld %ld\n", iVal1, iVal2);
                oldSP = SP;
                SP = FP;
                FP = popa();
                IP = popa();
                if(SP - iVal1 < stack) err("not enough stack bytes");
                SP -= iVal1;
                memmove(SP, oldSP - iVal2, iVal2);
                SP += iVal2;
                break;
                
            case O_STORE:
                iVal1 = IP->args[0].i;
                if(SP - (sizeof(void*) + iVal1) < stack) err("not enough stack bytes for SET");
                aVal1 = *(void**)(SP - ((sizeof(void*) + iVal1)));
                printf("STORE\t%ld\t(%p)\n", iVal1, aVal1);
                memcpy(aVal1, SP - iVal1, iVal1);
                SP -= sizeof(void*) + iVal1;
                IP = IP->next;
                break;

            case O_ADD_I:
                iVal1 = popi(); 
                iVal2 = popi(); 
                printf("ADD_I\t(%ld + %ld -> %ld)\n", iVal2, iVal1, iVal2 + iVal1);
                pushi(iVal2 + iVal1);
                IP = IP->next;
                break;

            case O_SUB_I:
                iVal1 = popi(); 
                iVal2 = popi(); 
                printf("SUB_I\t(%ld - %ld -> %ld)\n", iVal2, iVal1, iVal2 - iVal1);
                pushi(iVal2 - iVal1);
                IP = IP->next;
                break;

            case O_MUL_I:
                iVal1 = popi(); 
                iVal2 = popi(); 
                printf("MUL_I\t(%ld * %ld -> %ld)\n", iVal2, iVal1, iVal2 * iVal1);
                pushi(iVal2 * iVal1);
                IP = IP->next;
                break;

            case O_DIV_I:
                iVal1 = popi(); 
                iVal2 = popi(); 
                if (iVal1 == 0) err("Division by zero in O_DIV_I");
                printf("DIV_I\t(%ld / %ld -> %ld)\n", iVal2, iVal1, iVal2 / iVal1);
                pushi(iVal2 / iVal1);
                IP = IP->next;
                break;
                
            case O_SUB_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("SUB_D\t(%g-%g -> %g)\n", dVal2, dVal1, dVal2 - dVal1);
                pushd(dVal2 - dVal1);
                IP = IP->next;
                break;

            case O_ADD_D:
                dVal1 = popd(); 
                dVal2 = popd(); 
                printf("ADD_D\t(%g + %g -> %g)\n", dVal2, dVal1, dVal2 + dVal1);
                pushd(dVal2 + dVal1);
                IP = IP->next;
                break;

            case O_MUL_D:
                dVal1 = popd(); 
                dVal2 = popd(); 
                printf("MUL_D\t(%g * %g -> %g)\n", dVal2, dVal1, dVal2 * dVal1);
                pushd(dVal2 * dVal1);
                IP = IP->next;
                break;

            case O_DIV_D:
                dVal1 = popd(); 
                dVal2 = popd(); 
                if (dVal1 == 0.0) err("Division by zero in O_DIV_D");
                printf("DIV_D\t(%g / %g -> %g)\n", dVal2, dVal1, dVal2 / dVal1);
                pushd(dVal2 / dVal1);
                IP = IP->next;
                break;

            case O_ADD_C:
                cVal1 = popc(); 
                cVal2 = popc(); 
                printf("ADD_C\t(%d + %d -> %d)\n", cVal2, cVal1, cVal2 + cVal1);
                pushc(cVal2 + cVal1);
                IP = IP->next;
                break;

            case O_CAST_D_I:
                dVal1 = popd();
                iVal1 = (long int)dVal1;
                printf("CAST_D_I\t(%g -> %ld)\n", dVal1, iVal1);
                pushi(iVal1);
                IP = IP->next;
                break;

            case O_CAST_C_I:
                cVal1 = popc();
                iVal1 = (long int)cVal1; // Cast the 1-byte char to an 8-byte integer
                printf("CAST_C_I\t(%d -> %ld)\n", cVal1, iVal1);
                pushi(iVal1);
                IP = IP->next;
                break;

            default:
                err("invalid opcode: %d", IP->opcode);
        }
    }
}

void put_i()
{
printf("#%ld\n",popi());
}


void mvTest()
{
    Instr *L1;
    int *v=allocGlobal(sizeof(long int));
    addInstrA(O_PUSHCT_A,v);
    addInstrI(O_PUSHCT_I,3);
    addInstrI(O_STORE,sizeof(long int));
    L1=addInstrA(O_PUSHCT_A,v);
    addInstrI(O_LOAD,sizeof(long int));
    addInstrA(O_CALLEXT,requireSymbol(symTable,"put_i")->addr);
    addInstrA(O_PUSHCT_A,v);
    addInstrA(O_PUSHCT_A,v);
    addInstrI(O_LOAD,sizeof(long int));
    addInstrI(O_PUSHCT_I,1);
    addInstr(O_SUB_I);
    addInstrI(O_STORE,sizeof(long int));
    addInstrA(O_PUSHCT_A,v);
    addInstrI(O_LOAD,sizeof(long int));
    addInstrA(O_JT_I,L1);
    addInstr(O_HALT);
}