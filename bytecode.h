#include "decl.h"

typedef uint32_t Operator;
typedef int64_t Data;

enum OperatorType{
    ADD_OP,SUB_OP,NEG_OP,MUL_OP,DIV_OP,MOD_OP,
    PRINT_OP,CALL,
    PUSH,POP,STORE};

uint64_t* STACK; // Contains both Operator and index to used data
uint64_t sp = 0; // top of stack
Data data[256]; uint32_t data_idx = 0;
int64_t var_val[64]; // TODO Change this size to something else later

#define CREATE_CHUNK(op,d) ((uint64_t)(((uint64_t)op << 32) | d))

void lineToBytecode(uint64_t stack[], uint64_t *sp, Token tokens[],int tok_idx, Node expr[],int exp_idx, int stat_type);
void exprToBytecode(uint64_t stack[], uint64_t *sp, Node exp[], int exp_idx);

const char* builtin_func[] = {"sin","rand","at"};
const int BUILTIN_FUNC_SIZE = sizeof(builtin_func) / sizeof(builtin_func[0]);

int isFunction(char* func){ // TODO In future use fast compare
  for(int i = 0; i < BUILTIN_FUNC_SIZE; i++){
    if(strcmp(func,builtin_func[i]) == 0) return 1;
  }
  return 0;
}


