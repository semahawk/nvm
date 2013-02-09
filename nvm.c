/*
 *
 * nvm.c
 *
 * Created at:  02/09/2013 12:26:00 PM
 *
 * Author:  Szymon Urbas <szymon.urbas@aol.com>
 *
 * License: the MIT license
 *
 */

/*
 * This the main (although files name may say something different) file that implements the Virtual Machine.
 *
 * This VM is stack based.
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
static unsigned pc = 1;

int main(void)
{
  void *parser = ParseAlloc(malloc);

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

  return 0;
}

void push(int value)
{
  if (stack_size >= STACK_SIZE){
    /* TODO: extend the stack */
    fprintf(stderr, "stack overflow!\n");
    exit(1);
  }

#ifdef DEBUG
  debug("push %d", value);
#endif
  pc++;

  stack[stack_size++] = value;
}

int pop(void)
{
  int value = stack[--stack_size];

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

  switch (op){
    case BIN_ADD:
      res = a + b;
#ifdef DEBUG
      debug("add %d %d", a, b);
#endif
      break;
    case BIN_SUB:
      res = a - b;
#ifdef DEBUG
      debug("sub %d %d", a, b);
#endif
      break;
    case BIN_MUL:
      res = a * b;
#ifdef DEBUG
      debug("mul %d %d", a, b);
#endif
      break;
    case BIN_DIV:
      res = a / b;
#ifdef DEBUG
      debug("div %d %d", a, b);
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

/*
 * Avantasia, Edguy
 *
 */
