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
static void load_const(nvm_t *virtual_machine, INT value);
static INT pop(nvm_t *virtual_machine);
static void discard(nvm_t *virtual_machine);
static void rot_two(nvm_t *virtual_machine);
static void rot_three(nvm_t *virtual_machine);
static void binop(nvm_t *virtual_machine, BYTE operation);
static void load_name(nvm_t *virtual_machine, char *name);
static void prerun(nvm_t *virtual_machine);
static void call(nvm_t *vm, char *name);
static char *strdup(const char *p);
/* }}} */

/*
 * name:        load_const
 * description: pushes given <value> to the stack
 */
static void load_const(nvm_t *vm, INT value)
{
  /* {{{ load_const body */
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

  load_const(vm, FOS);
  load_const(vm, SOS);
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

  load_const(vm, FOS);
  load_const(vm, TOS);
  load_const(vm, SOS);
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
 * name:        load_name
 * description: pushes a value of a given variables <name> to the stack
 */
static void load_name(nvm_t *vm, char *name)
{
  /* {{{ load_name body */
  for (unsigned i = 0; i < vm->vars_ptr; i++){
    if (!strcmp(vm->vars[i].name, name)){
      load_const(vm, vm->vars[i].value);
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

  load_const(vm, FOS);
  load_const(vm, FOS);
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

  load_const(vm, result);
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

/*
 * name:        prerun
 * description: mainly used to search for functions and store them before
 *              running the bytecode (later it would probably also check for
 *              codes validity and some other stuff)
 */
static void prerun(nvm_t *vm)
{
  /* {{{ prerun body */
  char *name;
  int i, j;
  BYTE length;
  /* search for the BF (Begin Functions) byte */
  for (i = 0; i < vm->bytes_count; i++){
    if (vm->bytes[i] == BEGIN_FN){
      /* found it */
      vm->functions_offset = i;
      break;
    }
  }

  /* start from 1 to skip over the BF byte */
  for (i = 1; i < vm->bytes_count - vm->functions_offset; i++){
#define off i + vm->functions_offset
    if (vm->bytes[off] == FN_START){
      /* skip over FN_START */
      i++;
      length = vm->bytes[off];
      name = malloc(length);
      /* skip over the length */
      i++;
      /* get the name */
      for (j = 0; j < length; j++){
        name[j] = vm->bytes[off + j];
        vm->funcs[vm->funcs_ptr].name = name;
        vm->funcs[vm->funcs_ptr].offset = i + length;
      }
      vm->funcs_ptr++;
    }
    /* skip over the bytes */
    i += length + 1;
#undef off
  }
  /* }}} */
}

/*
 * name:        call
 * description: calls a function
 * parameters:  name - name of the function
 */
static void call(nvm_t *vm, char *name)
{
  /* {{{ call body */
  unsigned func, i = vm->functions_offset;
  int found = 0;
  /* search for the function */
  for (func = 0; func < vm->funcs_ptr; func++){
    /* found it */
    if (!strcmp(name, vm->funcs[func].name)){
      found = 1;
      break;
    }
  }

  if (!found){
    printf("function not found\n");
    return;
  }

  /* obtain the opcodes for the function */
  for (i += vm->funcs[func].offset; vm->bytes[i] != FN_END; i++){
    /* every loop under vm->bytes[i] there is next opcode */
    printf("%02X ", vm->bytes[i]);
  }
  printf("\n");
  /* }}} */
}

nvm_t *nvm_init(void *(*fn)(size_t), const char *filename)
{
  /* {{{ nvm_init body */
  nvm_t *vm = fn(sizeof(nvm_t));

  if (!vm){
    return NULL;
  }

  vm->filename         = filename;
  vm->bytes            = NULL;
  vm->vars_ptr         = 0;
  vm->stack_ptr        = 0;
  vm->functions_offset = -1;

  return vm;
  /* }}} */
}

void nvm_destroy(void (*fn)(void *), nvm_t *vm)
{
  /* {{{ nvm_destroy body */
  fn(vm->bytes);
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
  /* setting VMs bytes */
  vm->bytes = malloc(st.st_size);
  vm->bytes_count = st.st_size;
  /* fetch the file */
  fread(vm->bytes, st.st_size, sizeof(BYTE), f);

  /* start the pre-run (search for functions, store them, etc.) */
  prerun(vm);

  /* an `int` is four bytes, but we're reading one byte at a time */
  BYTE byte_one, byte_two, byte_three, byte_four;
  /* this is the final number which is a result of connecting the four mentioned
   * above */
  INT value;
  /* used to retrieve variables names */
  char *string = NULL;
  /* additional counter (it's here because GCC complains about redefining it) */
  int j = 0;

#if VERBOSE
  printf("## using NVM version %u.%u.%u ##\n\n", vm->bytes[0], vm->bytes[1], vm->bytes[2]);
#endif

  /* we start from 3 to skip over the version
     we end   at functions offset */
  for (int i = 3; i < vm->functions_offset; i++){
    /* increment the program counter */
    vm->ip++;
    switch (vm->bytes[i]){
      /* {{{ main op switch */
      case NOP:
        /* that was tough */
#if VERBOSE
        printf("%04x: nop\n", vm->ip);
#endif
        break;
      case LOAD_CONST:
        /* extract the bytes */
        byte_one   = vm->bytes[i + 1];
        byte_two   = vm->bytes[i + 2] << 2;
        byte_three = vm->bytes[i + 3] << 4;
        byte_four  = vm->bytes[i + 4] << 6;
        /* assemble the final number */
        value = byte_one ^ byte_two ^ byte_three ^ byte_four;
        /* skip over the bytes */
        i += 4;

#if VERBOSE
        printf("%04x: push\t(%d)\n", vm->ip, value);
#endif

        load_const(vm, value);
        break;
      case DISCARD:
#if VERBOSE
        printf("%04x: discard\n", vm->ip);
#endif
        discard(vm);
        break;
      case ROT_TWO:
#if VERBOSE
        printf("%04x: rot_two\n", vm->ip);
#endif
        rot_two(vm);
        break;
      case ROT_THREE:
#if VERBOSE
        printf("%04x: rot_three\n", vm->ip);
#endif
        rot_three(vm);
        break;
      case STORE:
        /* I don't want to declare any additional variables, so I'm using
         * 'byte_one', but calling it 'length' makes more sense */
#define length byte_one

        length = vm->bytes[++i];
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
          string[j] = vm->bytes[i + j];
        }
        /* skip over the bytes */
        i += length - 1;

#undef length
#if VERBOSE
        printf("%04x: store\t(%s)\n", vm->ip, string);
#endif
        store(vm, strdup(string));
        free(string);
        string = NULL;
        break;
      case LOAD_NAME:
        /* Some trick over here */
#define length byte_one
        length = vm->bytes[++i];
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
          string[j] = vm->bytes[i + j];
        }
        /* skip over the bytes */
        i += length - 1;

#if VERBOSE
        printf("%04x: get\t(%s)\n", vm->ip, string);
#endif
#undef length
        load_name(vm, strdup(string));
        free(string);
        string = NULL;
        break;
      case DUP:
#if VERBOSE
        printf("%04x: dup\n", vm->ip);
#endif
        dup(vm);
        break;
      case BINARY_ADD:
#if VERBOSE
        printf("%04x: add\n", vm->ip);
#endif
        binop(vm, vm->bytes[i]);
        break;
      case BINARY_SUB:
#if VERBOSE
        printf("%04x: sub\n", vm->ip);
#endif
        binop(vm, vm->bytes[i]);
        break;
      case BINARY_MUL:
#if VERBOSE
        printf("%04x: mul\n", vm->ip);
#endif
        binop(vm, vm->bytes[i]);
        break;
      case BINARY_DIV:
#if VERBOSE
        printf("%04x: div\n", vm->ip);
#endif
        binop(vm, vm->bytes[i]);
        break;
      case CALL:
#define length byte_one
        length = vm->bytes[++i];
        string = malloc(length);
        /* skip over the length byte */
        i++;
        /* get the name */
        for (j = 0; j < length; j++){
          string[j] = vm->bytes[i + j];
        }
        printf("%04x: call\t(%s)\n", vm->ip, string);
        i += length + 1;
#undef  length
        call(vm, strdup(string));
        free(string);
        break;
      default:
        printf("%04x: error: unknown op: %d (%08X)\n", vm->ip, vm->bytes[i], vm->bytes[i]);
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
