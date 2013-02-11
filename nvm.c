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
#include <sys/stat.h>

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
  /* {{{ push body */
  if (stack_size >= STACK_SIZE){
    /* TODO: extend the stack */
    fprintf(stderr, "stack overflow!\n");
    exit(1);
  }

  BYTE op = PUSH;

  /* write to the file */
  fwrite(&op, sizeof op, 1, file_p);
  fwrite(&value, sizeof value, 1, file_p);

#ifdef DEBUG
  debug("push %d", value);
#endif
  pc++;

  stack[stack_size++] = value;
  /* }}} */
}

int pop(void)
{
  /* {{{ pop body */
  int value = stack[--stack_size];
  BYTE op = POP;

  /* write to the file */
  fwrite(&op, sizeof op, 1, file_p);
  fwrite(&value, sizeof value, 1, file_p);

#ifdef DEBUG
  debug("pop %d", value);
#endif
  pc++;

  return value;
  /* }}} */
}

void binop(BYTE op){
  /* {{{ binop body */
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
  /* }}} */
}

bool is_empty(void)
{
  /* {{{ is_empty body */
  return stack_size == 0 ? true : false;
  /* }}} */
}

void print_stack(void)
{
  /* {{{ print_stack body */
  unsigned i;
  for (i = 0; i < stack_size; i++){
    printf("item on stack: %d\n", stack[i]);
  }
  /* }}} */
}

void debug(const char *msg, ...)
{
  /* {{{ debug body */
  va_list ap;

  printf("%03d: ", pc);
  va_start(ap, msg);
  vprintf(msg, ap);
  printf("\n");
  va_end(ap);
  /* }}} */
}

nvm_t *nvm_init(const char *filename)
{
  /* {{{ nvm_init body */
  nvm_t *vm = malloc(sizeof(nvm_t));

  if (!vm){
    return NULL;
  }

  file_p = fopen(filename, "wb");

  BYTE major = NVM_VERSION_MAJOR;
  BYTE minor = NVM_VERSION_MINOR;
  BYTE patch = NVM_VERSION_PATCH;

  fwrite(&major, sizeof major, 1, file_p);
  fwrite(&minor, sizeof minor, 1, file_p);
  fwrite(&patch, sizeof patch, 1, file_p);

  return vm;
  /* }}} */
}

void nvm_destroy(nvm_t *vm)
{
  /* {{{ nvm_destroy body */
  /*fclose(file_p);*/
  free(vm);
  /* }}} */
}

int nvm_blastoff(nvm_t *vm)
{
  /* TODO: close it somewhere else */
  fclose(file_p);
  /* open the file */
  FILE *f = fopen("bytecode.nc", "rb");
  /* get the file size */
  struct stat st;
  stat("bytecode.nc", &st);
  /* array of our bytes */
  BYTE bytes[st.st_size];
  /* fetch the file */
  fread(bytes, st.st_size, sizeof(BYTE), f);

  /* an `int` is four bytes, but we're reading one byte at a time */
  BYTE byte_one, byte_two, byte_three, byte_four;
  /* this is the final number which is a result of connecting the four mentioned
   * above */
  int num;

  /* we need to get over the version */
  for (int i = 3; i < st.st_size; i++){
    switch (bytes[i]){
      case PUSH:
        /* extract the bytes */
        byte_one   =  0x00000000 ^ bytes[i + 1];
        byte_two   = (0x00000000 ^ bytes[i + 2]) << 2;
        byte_three = (0x00000000 ^ bytes[i + 3]) << 4;
        byte_four  = (0x00000000 ^ bytes[i + 4]) << 6;
        /* assemble the final number */
        num = 0x00000000 ^ byte_one ^ byte_two ^ byte_three ^ byte_four;
        /* skip over the bytes */
        i += 4;

        printf("push %d\n", num, num);
        break;
      case POP:
        /* extract the bytes */
        byte_one   =  0x00000000 ^ bytes[i + 1];
        byte_two   = (0x00000000 ^ bytes[i + 2]) << 2;
        byte_three = (0x00000000 ^ bytes[i + 3]) << 4;
        byte_four  = (0x00000000 ^ bytes[i + 4]) << 6;
        /* assemble the final number */
        num = 0x00000000 ^ byte_one ^ byte_two ^ byte_three ^ byte_four;
        /* skibytes over the bytes */
        i += 4;

        printf("pop %d\n", num, num);
        break;
      case BINARY_ADD:
        printf("add\n");
        break;
      case BINARY_SUB:
        printf("sub\n");
        break;
      case BINARY_MUL:
        printf("mul\n");
        break;
      case BINARY_DIV:
        printf("div\n");
        break;
      default:
        printf("unknown %d (%08X)\n", bytes[i], bytes[i]);
        /* you fail the game */
        return 1;
        break;
    }
  }

  fclose(f);

  return 0;
}

/*
 * Avantasia, Edguy, Iron Savior
 *
 */
