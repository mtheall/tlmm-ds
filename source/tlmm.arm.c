#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#ifdef TLMM_HAS_IO
#include <stdio.h>
#endif
#include <stdio.h>
#include "tlmm.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TLMM_STACK_MAX
#define TLMM_STACK_MAX 256
#endif

typedef enum {
  SYM_ADD    = '+',
  SYM_COS    = 'c',
  SYM_DIV    = '/',
  SYM_LPAREN = '(',
  SYM_MULT   = '*',
  SYM_POWER  = '^',
  SYM_RPAREN = ')',
  SYM_SIN    = 's',
  SYM_SQRT   = 'S',
  SYM_SUB    = '-',
  SYM_TAN    = 't',
  SYM_VAL    = '#',
  SYM_X      = 'x',
} symbol;

typedef struct {
  float stack[TLMM_STACK_MAX];
  int   top;
} reg_stack;

typedef struct {
  symbol stack[TLMM_STACK_MAX];
  int    top;
} sym_stack;

typedef void(*instruction)(reg_stack*, float, reg_stack*);

typedef struct {
  unsigned int magic;
  unsigned int codeSize;
  unsigned int dataSize;
} tlmmHeader;
static const char* MAGIC = "TLMM";

struct tlmmProgram {
  unsigned int codeSize;
  unsigned int dataSize;
  symbol       *codeSeg;
  float        *dataSeg;
  char         *eqn;
};

// interface functions
tlmmProgram* tlmmInitProgram      (void);
tlmmReturn   tlmmTerminateProgram (tlmmProgram **prog);
#ifdef TLMM_COMPILE
tlmmReturn   tlmmParseProgram     (tlmmProgram *prog, const char *program);
#endif
tlmmReturn   tlmmLoadProgramBinary(tlmmProgram *prog, const void *data, size_t size);
float        tlmmGetValue         (tlmmProgram *prog, float ref);
const char*  tlmmGetEquation      (tlmmProgram *prog);
#ifdef TLMM_HAS_IO
tlmmReturn   tlmmLoadProgram      (tlmmProgram *prog, const char *filename);
tlmmReturn   tlmmSaveProgram      (tlmmProgram *prog, const char *filename);
#endif

// helper functions
static inline void  reg_push(reg_stack *stack, float val) {
  stack->stack[stack->top++] = val;
}
static inline float reg_pop (reg_stack *stack) {
  return stack->stack[--stack->top];
}
static inline float reg_top (reg_stack *stack) {
  return stack->stack[stack->top-1];
}
static inline void  sym_push(sym_stack *stack, symbol sym) {
  stack->stack[stack->top++] = sym;
}
static inline symbol sym_pop (sym_stack *stack) {
  return stack->stack[--stack->top];
}
static inline symbol sym_top (sym_stack *stack) {
  return stack->stack[stack->top-1];
}

#define PUSH(x) reg_push(stack, x)
#define POP()   reg_pop(stack)
#define REG()   reg_pop(regs)

static inline int getPrecedence(symbol s) {
  switch(s) {
    case SYM_COS:
    case SYM_SIN:
    case SYM_SQRT:
    case SYM_TAN:    return 4;
    case SYM_POWER:  return 3;
    case SYM_DIV:
    case SYM_MULT:   return 2;
    case SYM_ADD:
    case SYM_SUB:    return 1;
    default:         return 0;
  }
}

static inline int isOperator(symbol s) {
  return getPrecedence(s) > 0;
}

static inline int isLeftAssociative(symbol s) {
  switch(s) {
    case SYM_POWER: return 0; // exponentiation is right-associative
    default:        return 1;
  }
}

static inline void tlmmClearProgram(tlmmProgram *prog) {
  if(prog != NULL) {
    free(prog->codeSeg);
    free(prog->dataSeg);
    free(prog->eqn);
    memset(prog, 0, sizeof(*prog));
  }
}

#define INSTRUCTION(fn) void tlmmFunc##fn(reg_stack *stack, float x, reg_stack *regs)
INSTRUCTION(Add)  { PUSH(POP() + POP()); }
INSTRUCTION(Cos)  { PUSH(cos(POP())); }
INSTRUCTION(Div)  {
  float op2 = POP(); // operands pop in reverse order
  float op1 = POP();
  PUSH(op1 / op2);
}
INSTRUCTION(Mul)  { PUSH(POP() * POP()); }
INSTRUCTION(Pow)  {
  float op2 = POP(); // operands pop in reverse order
  float op1 = POP();
  PUSH(pow(op1, op2));
}
INSTRUCTION(Sin)  { PUSH(sin(POP())); }
INSTRUCTION(Sqrt) { PUSH(sqrt(POP())); }
INSTRUCTION(Sub)  {
  float op2 = POP(); // operands pop in reverse order
  float op1 = POP();
  PUSH(op1 - op2);
}
INSTRUCTION(Tan)  { PUSH(tan(POP())); }
INSTRUCTION(Val)  { PUSH(REG()); }
INSTRUCTION(X)    { reg_push(stack, x); }

static instruction g_Instructions[256];

tlmmProgram* tlmmInitProgram(void) {
  static int loaded = 0;
  tlmmProgram *prog = (tlmmProgram*)malloc(sizeof(tlmmProgram));

  if(!loaded) { // let's initialize the instructions
    g_Instructions[SYM_ADD]   = tlmmFuncAdd;
    g_Instructions[SYM_COS]   = tlmmFuncCos;
    g_Instructions[SYM_DIV]   = tlmmFuncDiv;
    g_Instructions[SYM_MULT]  = tlmmFuncMul;
    g_Instructions[SYM_POWER] = tlmmFuncPow;
    g_Instructions[SYM_SIN]   = tlmmFuncSin;
    g_Instructions[SYM_SQRT]  = tlmmFuncSqrt;
    g_Instructions[SYM_SUB]   = tlmmFuncSub;
    g_Instructions[SYM_TAN]   = tlmmFuncTan;
    g_Instructions[SYM_VAL]   = tlmmFuncVal;
    g_Instructions[SYM_X]     = tlmmFuncX;
  }

  if(prog != NULL)
    memset(prog, 0, sizeof(*prog));

  return prog;
}

tlmmReturn tlmmTerminateProgram(tlmmProgram **prog) {
  if(prog != NULL) {
    tlmmClearProgram(*prog);
    free(*prog);
    *prog = NULL;
    return TLMM_SUCCESS;
  }
  return TLMM_FAILURE;
}

const char* tlmmGetEquation(tlmmProgram *prog) {
  return prog != NULL ? prog->eqn : NULL;
}

#ifdef TLMM_HAS_IO
tlmmReturn tlmmLoadProgram(tlmmProgram *prog, const char *filename) {
  long size;
  char *data;
  FILE* fp;
  tlmmReturn rc = TLMM_SUCCESS;

  if(prog == NULL || filename == NULL || (fp = fopen(filename, "rb")) == NULL)
    return TLMM_FAILURE;

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  rewind(fp);
  data = (char*)malloc(size+1);
  if(data != NULL) {
    fread(data, 1, size, fp);
    data[size] = 0;
    if((rc = tlmmLoadProgramBinary(prog, data, size)) == TLMM_FAILURE)
      rc = tlmmParseProgram(prog, data);
    free(data);
  }
  else
    rc = TLMM_FAILURE;

  fclose(fp);
  return rc;
}

tlmmReturn tlmmSaveProgram(tlmmProgram *prog, const char *filename) {
  tlmmHeader header;
  tlmmReturn rc = TLMM_SUCCESS;
  FILE *fp;

  memcpy(&header.magic, MAGIC, 4);
  header.codeSize = prog->codeSize;
  header.dataSize = prog->dataSize;

  if(prog == NULL || filename == NULL || (fp = fopen(filename, "wb")) == NULL)
    return TLMM_FAILURE;

  fwrite(&header, sizeof(header), 1, fp);
  fwrite(prog->codeSeg, sizeof(symbol)*prog->codeSize, 1, fp);
  fwrite(prog->dataSeg, sizeof(float)*prog->dataSize, 1, fp);
  fprintf(fp, "%s", prog->eqn);
  fclose(fp);

  return rc;
}
#endif /* TLMM_HAS_IO */

tlmmReturn tlmmLoadProgramBinary(tlmmProgram *prog, const void *_data, size_t sz) {
  char *data = (char*)_data;
  tlmmHeader *header = (tlmmHeader*)data;

  if(prog == NULL || data == NULL || sz == 0 || memcmp(&header->magic, MAGIC, 4) != 0)
    return TLMM_FAILURE;

  tlmmClearProgram(prog);
  prog->codeSize = header->codeSize;
  prog->dataSize = header->dataSize;

  data += sizeof(tlmmHeader);

  prog->codeSeg = (symbol*)malloc(sizeof(symbol[prog->codeSize]));
  prog->dataSeg = (float*)malloc(sizeof(float[prog->dataSize]));
  prog->eqn     = strdup(data+sizeof(symbol)*prog->codeSize+sizeof(float)*prog->dataSize);
  if(prog->eqn == NULL || prog->codeSeg == NULL || prog->dataSeg == NULL) {
    tlmmClearProgram(prog);
    return TLMM_FAILURE;
  }

  return TLMM_SUCCESS;
}

#ifdef TLMM_COMPILE
tlmmReturn tlmmConvertRPN(sym_stack *syms) {
  sym_stack out, ops;
  int i;

  memset(&out, 0, sizeof(out));
  memset(&ops, 0, sizeof(ops));

  for(i = 0; i < syms->top; i++) {
    symbol sym = syms->stack[i];

    // always push ( onto ops
    if(sym == SYM_LPAREN)
      sym_push(&ops, sym);
    // always push values onto output
    else if(sym == SYM_X || sym == SYM_VAL)
      sym_push(&out, sym);
    // found a ) so close its (
    else if(sym == SYM_RPAREN) {
      // copy the ops over to output
      while(ops.top != 0 && sym_top(&ops) != SYM_LPAREN)
        sym_push(&out, sym_pop(&ops));

      // erase the (
      if(ops.top != 0)
        sym_pop(&ops);
      // whoops we didn't find the (
      else
        return TLMM_PARSE_ERROR;
    }
    // everything else should be ops
    else {
      if(!isOperator(sym))
        return TLMM_PARSE_ERROR;

      // if the ops is empty, or top is (, then just add this op
      if(ops.top == 0 || sym_top(&ops) == SYM_LPAREN)
        sym_push(&ops, sym);

      // if current precedence is higher than ops top, push onto ops
      else if(getPrecedence(sym) > getPrecedence(sym_top(&ops)))
        sym_push(&ops, sym);

      // if they have equal precedence, check for associativity
      else if(getPrecedence(sym) == getPrecedence(sym_top(&ops))) {
        // left associative means bring old op from ops to output
        if(isLeftAssociative(sym))
          sym_push(&out, sym_pop(&ops));
        // either case, push the current op onto ops
        sym_push(&ops, sym);
      }

      else {
        // lower precedence, so start moving stuff from ops to output
        while(ops.top != 0 && sym_top(&ops) != SYM_LPAREN && getPrecedence(sym) < getPrecedence(sym_top(&ops)))
          sym_push(&out, sym_pop(&ops));
        // finally, push our op onto ops
        sym_push(&ops, sym);
      }
    }
  }

  // reached the end, so pop everything from ops and push onto output
  while(ops.top != 0)
    sym_push(&out, sym_pop(&ops));

  // overwrite the input with out output
  memcpy(syms, &out, sizeof(out));

  return TLMM_SUCCESS;
}

tlmmReturn tlmmParseProgram(tlmmProgram* prog, const char* program) {
  const char* p = program;
  int found, i;
  sym_stack syms;
  reg_stack regs;

  memset(&syms, 0, sizeof(syms));
  memset(&regs, 0, sizeof(regs));

  while(*p) {
    found = 0;
    switch(tolower((int)*p)) {
      case SYM_LPAREN:
      case SYM_RPAREN:
      case SYM_POWER:
      case SYM_MULT:
      case SYM_DIV:
      case SYM_ADD:
      case SYM_SUB:
      case SYM_X:
        sym_push(&syms, (symbol)tolower((int)(*p++)));
        break;
      case SYM_SIN:
        if(strncasecmp(p, "sin", 3) == 0) {
          sym_push(&syms, SYM_SIN);
          p += 3;
        }
        else if(strncasecmp(p, "sqrt", 4) == 0) {
          sym_push(&syms, SYM_SQRT);
          p += 4;
        }
        else
          return TLMM_PARSE_ERROR;
        break;
      case SYM_COS:
        if(strncasecmp(p, "cos", 3) == 0) {
          sym_push(&syms, SYM_COS);
          p += 3;
        }
        else
          return TLMM_PARSE_ERROR;
        break;
      case SYM_TAN:
        if(strncasecmp(p, "tan", 3) == 0) {
          sym_push(&syms, SYM_TAN);
          p += 3;
        }
        else
          return TLMM_PARSE_ERROR;
        break;
      default:
        if((*p >= '0' && *p <= '9') || (*p == '.' && (found = 1))) {
          reg_push(&regs, atof(p++));
          sym_push(&syms, SYM_VAL);
          while((*p >= '0' && *p <= '9') || (!found && *p == '.' && (found = 1)))
            p++;
          break;
        }
        else if(*p == ' ' || *p == '\t' || *p == '\n')
          p++;
        else
          return TLMM_PARSE_ERROR;
    }
  }

  // if we've got here, we've successfully parsed the program
  tlmmConvertRPN(&syms);
  tlmmClearProgram(prog);

  prog->codeSize = syms.top;
  prog->dataSize = regs.top;
  prog->codeSeg  = (symbol*)malloc(sizeof(symbol[syms.top]));
  prog->dataSeg  = (float*)malloc(sizeof(float[regs.top]));
  prog->eqn      = strdup(program);

  if(prog->eqn == NULL || prog->codeSeg == NULL || prog->dataSeg == NULL) {
    tlmmClearProgram(prog);
    return TLMM_FAILURE;
  }

  memcpy(prog->codeSeg, syms.stack, sizeof(symbol)*syms.top);

  // we want to copy the data in 'stack' order
  for(i = 0; regs.top != 0; i++)
    prog->dataSeg[i] = reg_pop(&regs);
  
  return TLMM_SUCCESS;
}
#endif/*TLMM_COMPILE*/

float tlmmGetValue(tlmmProgram *prog, float ref) {
  reg_stack regs, stack;
  unsigned int i;

  regs.top = prog->dataSize;
  memcpy(regs.stack, prog->dataSeg, sizeof(float)*prog->dataSize);
  memset(&stack, 0, sizeof(stack));

  for(i = 0; i < prog->codeSize; i++)
    g_Instructions[(int)prog->codeSeg[i]](&stack, ref, &regs);

  return reg_top(&stack);
}

#ifdef __cplusplus
}

#ifndef TLMM_LEAN
namespace tlmm {
  Program::Program()  { prog = tlmmInitProgram(); }
  Program::~Program() { tlmmTerminateProgram(&prog); }

#ifdef TLMM_HAS_IO
  tlmmReturn Program::Load (const char *filename) {
    return tlmmLoadProgram(prog, filename);
  }
  tlmmReturn Program::Save (const char *filename) {
    return tlmmSaveProgram(prog, filename);
  }
#endif

#ifdef TLMM_COMPILE
  tlmmReturn Program::Parse(const char *program) {
    return tlmmParseProgram(prog, program);
  }
#endif

  float Program::GetValue(float ref) {
    return tlmmGetValue(prog, ref);
  }
  const char* Program::GetEquation() {
    return tlmmGetEquation(prog);
  }
}
#endif /* TLMM_LEAN */
#endif /* __cplusplus */
