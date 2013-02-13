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
#include <stdint.h>
#include <sys/stat.h>

#include "nvm.h"
#include "grammar.h"

/* The Stack */
static int stack[STACK_SIZE];
/* 'points' to the current value on the stack */
static unsigned stack_ptr = 0;

void push(uint16_t pc, int value)
{
  /* {{{ push body */
  if (stack_ptr >= STACK_SIZE){
    /* TODO: extend the stack */
    fprintf(stderr, "stack overflow!\n");
    exit(1);
  }

  stack[stack_ptr++] = value;
  /* }}} */
}

int pop(void)
{
  /* {{{ pop body */
  return stack[--stack_ptr];
  /* }}} */
}

void discard(uint16_t pc)
{
  /* {{{ discard body */
  stack[stack_ptr--] = 0;
  /* }}} */
}

void binop(uint16_t pc, BYTE op){
  /* {{{ binop body */
  int b = pop();
  int a = pop();
  int res;

  switch (op){
    case BINARY_ADD:
      res = a + b;
      break;
    case BINARY_SUB:
      res = a - b;
      break;
    case BINARY_MUL:
      res = a * b;
      break;
    case BINARY_DIV:
      res = a / b;
      break;
  }

  push(pc, res);
  /* }}} */
}

bool is_empty(void)
{
  /* {{{ is_empty body */
  return stack_ptr == 0 ? true : false;
  /* }}} */
}

void print_stack(void)
{
  /* {{{ print_stack body */
  unsigned i;

  if (stack_ptr == 0)
    printf("stack empty!\n");

  for (i = 0; i < stack_ptr; i++){
    printf("item on stack: %d\n", stack[i]);
  }
  /* }}} */
}

nvm_t *nvm_init(const char *filename)
{
  /* {{{ nvm_init body */
  nvm_t *vm = malloc(sizeof(nvm_t));

  if (!vm){
    return NULL;
  }

  vm->filename = filename;

  return vm;
  /* }}} */
}

void nvm_destroy(nvm_t *vm)
{
  /* {{{ nvm_destroy body */
  free(vm);
  /* }}} */
}

int nvm_blastoff(nvm_t *vm)
{
  /* {{{ nvm_blastoff body */
  /* open the file */
  FILE *f = fopen(vm->filename, "rb");
  /* get the file size */
  struct stat st;
  stat(vm->filename, &st);
  /* array of our bytes */
  BYTE bytes[st.st_size];
  /* fetch the file */
  fread(bytes, st.st_size, sizeof(BYTE), f);

  /* an `int` is four bytes, but we're reading one byte at a time */
  BYTE byte_one, byte_two, byte_three, byte_four;
  /* this is the final number which is a result of connecting the four mentioned
   * above */
  int value;
  /* program counter */
  uint16_t pc;

#if VERBOSE
  printf("## using NVM version %u.%u.%u ##\n\n", bytes[0], bytes[1], bytes[2]);
#endif

  /* we start from 3 to skip over the version */
  for (int i = 3; i < st.st_size; i++){
    /* extract the bytes */
    byte_one   = bytes[i];
    byte_two   = bytes[i + 1] << 2;
    /* assemble the final number */
    pc = byte_one ^ byte_two;
    /* skip over the bytes */
    i += 2;

    switch (bytes[i]){
      /* {{{ main op switch */
      case PUSH:
#if VERBOSE
        printf("%04x: push %d\n", pc, value);
#endif
        /* extract the bytes */
        byte_one   = bytes[i + 1];
        byte_two   = bytes[i + 2] << 2;
        byte_three = bytes[i + 3] << 4;
        byte_four  = bytes[i + 4] << 6;
        /* assemble the final number */
        value = byte_one ^ byte_two ^ byte_three ^ byte_four;
        /* skip over the bytes */
        i += 4;

        push(pc, value);
        break;
      case DISCARD:
#if VERBOSE
        printf("%04x: discard\n", pc);
#endif
        discard(pc);
        break;
      case BINARY_ADD:
#if VERBOSE
        printf("%04x: add\n", pc);
#endif
        binop(pc, bytes[i]);
        break;
      case BINARY_SUB:
#if VERBOSE
        printf("%04x: sub\n", pc);
#endif
        binop(pc, bytes[i]);
        break;
      case BINARY_MUL:
#if VERBOSE
        printf("%04x: mul\n", pc);
#endif
        binop(pc, bytes[i]);
        break;
      case BINARY_DIV:
#if VERBOSE
        printf("%04x: div\n", pc);
#endif
        binop(pc, bytes[i]);
        break;
      default:
        printf("%04x: error: unknown op: %d (%08X)\n", pc, bytes[i], bytes[i]);
        /* you failed the game */
        return 1;
        break;
      /* }}} */
    }
  }

  fclose(f);

  return 0;
  /* }}} */
}

/*
 * Avantasia, Edguy, Iron Savior
 *
 */
