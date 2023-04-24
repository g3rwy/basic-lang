#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <inttypes.h>

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#define UNREACHABLE __builtin_unreachable();
#define needsValue(x) (x == STR_LIT || x == INT_LIT || x == IDENT)

char* getLine(FILE* file,char* buffer);
int contains(char c,const char* set);
int precedence(char c);

const char operators[] = "()+-*&%_;:,=";
const char ops_whitespace[] = "()+-*&%;:,=\" \t";
const int OPS_SIZE = (sizeof(operators) / sizeof(operators[0]));

const char* keywords[] = {"LET","FOR","PRINT","INPUT","NEXT","IF","JUMP"};
const int KEYS_SIZE = (sizeof(keywords) / sizeof(keywords[0]));

enum TokenType{ LET = 0, FOR, PRINT, INPUT, NEXT, IF ,JUMP, // 6  
                BAD ,INT_LIT, STR_LIT ,IDENT,END_LINE,// 11
                L_PAREN,R_PAREN,ADD,MINUS,MUL,AND,MOD,UNARY,SEMICOLON,COLON,COMMA,EQUAL, // 22
                WHATEVER,EXPR // 24
};
typedef enum TokenType TokenType;
const char* tokens_str[] = {"LET","FOR","PRINT","INPUT","NEXT","IF","JUMP","BAD","INT_LIT","STR_LIT","IDENT","END_LINE",
                "L_PAREN","R_PAREN","ADD","MINUS","MUL","AND","MOD","UNARY","SEMICOLON","COLON","COMMA","EQUAL","WHATEVER","EXPR"};
const int TOKENS_SIZE = sizeof(tokens_str) / sizeof(tokens_str[0]);

TokenType statement_expected[][6] = {
  {LET,IDENT,EQUAL,EXPR,END_LINE},
  {PRINT,EXPR,END_LINE},
  {IDENT,EQUAL,EXPR,END_LINE}
};
const int STAT_SIZE = sizeof(statement_expected) / sizeof(statement_expected[0]);

struct Token{
  uint line;
  TokenType type;
  char* value;
};
typedef struct Token Token;

typedef uintptr_t Node; // Tagged Pointer
// xxxxxxxx...VNI
// if all 0, then its string_lit
// if V is 0 and other are 0, then its variable name (char)
// if I is set then N is used as a sign for 61 bit int

// DISCLAIMER will work only on x86_64 probably

#define IS_INT(x) (((x) & 1) == 1)
#define IS_VAR(x) (((x) & 4) == 4 && ((x) & 1) == 0)
#define IS_STR(x) (!IS_INT(x) && !IS_VAR(x))

#define GET_STR(x) ((char*)(x))
#define GET_VAR(x) ((char)((x) >> 3))

#define MAKE_STR(x) ((Node)(x))
#define MAKE_VAR(x) ((Node) (((uint64_t)(x)) << 3 | 4))

void printbits(uint64_t v) {
  int i;
  for (i = 63; i >= 0; i--) putchar('0' + ((v >> i) & 1));
}

__always_inline int64_t GET_INT(Node x){
  uint64_t sign = (x & 2);
  if(sign) sign = sign | 1;
  sign = sign << 62;
  return (int64_t)((x >> 2) | sign);
}

__always_inline Node MAKE_INT(int64_t x){
  char sign = x < 0 ? 2 : 0;
  return (Node)((x << 2) | sign | 1);
}


void parseToRPN(Node exp[], int* exp_id);
void convertUnary(Node exp[], int exp_id);