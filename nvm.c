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
#include <string.h>
#include <sys/stat.h>

#include "nvm.h"
#include "grammar.h"

/*
 * FOS - First On Stack
 * SOS - Second On Stack
 * TOS - Third On Stack
 *
 */

/* {{{ static funtion declarations */
static void push(nvm_t *virtual_machine, INT value);
static INT pop(nvm_t *virtual_machine);
static void discard(nvm_t *virtual_machine);
static void rot_two(nvm_t *virtual_machine);
static void rot_three(nvm_t *virtual_machine);
static void binop(nvm_t *virtual_machine, BYTE operation);
static char *strdup(const char *p);
/* }}} */

/*
 * name:        push
 * description: pushes given <value> to the stack
 */
static void push(nvm_t *vm, INT value)
{
  /* {{{ push body */
  if (vm->stack_ptr >= STACK_SIZE){
    /* TODO: extend the stack */
    fprintf(stderr, "stack overflow!\n");
    exit(1);
  }

  vm->stack[vm->stack_ptr++] = value;
  /* }}} */
}

/*
 * name:        pop
 * description: returns the top-most value from the stack, and reduces it's size
 */
static INT pop(nvm_t *vm)
{
  /* {{{ pop body */
  return vm->stack[--vm->stack_ptr];
  /* }}} */
}

/*
 * name:        discard
 * description: changes the top-most value on the stack to 0, and reduces stack's size
 *
 *              It differs from `pop` in that it doesn't return anything, and
 *              it's an op in this VM.
 */
static void discard(nvm_t *vm)
{
  /* {{{ discard body */
  vm->stack[vm->stack_ptr--] = 0;
  /* }}} */
}

/*
 * name:        rot_two
 * description: swaps the two top-most values on stack
 */
static void rot_two(nvm_t *vm)
{
  /* {{{ rot_two body */
  /* First on Stack */
  INT FOS = pop(vm);
  /* Second on Stack */
  INT SOS = pop(vm);

  push(vm, FOS);
  push(vm, SOS);
  /* }}} */
}

/*
 * name:        rot_three
 * description: lifts second and third stack item one position up, moves top
 *              item down to position three
 */
static void rot_three(nvm_t *vm)
{
  /* {{{ rot_three body */
  INT FOS = pop(vm);
  INT SOS = pop(vm);
  INT TOS = pop(vm);

  push(vm, FOS);
  push(vm, TOS);
  push(vm, SOS);
  /* }}} */
}

/*
 * name:        store
 * description: stores a variable of a given <name>
 */
static void store(nvm_t *vm, char *name)
{
  /* {{{ store body */
  INT FOS = pop(vm);

  vm->vars[vm->vars_ptr].name = name;
  vm->vars[vm->vars_ptr].value = FOS;
  vm->vars_ptr++;
  /* }}}  */
}

/*
 * name:        get
 * description: pushes a value of a given variables <name> to the stack
 */
static void get(nvm_t *vm, char *name)
{
  /* {{{ get body */
  for (unsigned i = 0; i < vm->vars_ptr; i++){
    if (!strcmp(vm->vars[i].name, name)){
      push(vm, vm->vars[i].value);
      return;
    }
  }
  /* }}}  */
}

/*
 * name:        dup
 * description: duplicates the FOS
 */
static void dup(nvm_t *vm)
{
  /* {{{ dup body */
  INT FOS = pop(vm);

  push(vm, FOS);
  push(vm, FOS);
  /* }}} */
}

/*
 * name:        binop
 * description: pops a value twice, and performs a binary <operation> on those
 *              operands, and pushes the result
 */
static void binop(nvm_t *vm, BYTE op){
  /* {{{ binop body */
  INT FOS = pop(vm);
  INT SOS = pop(vm);
  INT result;

  switch (op){
    case BINARY_ADD:
      result = SOS + FOS;
      break;
    case BINARY_SUB:
      result = SOS - FOS;
      break;
    case BINARY_MUL:
      result = SOS * FOS;
      break;
    case BINARY_DIV:
      result = SOS / FOS;
      break;
  }

  push(vm, result);
  /* }}} */
}

void nvm_print_stack(nvm_t *vm)
{
  /* {{{ print_stack body */
  unsigned i;

  for (i = 0; i < vm->stack_ptr; i++){
    printf("item on stack: %d\n", vm->stack[i]);
  }
  /* }}} */
}

nvm_t *nvm_init(void *(*fn)(size_t), const char *filename)
{
  /* {{{ nvm_init body */
  nvm_t *vm = fn(sizeof(nvm_t));

  if (!vm){
    return NULL;
  }

  vm->filename = filename;
  vm->vars_ptr = 0;
  vm->stack_ptr = 0;

  return vm;
  /* }}} */
}

void nvm_destroy(void (*fn)(void *), nvm_t *vm)
{
  /* {{{ nvm_destroy body */
  fn(vm);
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
  INT value;
  /* program counter */
  uint16_t pc;
  /* used to retrieve variables names */
  char *string = NULL;
  /* additional counter (it's here because GCC complains about redefining it) */
  int j = 0;

#if VERBOSE
  printf("## using NVM version %u.%u.%u ##\n\n", bytes[0], bytes[1], bytes[2]);
#endif

  /* we start from 3 to skip over the version */
  for (int i = 3; i < st.st_size; i++){
    /* extract the bytes */
    byte_one = bytes[i];
    byte_two = bytes[i + 1] << 2;
    /* assemble the final number */
    pc = byte_one ^ byte_two;
    /* skip over the bytes */
    i += 2;

    switch (bytes[i]){
      /* {{{ main op switch */
      case NOP:
        /* that was tough */
#if VERBOSE
        printf("%04x: nop\n", pc);
#endif
        break;
      case PUSH:
        /* extract the bytes */
        byte_one   = bytes[i + 1];
        byte_two   = bytes[i + 2] << 2;
        byte_three = bytes[i + 3] << 4;
        byte_four  = bytes[i + 4] << 6;
        /* assemble the final number */
        value = byte_one ^ byte_two ^ byte_three ^ byte_four;
        /* skip over the bytes */
        i += 4;

#if VERBOSE
        printf("%04x: push %d\n", pc, value);
#endif

        push(vm, value);
        break;
      case DISCARD:
#if VERBOSE
        printf("%04x: discard\n", pc);
#endif
        discard(vm);
        break;
      case ROT_TWO:
#if VERBOSE
        printf("%04x: rot_two\n", pc);
#endif
        rot_two(vm);
        break;
      case ROT_THREE:
#if VERBOSE
        printf("%04x: rot_three\n", pc);
#endif
        rot_three(vm);
        break;
      case STORE:
        /* I don't want to declare any additional variables, so I'm using
         * 'byte_one', but calling it 'length' makes more sense */
#define length byte_one

        length = bytes[++i];
        string = malloc(length);

        if (!string){
          fprintf(stderr, "malloc: failed to allocate %d bytes.\n", length);
          return 2;
        }

        /* skip over the length byte */
        i++;
        /* getting the variables name, iterating through the <length> next
         * numbers */
        for (j = 0; j < length; j++){
          string[j] = bytes[i + j];
        }
        /* skip over the bytes */
        i += length - 1;

#undef length
#if VERBOSE
        printf("%04x: store %s\n", pc, string);
#endif
        store(vm, strdup(string));
        free(string);
        string = NULL;
        break;
      case GET:
        /* Some trick over here */
#define length byte_one
        length = bytes[++i];
        string = malloc(length);

        if (!string){
          fprintf(stderr, "malloc: failed to allocate %d bytes.\n", length);
          return 2;
        }

        /* skip over the length byte */
        i++;
        /* getting the variables name, iterating through the <length> next
         * numbers */
        for (j = 0; j < length; j++){
          string[j] = bytes[i + j];
        }
        /* skip over the bytes */
        i += length - 1;

#if VERBOSE
        printf("%04x: get %s\n", pc, string);
#endif
#undef length
        get(vm, strdup(string));
        free(string);
        string = NULL;
        break;
      case DUP:
#if VERBOSE
        printf("%04x: dup\n", pc);
#endif
        dup(vm);
        break;
      case BINARY_ADD:
#if VERBOSE
        printf("%04x: add\n", pc);
#endif
        binop(vm, bytes[i]);
        break;
      case BINARY_SUB:
#if VERBOSE
        printf("%04x: sub\n", pc);
#endif
        binop(vm, bytes[i]);
        break;
      case BINARY_MUL:
#if VERBOSE
        printf("%04x: mul\n", pc);
#endif
        binop(vm, bytes[i]);
        break;
      case BINARY_DIV:
#if VERBOSE
        printf("%04x: div\n", pc);
#endif
        binop(vm, bytes[i]);
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

static char *strdup(const char *p)
{
  /* {{{ strdup body */
  char *np = malloc(strlen(p) + 1);

  return np ? strcpy(np, p) : np;
  /* }}} */
}

/*
 * Rhapsody of Fire, Avantasia, Edguy, Iron Savior, Michael Schenker Group
 *
 * The Office, Family Guy
 *
 */
