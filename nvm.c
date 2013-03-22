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
static void dispatch(nvm_t *vm);
static char *strdup(const char *p);
/* }}} */

/* to make the output nicer */
static unsigned shiftwidth = 1;
#define shiftright() shiftwidth += 2
#define shiftleft() shiftwidth -= 2
#define print_spaces() do {\
  unsigned counter = 0;\
  for (; counter < shiftwidth; counter++)\
    printf(" ");\
} while (0);

/*
 * name:        load_const
 * description: pushes given <value> to the stack
 */
static void load_const(nvm_t *vm, INT value)
{
  /* {{{ load_const body */
  /* check for overflow */
  if (vm->stack.ptr >= vm->stack.size){
    vm->stack.size += 10;
    vm->stack.stack = realloc(vm->stack.stack, vm->stack.size);
  }

  vm->stack.stack[vm->stack.ptr++] = value;
  /* }}} */
}

/*
 * name:        pop
 * description: returns the top-most value from the stack, and reduces it's size
 */
static INT pop(nvm_t *vm)
{
  /* {{{ pop body */
  /* check if the stack is empty */
  if (vm->stack.ptr <= 0){
    fprintf(stderr, "nvm: error: attempting to pop from an empty stack\n");
    exit(1);
  }

  return vm->stack.stack[--vm->stack.ptr];
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
  vm->stack.stack[vm->stack.ptr--] = 0;
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
  /* check for overflow */
  if (vm->vars.ptr >= vm->vars.size){
    vm->vars.size += 10;
    vm->vars.stack = realloc(vm->vars.stack, vm->vars.size);
  }

  INT FOS = pop(vm);

  vm->vars.stack[vm->vars.ptr].name = name;
  vm->vars.stack[vm->vars.ptr].value = FOS;
  vm->vars.ptr++;
  /* }}}  */
}


/*
 * name:        load_name
 * description: pushes a value of a given variables <name> to the stack
 */
static void load_name(nvm_t *vm, char *name)
{
  /* {{{ load_name body */
  for (unsigned i = 0; i < vm->vars.ptr; i++){
    if (!strcmp(vm->vars.stack[i].name, name)){
      load_const(vm, vm->vars.stack[i].value);
      return;
    }
  }

  fprintf(stderr, "nvm: variable '%s' not found\n", name);
  exit(1);
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

  if (vm->stack.ptr <= 0){
    /* the stack is empty */
    printf("the stack is empty\n");
    return;
  }

  for (i = 0; i < vm->stack.ptr; i++){
    printf("item on stack: %d\n", vm->stack.stack[i]);
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
    /* found a function definition */
    if (vm->bytes[i + vm->functions_offset] == FN_START){
      /* skip over FN_START */
      i++;
      /* get the length */
      length = vm->bytes[i + vm->functions_offset];
      name = vm->mallocer(length + 1);
      /* skip over the length byte */
      i++;
      /* get the name */
      for (j = 0; j < length; j++){
        name[j] = vm->bytes[i + vm->functions_offset + j];
      }
      name[j] = '\0';
      /* skip over the whole name */
      i += length;
      /* append that function to the functions stack */
      vm->funcs.stack[vm->funcs.ptr].name = strdup(name);
      vm->funcs.stack[vm->funcs.ptr].offset = i;
      vm->funcs.ptr++;
      free(name);
    }
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
  int found = 0, old_ip;
  /* new frame for the call */
  nvm_call_frame *new_frame = vm->mallocer(sizeof(nvm_call_frame));
  /* store the old variables stack */
  nvm_vars_stack old_vars_stack = vm->vars;
  /* search for the function */
  for (func = 0; func < vm->funcs.ptr; func++){
    /* found it */
    if (!strcmp(name, vm->funcs.stack[func].name)){
      found = 1;
      break;
    }
  }

  if (!found){
    printf("nvm: error: function '%s' not found\n", name);
    exit(1);
  }

  /* create the stack */
  new_frame->vars.stack = vm->mallocer(INITIAL_VARS_STACK_SIZE * sizeof(nvm_vars_stack));
  /* set the variables stack to the newly created one */
  vm->vars.stack = new_frame->vars.stack;
  vm->vars.ptr = 0;
  vm->vars.size = 0;
  /* store the old value of the instruction pointer */
  old_ip = vm->ip;
  /* set the instruction pointer to the body of the function */
  vm->ip = i + vm->funcs.stack[func].offset;
  shiftright();
  /* execute the WHOLE body */
  while (vm->bytes[vm->ip] != FN_END){
    dispatch(vm);
    vm->ip++;
  }
  /* restore the last position of the instruction, before calling, so it could
   * move on with the code */
  /* XXX, why do I have to decrement old_ip by two, to make it work? */
  vm->ip = (old_ip -= 2);
  /* restore the old variables stack */
  vm->vars = old_vars_stack;
  /* free the functions call stack */
  vm->freeer(new_frame->vars.stack);
  new_frame->vars.stack = NULL;
  vm->freeer(new_frame);
  new_frame = NULL;
  shiftleft();
  /* }}} */
}

nvm_t *nvm_init(const char *filename, void *(*mallocer)(size_t), void (*freeer)(void *))
{
  /* {{{ nvm_init body */
  /* set the defaults for mallocer and freeer */
  if (mallocer == NULL)
    mallocer = malloc;
  if (freeer == NULL)
    freeer = free;

  nvm_t *vm = mallocer(sizeof(nvm_t));

  if (!vm){
    return NULL;
  }

  vm->filename         = filename;
  vm->bytes            = NULL;
  vm->mallocer         = mallocer;
  vm->freeer           = freeer;
  vm->stack.stack      = mallocer(INITIAL_STACK_SIZE * sizeof(INT));
  vm->stack.size       = INITIAL_STACK_SIZE;
  vm->stack.ptr        = 0;
  vm->vars.stack       = mallocer(INITIAL_VARS_STACK_SIZE * sizeof(nvm_var));
  vm->vars.size        = INITIAL_VARS_STACK_SIZE;
  vm->vars.ptr         = 0;
  vm->funcs.stack      = mallocer(INITIAL_FUNCS_STACK_SIZE * sizeof(nvm_func));
  vm->funcs.size       = INITIAL_FUNCS_STACK_SIZE;
  vm->funcs.ptr        = 0;
  vm->functions_offset = -1;

  return vm;
  /* }}} */
}

void nvm_destroy(nvm_t *vm)
{
  /* {{{ nvm_destroy body */
  vm->freeer(vm->stack.stack);
  vm->freeer(vm->bytes);
  vm->freeer(vm->vars.stack);
  vm->freeer(vm->funcs.stack);
  vm->freeer(vm);
  vm->stack.stack = NULL;
  vm->bytes = NULL;
  vm->vars.stack = NULL;
  vm->funcs.stack = NULL;
  vm = NULL;
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
  vm->bytes = vm->mallocer(st.st_size);
  vm->bytes_count = st.st_size;
  /* fetch the file */
  fread(vm->bytes, st.st_size, sizeof(BYTE), f);

  /* start the pre-run (search for functions, store them, etc.) */
  prerun(vm);

#if VERBOSE
  printf("## using NVM version %u.%u.%u ##\n\n", vm->bytes[0], vm->bytes[1], vm->bytes[2]);
#endif

  /* we start from 3 to skip over the version
     we end   at functions offset */
  for (vm->ip = 3; vm->ip < vm->functions_offset; vm->ip++){
    dispatch(vm);
  }

  fclose(f);

  return 0;
  /* }}} */
}

static void dispatch(nvm_t *vm)
{
  /* {{{ dispatch body */
  /* an `int` is four bytes, but we're reading one byte at a time */
  BYTE byte_one, byte_two, byte_three, byte_four;
  /* this is the final number which is a result of connecting the four mentioned
   * above */
  INT value;
  /* used to retrieve variables names */
  char *string = NULL;
  /* additional counter (it's here because GCC complains about redefining it) */
  int j = 0;

  switch (vm->bytes[vm->ip]){
    /* {{{ main op switch */
    case NOP:
      /* that was tough */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("nop\n");
#endif
      break;
    case LOAD_CONST:
      /* extract the bytes */
      byte_one   = vm->bytes[vm->ip + 1];
      byte_two   = vm->bytes[vm->ip + 2] << 2;
      byte_three = vm->bytes[vm->ip + 3] << 4;
      byte_four  = vm->bytes[vm->ip + 4] << 6;
      /* assemble the final number */
      value = byte_one ^ byte_two ^ byte_three ^ byte_four;
      /* skip over the bytes */
      vm->ip += 4;

#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("push\t(%d)\n", value);
#endif

      load_const(vm, value);
      break;
    case DISCARD:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("discard\n");
#endif
      discard(vm);
      break;
    case ROT_TWO:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("rot_two\n");
#endif
      rot_two(vm);
      break;
    case ROT_THREE:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("rot_three\n");
#endif
      rot_three(vm);
      break;
    case STORE:
      /* I don't want to declare any additional variables, so I'm using
       * 'byte_one', but calling it 'length' makes more sense */
#define length byte_one

      length = vm->bytes[++vm->ip];
      string = vm->mallocer(length + 1);

      if (!string){
        fprintf(stderr, "malloc: failed to allocate %d bytes.\n", length);
        return;
      }

      /* skip over the length byte */
      vm->ip++;
      /* getting the variables name, iterating through the <length> next
       * numbers */
      for (j = 0; j < length; j++){
        string[j] = vm->bytes[vm->ip + j];
      }
      string[j] = '\0';
      /* skip over the bytes */
      vm->ip += length - 1;

#undef length
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("store\t(%s)\n", string);
#endif
      store(vm, strdup(string));
      vm->freeer(string);
      string = NULL;
      break;
    case LOAD_NAME:
      /* Some trick over here */
#define length byte_one
      length = vm->bytes[++vm->ip];
      string = vm->mallocer(length + 1);

      if (!string){
        fprintf(stderr, "malloc: failed to allocate %d bytes.\n", length);
        return;
      }

      /* skip over the length byte */
      vm->ip++;
      /* getting the variables name, iterating through the <length> next
       * numbers */
      for (j = 0; j < length; j++){
        string[j] = vm->bytes[vm->ip + j];
      }
      string[j] = '\0';
      /* skip over the bytes */
      vm->ip += length - 1;

#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("get\t(%s)\n", string);
#endif
#undef length
      load_name(vm, strdup(string));
      vm->freeer(string);
      string = NULL;
      break;
    case DUP:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("dup\n");
#endif
      dup(vm);
      break;
    case BINARY_ADD:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("add\n");
#endif
      binop(vm, vm->bytes[vm->ip]);
      break;
    case BINARY_SUB:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("sub\n");
#endif
      binop(vm, vm->bytes[vm->ip]);
      break;
    case BINARY_MUL:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("mul\n");
#endif
      binop(vm, vm->bytes[vm->ip]);
      break;
    case BINARY_DIV:
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("div\n");
#endif
      binop(vm, vm->bytes[vm->ip]);
      break;
    case CALL:
#define length byte_one
      length = vm->bytes[++vm->ip];
      string = vm->mallocer(length + 1);
      /* skip over the length byte */
      vm->ip++;
      /* get the name */
      for (j = 0; j < length; j++){
        string[j] = vm->bytes[vm->ip + j];
      }
      string[j] = '\0';
      /* skip over the bytes */
      vm->ip += length + 1;

#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("call\t(%s)\n", string);
#endif
#undef  length
      call(vm, strdup(string));
      vm->freeer(string);
      break;
    default:
      printf("%04x: error: unknown op: %d (%08X)\n", vm->ip, vm->bytes[vm->ip], vm->bytes[vm->ip]);
      /* you failed the game */
      return;
      break;
    /* }}} */
  }
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
 * Helloween, Rhapsody of Fire, Avantasia, Edguy, Iron Savior
 * Michael Schenker Group, Testament
 *
 * The Office, Family Guy
 *
 */
