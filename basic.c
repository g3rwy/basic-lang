#include "bytecode.h"

#define LINESIZE 255
#define tokstr(type) tokens_str[type]
#define panic(s) puts(s); exit(-1);
#define print_node(x) if(IS_INT(x)){printf("%d\n",GET_INT(x));}else{printf("%s\n",GET_STR(x));}

char* getLine(FILE* file,char* buffer){
  return fgets(buffer,LINESIZE,file);
}

int contains(char c,const char* set){
  int i = 0;
  while(set[i]){
    if(set[i] == c) return 1;
    i++;
  }
  return 0;
}

int precedence(char c){
  switch(c){
    case '+':
    case '-':
      return 1;
    case '/':
    case '*':
    case '%':
      return 2;
    case '_':
      return 3;
    default:
      return -1;
  }
}

// Takes string and outputs what type is it in enum (or BAD for error)
TokenType strToType(const char* s,int len){
  int r = 0;
  const int length = strlen(s);

  while(r < KEYS_SIZE){
    if(strncmp(keywords[r],s,len) == 0) return (TokenType)(r + LET);
    r++;
  }
  
  r = 0;
  while(r < OPS_SIZE){
    if(s[0] == operators[r]) return (TokenType)(r + L_PAREN); 
    r++;
  }
  
  r = 0;
  if(isdigit(s[0])){
    ++r;
    while(r < length){ // If one character comes to be not digit, return error
      if(!isdigit(s[r])) break;
      r++;
    }
    return INT_LIT;
  }
  
  return IDENT;
}

Token getToken(int* start, const char* s, int line){ // something allocated here leaks memory, but its not a lot
  int len = 0;
  while(s[*start] == ' ' || s[*start] == '\t'){++(*start);}
  
  char end_line = 0;
  
  while(!contains(s[*start + len],ops_whitespace)){ 
    if(s[*start + len] == '\n'){end_line = 1; break;}
    ++len;
  }

  if(s[*start + len] == '\"'){ // STRING
    (*start)++;
    while(!contains(s[*start + len],"\";\n")){ len++; } len++;
    if(s[*start + len - 1] != '\"'){ printf("Unclosed string at: %d line\nGot %c instead\n",line,s[*start + len]); exit(-1);} // FIXME instead of quitting we should go back to main and free all the tokens
    char* value = malloc(len); // TODO check if im not wasting one char 
    value[len] = 0;
    Token result = (Token){line,STR_LIT,strncpy(value,s + (*start),len)};
    *start += len + 2;
    return result;
  }
  TokenType type = -1;
  if(s[*start + len] == '-' && isdigit(s[*start + len + 1])){
    type = INT_LIT;
    len += 2;
    while(isdigit(s[*start + len])){ len++; }
  }

  if(len == 0){ // Probably one letter so we need to increase the size
     ++len;
     if(s[*start] == ';') end_line = 1;
  }
  if(type == -1)
    type = strToType(s + (*start),len);

  char* value = 0;
  if(needsValue(type)){
    value = malloc(len+1);
    value[len] = 0;
  }
  
  Token result = (Token){line,type, needsValue(type) ? strncpy(value,s + (*start),len) : 0};
  *start += len;

  if(end_line){
    *start = -1;
    return (Token){line,END_LINE,NULL};
  }
  
  if(*start >= strlen(s)) *start = -1;
  
  return result;
}

void parseToRPN(Node exp[], int* exp_id){ // When _ is added to stack, it gets the wrong adress
  Node stack[64]; int stack_idx = 0;
  Node op_stack[32]; int op_idx = 0;

  for(int i = 0; i < *exp_id; i++){
    if(IS_INT(exp[i])){ // Number
      stack[stack_idx++] = exp[i]; continue;
    }
    if(IS_STR(exp[i])){
      // Find a way to find if it's a function or not
      // Idea 1. Leave " as first character for string literals
      const unsigned int len = strlen(GET_STR(exp[i]));
      if( GET_STR(exp[i])[len - 1] == '\"' ){
        puts("MUST be a string:");
        GET_STR(exp[i])[len-1] = 0; // removing \" at the end cause we dont need it anymore
        puts(GET_STR(exp[i]));
      }
      else{ // TODO implement adding function to operator stack (and popping it)
        puts("MUST be a function:");
        puts(GET_STR(exp[i]));
      }
      stack[stack_idx++] = exp[i]; continue;
    }

    if(contains(GET_VAR(exp[i]),operators)){ // Operator
      if(GET_VAR(exp[i]) == ','){
        if(op_idx != 0){
          while(GET_VAR(op_stack[op_idx-1]) != '('){
            stack[stack_idx++] = op_stack[op_idx-1];
            op_idx--;
            if(op_idx == 0) exit(-1);
          }
        }
        continue;
      }

      if(GET_VAR(exp[i]) != '(' && GET_VAR(exp[i]) != ')'){
        if(op_idx != 0){
          while(GET_VAR(op_stack[op_idx-1]) != '(' && precedence(GET_VAR(op_stack[op_idx-1])) >= precedence(GET_VAR(exp[i])) ){
            stack[stack_idx++] = op_stack[op_idx-1];
            op_idx--;
            if(op_idx == 0){ break;}
          }
        }
        op_stack[op_idx++] = exp[i];
        continue;
      }
      if(GET_VAR(exp[i]) == '('){
        op_stack[op_idx++] = exp[i];
        continue;
      }
      if(GET_VAR(exp[i]) == ')'){
        if(op_idx == 0){
          panic("Unmatched parenthesis 3");
        }
        while(GET_VAR(op_stack[op_idx-1]) != '('){
          stack[stack_idx++] = op_stack[op_idx-1];
          op_idx--;
          if(op_idx == 0){ panic("Unmatched parenthesis 2"); }
        }

        op_idx--;
        // TODO when function is added, check if there is a function at top of op_stack, if so push it to output
        continue;
      }
    }
    stack[stack_idx++] = exp[i]; // Symbol (variable or function
  }

  while(op_idx--){
    if(GET_VAR(op_stack[op_idx]) == '(') { panic("Unmatched parenthesis 4"); }
    stack[stack_idx++] = op_stack[op_idx];
  }

  for(int i = 0; i < stack_idx; i++){
    exp[i] = stack[i];
  }
  *exp_id = stack_idx;

}

void convertUnary(Node exp[], int exp_id){
  for(int i = 0;i < exp_id; i++){
    if(IS_VAR(exp[i])){ // Checking if first token is -
      if(i == 0 && GET_VAR(exp[i]) == '-'){ exp[i] = MAKE_VAR('_'); continue; }
    }
    if(IS_VAR(exp[i]) && IS_VAR(exp[i-1])){ // Checking if the - is after any operator (meaning its unary)
      if(contains(GET_VAR(exp[i-1]),"+-/*&%()") && GET_VAR(exp[i]) == '-'){
        exp[i] = MAKE_VAR('_');
      }
    }
  }
}

int main(int argc,char** argv){
  assert(EXPR + 1 == TOKENS_SIZE);

  if(argc == 1){
    puts("Please provide a file :(");
    return -1;
  }

  FILE* filep = fopen(argv[1], "r");
  STACK = malloc(sizeof(uint64_t) * 64);
  char line[LINESIZE] = {0};
  int line_count = 1;
  
  while(getLine(filep,line)) {
      int start = 0;
      int stat_type = 0;
      int expected_idx = 0;
      
      Token tokens[64] = {0}; int tokens_idx = 0;
      Node expression[64];    int exp_idx = 0;

      while(1){
        Token tok;

        if(start == 0){ // If its first token then we find what type of statement it might be
          tok = getToken(&start,line,line_count);
          while(stat_type < STAT_SIZE){ // Finding what statement this line is
            if(statement_expected[stat_type][expected_idx] == tok.type) break;
            stat_type++;
          }
        
          if(stat_type == STAT_SIZE){ 
            if(tok.value) free(tok.value); //FIXME prob should free all the values not just this one
            printf("Unrecognized statement: %s",line);
            exit(-1);
          }
          expected_idx++;
          tokens[tokens_idx++] = tok;
        }
      
        tok = getToken(&start,line,line_count);

        switch(statement_expected[stat_type][expected_idx]){
          case EXPR:
                if(tok.type == statement_expected[stat_type][expected_idx + 1]){
                  expected_idx++;
                  goto end_line;
                }

                if(tok.type == INT_LIT){ // Number
                  expression[exp_idx] = MAKE_INT(atoll(tok.value));
                  exp_idx++;
                }
                else if(tok.type == STR_LIT){ // String literal like "abc"
                  char* value = calloc(strlen(tok.value) + 1,sizeof(char));
                  strcpy(value,tok.value);
                  expression[exp_idx] = MAKE_STR(value);
                  exp_idx++;
                }
                else{ // Variable or Operator
                  char value;
                  if(tok.type >= L_PAREN && tok.type <= COMMA){
                    value = operators[tok.type - L_PAREN];
                  }
                  else{
                    if(strlen(tok.value) > 1){
                      expression[exp_idx] = MAKE_STR(tok.value);
                      exp_idx++; break;
                    }
                    value = tok.value[0];
                  }

                  expression[exp_idx] = MAKE_VAR(value); // It was double
                  exp_idx++;
                }
                break;

            default:
            end_line:
              if(statement_expected[stat_type][expected_idx] == tok.type){
                 puts("GOOD");
                 expected_idx++;
                 tokens[tokens_idx++] = tok;
              }
              else{
                 printf("Token: typ:%s val:%s\n",tokens_str[tok.type],tok.value ? tok.value : "null");
                 printf("Unexpected token at line: %d\n",line_count);
                 exit(-1);
              }
        }
        if(start  == -1) break;
      }
      convertUnary(expression,exp_idx);
      parseToRPN(expression,&exp_idx);
      // TODO Uncomment to work on bytecode
//      lineToBytecode(STACK, &sp, tokens, expression, 0);

      printf("Expr: ");
      for(int i = 0; i < exp_idx; i++){
        if(IS_INT(expression[i])){
          printf("%li ", GET_INT(expression[i]));
        }
        else if(IS_VAR(expression[i])){
          printf("%c ", GET_VAR(expression[i]));
        }
        else{
          printf("%s ", GET_STR(expression[i]));
          free((char*)expression[i]);
        }
      }

      while(tokens_idx-- > 0){
          if(tokens[tokens_idx].value) free(tokens[tokens_idx].value);
      }
    
      line_count++;
      break; // REMOVE WHEN STOPPED DEBUGGING
  }
  fclose(filep);
  free(STACK);
}

// TODO Optimize the parameters away (maybe with structs)
void lineToBytecode(uint64_t stack[], uint64_t *stack_idx, Token tokens[], int tok_idx, Node expr[], int exp_idx, int stat_type) {
  switch(stat_type){
    case 0:
      puts("declaring");
      char var_name = tokens[1].value[0];

      break;
    case 1: // Print variable
      puts("print");
      break;
    case 2: // Change value of variable
      puts("assign value");
      break;
    default:
      panic("Shouldn't happen");
  }
}

void exprToBytecode(uint64_t stack[], uint64_t *stack_idx, Node exp[],int exp_idx){
  int s_idx = *stack_idx;
  for(int i = 0;i < exp_idx; i++){
    if(IS_INT(exp[i])){
      data[data_idx] = GET_INT(exp[i]);
      stack[s_idx++] = CREATE_CHUNK(PUSH,data_idx);
      data_idx++;
    }
    else if(IS_STR(exp[i])){

    }
    else{ // Variable or Operator or function

    }
  }
  *stack_idx = s_idx;
}