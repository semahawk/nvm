/*
 *
 * nvm.c
 *
 * Created at:  02/09/2013 12:26:00 PM
 *
 * Author:  Szymon Urbaś <szymon.urbas@aol.com>
 *
 * License: the MIT license
 *
 */

/*
 * This is the main file that implements the Virtual Machine.
 *
 * This VM is stack based.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "nvm.h"
#include "grammar.h"

/* The Stack */
static int stack[STACK_SIZE];
/* number of values on the stack */
static unsigned stack_size = 0;
/* program counter, used only in the `debug` function */
static unsigned pc = 0;
/* file containing the bytecode */
static FILE *file_p;

void push(int value)
{
  if (stack_size >= STACK_SIZE){
    /* TODO: extend the stack */
    fprintf(stderr, "stack overflow!\n");
    exit(1);
  }

  unsigned op = PUSH;

  /* write to the file */
  fwrite(&op, sizeof op, 1, file_p);
  fwrite(&value, sizeof value, 1, file_p);

#ifdef DEBUG
  debug("push %d", value);
#endif
  pc++;

  stack[stack_size++] = value;
}

int pop(void)
{
  int value = stack[--stack_size];
  unsigned op = POP;

  /* write to the file */
  fwrite(&op, sizeof op, 1, file_p);
  fwrite(&value, sizeof value, 1, file_p);

#ifdef DEBUG
  debug("pop %d", value);
#endif
  pc++;

  return value;
}

void binop(unsigned op){
  int b = pop();
  int a = pop();
  int res;

  /* write to the file */
  fwrite(&op, sizeof(op), 1, file_p);

  switch (op){
    case BINARY_ADD:
      res = a + b;
#ifdef DEBUG
      debug("add");
#endif
      break;
    case BINARY_SUB:
      res = a - b;
#ifdef DEBUG
      debug("sub");
#endif
      break;
    case BINARY_MUL:
      res = a * b;
#ifdef DEBUG
      debug("mul");
#endif
      break;
    case BINARY_DIV:
      res = a / b;
#ifdef DEBUG
      debug("div");
#endif
      break;
  }

  pc++;

  push(res);
}

bool is_empty(void)
{
  return stack_size == 0 ? true : false;
}

void print_stack(void)
{
  unsigned i;
  for (i = 0; i < stack_size; i++){
    printf("item on stack: %d\n", stack[i]);
  }
}

void debug(const char *msg, ...)
{
  va_list ap;

  printf("%03d: ", pc);
  va_start(ap, msg);
  vprintf(msg, ap);
  printf("\n");
  va_end(ap);
}

int main(void)
{
  void *parser = ParseAlloc(malloc);
  file_p = fopen("bytecode.nc", "wb");

  /* input: (2 + 2) * 2 / 4 * (10 + 4) + 9 */
  Parse(parser, LPAREN, 0);
  Parse(parser, NUMBER, 2);
  Parse(parser, PLUS, 0);
  Parse(parser, NUMBER, 2);
  Parse(parser, RPAREN, 0);
  Parse(parser, TIMES, 0);
  Parse(parser, NUMBER, 2);
  Parse(parser, DIVIDE, 0);
  Parse(parser, NUMBER, 4);
  Parse(parser, TIMES, 0);
  Parse(parser, LPAREN, 0);
  Parse(parser, NUMBER, 10);
  Parse(parser, PLUS, 0);
  Parse(parser, NUMBER, 4);
  Parse(parser, RPAREN, 0);
  Parse(parser, PLUS, 0);
  Parse(parser, NUMBER, 9);
  Parse(parser, 0, 0);

  ParseFree(parser, free);
  fclose(file_p);

  return 0;
}

/*
 * Avantasia, Edguy
 *
 */
