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
static void load_const(nvm_t *virtual_machine, nvm_value value);
static nvm_value pop(nvm_t *virtual_machine);
static void prerun(nvm_t *virtual_machine);
static void dispatch(nvm_t *vm);
static char *strdup(nvm_t *virtual_machine, const char *p);
/* }}} */

#if VERBOSE
  /* to make the output nicer */
  static unsigned shiftwidth = 1;
# define shiftright() shiftwidth += 2
# define shiftleft() shiftwidth -= 2
# define print_spaces() do {\
    unsigned counter = 0;\
    for (; counter < shiftwidth; counter++)\
      printf(" ");\
  } while (0);
#endif

/*
 * name:        load_const
 * description: pushes given <value> to the stack
 */
static void load_const(nvm_t *vm, nvm_value value)
{
  /* {{{ load_const body */
  nvm_stack_element *new = vm->mallocer(sizeof(nvm_stack_element));
  if (!new){
    fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", sizeof(nvm_stack_element), __LINE__ - 2);
    exit(1);
  }

  /* set its value */
  new->value = value;

  /* appending the value to the stack */
  /*   the stack is not empty */
  if (vm->stack->head && vm->stack->tail){
    new->next = vm->stack->head->next;
    vm->stack->head->next = new;
    new->prev = vm->stack->head;
    vm->stack->head = new;
  /*   the stack is empty */
  } else {
    new->next = vm->stack->head;
    new->prev = vm->stack->tail;
    vm->stack->head = new;
    vm->stack->tail = new;
  }
  /* }}} */
}

/*
 * name:        pop
 * description: returns the top-most value from the stack, and reduces it's size
 */
static nvm_value pop(nvm_t *vm)
{
  /* {{{ pop body */
  /* check if the stack is empty */
  if (!vm->stack->head && !vm->stack->tail){
    fprintf(stderr, "nvm: error: attempting to pop from an empty stack\n");
    exit(1);
  }

  nvm_value ret = vm->stack->head->value;

  /* there is only one element on the stack */
  if (vm->stack->head == vm->stack->tail){
    nvm_stack_element *tmp = vm->stack->head;
    free(tmp->value.ptr);
    free(tmp);
    vm->stack->head = vm->stack->tail = NULL;
  /* there is more than one element on the stack */
  } else {
    nvm_stack_element *tmp = vm->stack->head;
    vm->stack->head = tmp->prev;
    vm->stack->head->next = tmp->next;
    free(tmp->value.ptr);
    free(tmp);
  }

  return ret;
  /* }}} */
}

void nvm_print_stack(nvm_t *vm)
{
  /* {{{ print_stack body */
  if (!vm->stack->head && !vm->stack->tail){
    /* the stack is empty */
    printf("the stack is empty\n");
    return;
  }

  for (nvm_stack_element *p = vm->stack->tail; p != NULL; p = p->next){
    printf("item on stack: %d\n", *(int *)p->value.ptr);
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
  /* start from 3 to skip over version */
  for (i = 3; i < vm->bytes_count; i++){
    /* found a function definition */
    if (vm->bytes[i] == FN_START){
      /* skip over FN_START */
      i++;
      /* get the length */
      length = vm->bytes[i];
      name = vm->mallocer(length + 1);
      /* skip over the length byte */
      i++;
      /* get the name */
      for (j = 0; j < length; j++){
        name[j] = vm->bytes[i + j];
      }
      name[j] = '\0';
      /* skip over the whole name */
      i += length;
      /* create the function */
      nvm_func *new_func = vm->mallocer(sizeof(nvm_func));
      nvm_funcs_stack *new_elem = vm->mallocer(sizeof(nvm_funcs_stack));
      /* set its things */
      new_func->name = strdup(vm, name);
      new_func->offset = i;
      new_elem->func = new_func;
      /* append that function to the functions stack */
      new_elem->next = vm->funcs;
      vm->funcs = new_elem;
      /* release the string */
      vm->freeer(name);
      name = NULL;
    }
  }
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
  vm->stack            = mallocer(sizeof(nvm_stack));
  vm->stack->head      = NULL;
  vm->stack->tail      = NULL;
  vm->funcs            = NULL;
  vm->blocks           = mallocer(sizeof(nvm_blocks_stack));
  vm->blocks->head     = NULL;
  vm->blocks->tail     = NULL;
  vm->call_stack       = mallocer(sizeof(nvm_call_stack));
  vm->call_stack->head = NULL;
  vm->call_stack->tail = NULL;
  vm->free_stack       = NULL;

  /* initialize the bytes */
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
  /* close the file */
  fclose(f);

  return vm;
  /* }}} */
}

void nvm_destroy(nvm_t *vm)
{
  /* {{{ nvm_destroy body */
  void *next = NULL;
  /* free everything on the free_stack */
  for (nvm_free_stack *p = vm->free_stack; p != NULL; p = next){
    next = p->next;
    vm->freeer(p->ptr);
    vm->freeer(p);
  }
  next = NULL;
  /* free everything on the variables stack */
  for (nvm_vars_stack *p = vm->blocks->head->vars; p != NULL; p = next){
    next = p->next;
    vm->freeer(p->var);
    vm->freeer(p);
  }
  next = NULL;
  /* free everything on the functions stack */
  for (nvm_funcs_stack *p = vm->funcs; p != NULL; p = next){
    next = p->next;
    vm->freeer(p->func);
    vm->freeer(p);
  }
  /* free the main block */
  vm->freeer(vm->blocks->head);
  /* free the blocks stack */
  vm->freeer(vm->blocks);
  next = NULL;
  /* free the main stack */
  for (nvm_stack_element *p = vm->stack->tail; p != NULL; p = next){
    next = p->next;
    vm->freeer(p);
  }
  /* the main stack itself */
  vm->freeer(vm->stack);
  /* free every other stack */
  vm->freeer(vm->bytes);
  vm->freeer(vm->call_stack);
  vm->freeer(vm);
  /* }}} */
}

int nvm_blastoff(nvm_t *vm)
{
  /* {{{ nvm_blastoff body */
  /* start the pre-run (search for functions, store them, etc.) */
  prerun(vm);

#if VERBOSE
  printf("## using NVM version %u.%u.%u ##\n\n", vm->bytes[0], vm->bytes[1], vm->bytes[2]);
#endif

  /* the main program is one big block, so create one now */
  nvm_block *main_block = vm->mallocer(sizeof(nvm_block));
  if (!main_block){
    fprintf(stderr, "nvm: error: failed to allocate %lu bytes at line %d\n", sizeof(nvm_block), __LINE__ - 2);
    exit(1);
  }
  /* initialize the main block */
  main_block->vars = NULL;
  /* append the block to the stack */
  main_block->next = vm->blocks->head;
  main_block->prev = vm->blocks->tail;
  vm->blocks->head = main_block;
  vm->blocks->tail = main_block;

  /* and the bytecode executing itself
   *
   * we start from 3 to skip over the version
     we end   at functions offset */
  for (vm->ip = 3; vm->ip < vm->bytes_count; vm->ip++){
    dispatch(vm);
  }

  return 0;
  /* }}} */
}

int nvm_validate(nvm_t *vm)
{
  /* {{{ nvm_validate body */
  if (vm->bytes){
    int tmp;
    for (unsigned i = 3; i < vm->bytes_count; i++){
      switch (vm->bytes[i]){
        /* fall throughs */
        case NOP:
          break;
        case LOAD_CONST:
          i += 4;
          break;
        case DISCARD:
        case ROT_TWO:
        case ROT_THREE:
          break;
        case STORE:
          /* byte next to STORE is that variables name length */
          tmp = vm->bytes[++i];
          /* skip over the length byte */
          i++;
          /* skip over the name */
          i += tmp - 1;
          break;
        case LOAD_NAME:
          /* byte next to LOAD_NAME is that variables name length */
          tmp = vm->bytes[++i];
          /* skip over the length byte */
          i++;
          /* skip over the name */
          i += tmp - 1;
          break;
        case DUP:
        case BINARY_ADD:
        case BINARY_SUB:
        case BINARY_MUL:
        case BINARY_DIV:
          break;
        case CALL:
          /* byte next to CALL is that functions name length */
          tmp = vm->bytes[++i];
          /* skip over the length byte */
          i++;
          /* skip over the name */
          i += tmp - 1;
          break;
        case FN_START:
          /* byte next to FN_START is that functions name length */
          tmp = vm->bytes[++i];
          /* skip over the length byte */
          i++;
          /* skip over the name */
          i += tmp - 1;
          break;
        case FN_END:
          break;
        case ENTER_BLOCK:
          break;
        case LEAVE_BLOCK:
          break;
        default:
          fprintf(stderr, "nvm: error: unknown op 0x%02X at position 0x%02X\n", vm->bytes[i], i);
          return -1;
      }
    }
    return 0;
  } else {
    return -2;
  }
  /* }}} */
}

static void dispatch(nvm_t *vm)
{
  /* {{{ dispatch body */
  /* an `int` is four bytes, but we're reading one byte at a time */
  BYTE byte_one, byte_two, byte_three, byte_four;
  /* used to retrieve variables names */
  char *string = NULL;
  /* additional counter */
  int j = 0;

  switch (vm->bytes[vm->ip]){
    case NOP: {
      /* {{{ NOP body */
      /* that was tough */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("nop\n");
#endif
      break;
      /* }}} */
    } case LOAD_CONST: {
      /* {{{ LOAD_CONST body */
      /* this is the final number which is a result of connecting the four
       * bytes that follow the LOAD_CONST byte */
      INT integer;
#if VERBOSE
      printf("%04x:", vm->ip);
#endif
      /* extract the bytes */
      byte_one   = vm->bytes[vm->ip + 1];
      byte_two   = vm->bytes[vm->ip + 2];
      byte_three = vm->bytes[vm->ip + 3];
      byte_four  = vm->bytes[vm->ip + 4];
      /* assemble the final number */
      integer = byte_one ^ (byte_two << 8) ^ (byte_three << 16) ^ (byte_four << 24);
      /* skip over the bytes */
      vm->ip += 4;
#if VERBOSE
      print_spaces();
      printf("load_const\t(%d)\n", integer);
#endif
      nvm_value value;
      int *new = vm->mallocer(sizeof(INT));
      if (!new){
        fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", sizeof(INT), __LINE__ - 2);
        exit(1);
      }
      *new = integer;
      value.ptr = new;
      value.type = INTEGER;
      load_const(vm, value);
      break;
      /* }}} */
    } case DISCARD: {
      /* {{{ DISCARD body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("discard\n");
#endif
      /* check if the stack is empty */
      if (!vm->stack->head && vm->stack->tail){
        fprintf(stderr, "nvm: error: attempting to discard on an empty stack\n");
        exit(1);
      }
      /* remove it from the stack */
      vm->stack->head->prev->next = vm->stack->head->next;
      nvm_stack_element *tmp = vm->stack->head->prev;
      free(vm->stack->head->value.ptr);
      free(vm->stack->head);
      vm->stack->head = tmp;
      break;
      /* }}} */
    } case ROT_TWO: {
      /* {{{ ROT_TWO body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("rot_two\n");
#endif
      /* First on Stack */
      nvm_value FOS = pop(vm);
      /* Second on Stack */
      nvm_value SOS = pop(vm);
      /* Load'em all */
      load_const(vm, FOS);
      load_const(vm, SOS);
      break;
      /* }}} */
    } case ROT_THREE: {
      /* {{{ ROT_THREE body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("rot_three\n");
#endif
      /* Pop'em all */
      nvm_value FOS = pop(vm);
      nvm_value SOS = pop(vm);
      nvm_value TOS = pop(vm);
      /* Load'em all */
      load_const(vm, FOS);
      load_const(vm, TOS);
      load_const(vm, SOS);
      break;
      /* }}} */
    } case STORE: {
      /* {{{ STORE body */
#if VERBOSE
      printf("%04x:", vm->ip);
#endif
      /* byte next to STORE is that variables name length */
      byte_one = vm->bytes[++vm->ip];
      string = vm->mallocer(byte_one + 1);
      if (!string){
        fprintf(stderr, "nvm: error: failed to allocate %d bytes.\n", byte_one);
        return;
      }
      /* skip over the length byte */
      vm->ip++;
      /* getting the variables name, iterating through the <length> next
       * numbers */
      for (j = 0; j < byte_one; j++){
        string[j] = vm->bytes[vm->ip + j];
      }
      string[j] = '\0';
      /* skip over the bytes */
      vm->ip += byte_one - 1;
#if VERBOSE
      print_spaces();
      printf("store\t\t(%s)\n", string);
#endif
      /* create variable and its place in the list */
      nvm_vars_stack *new_stack = vm->mallocer(sizeof(nvm_vars_stack));
      if (!new_stack){
        fprintf(stderr, "nvm: error: failed to allocate %lu bytes at line %d\n", sizeof(nvm_vars_stack), __LINE__ - 2);
        exit(1);
      }
      nvm_var *new_var = vm->mallocer(sizeof(nvm_var));
      if (!new_var){
        fprintf(stderr, "nvm: error: failed to allocate %lu bytes at line %d\n", sizeof(nvm_var), __LINE__ - 2);
        exit(1);
      }
      /* initialize */
      new_stack->var = new_var;
      new_stack->var->name = strdup(vm, string);
      new_stack->var->value = pop(vm);
      /* append the variable to the variables list */
      new_stack->next = vm->blocks->head->vars;
      vm->blocks->head->vars = new_stack;
      /* free the string */
      vm->freeer(string);
      string = NULL;
      break;
      /* }}} */
    } case LOAD_NAME: {
      /* {{{ LOAD_NAME body */
#if VERBOSE
      printf("%04x:", vm->ip);
#endif
      /* byte next to LOAD_NAME is that name's length */
      byte_one = vm->bytes[++vm->ip];
      string = vm->mallocer(byte_one + 1);

      if (!string){
        fprintf(stderr, "malloc: failed to allocate %d bytes.\n", byte_one);
        return;
      }

      /* skip over the length byte */
      vm->ip++;
      /* getting the variables name, iterating through the <length> next
       * numbers */
      for (j = 0; j < byte_one; j++){
        string[j] = vm->bytes[vm->ip + j];
      }
      string[j] = '\0';
      /* skip over the bytes */
      vm->ip += byte_one - 1;
#if VERBOSE
      print_spaces();
      printf("load_name\t\t(%s)\n", string);
#endif
      int found = 0;
      /* iterate through the variables list */
      for (nvm_vars_stack *p = vm->blocks->head->vars; p != NULL; p = p->next){
        /* we found the variable */
        if (!strcmp(p->var->name, string)){
          /* push its value onto the stack */
          load_const(vm, p->var->value);
          found = 1;
          break;
        }
      }
      /* inform if we have not found the variable */
      if (!found){
        fprintf(stderr, "nvm: variable '%s' not found\n", string);
        exit(1);
      }
      vm->freeer(string);
      string = NULL;
      break;
      /* }}} */
    } case DUP: {
      /* {{{ DUP body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("dup\n");
#endif
      /* Pop the top-most value */
      nvm_value FOS = pop(vm);
      /* Put it twice to the stack */
      load_const(vm, FOS);
      load_const(vm, FOS);
      break;
      /* }}} */
    } case BINARY_ADD: {
      /* {{{ BINARY_ADD body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("add\n");
#endif
      nvm_value FOS = pop(vm);
      nvm_value SOS = pop(vm);
      nvm_value res;
      /* create new INT */
      INT *new = vm->mallocer(sizeof(INT));
      if (!new){
        fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", sizeof(INT), __LINE__ - 2);
        exit(1);
      }
      *new = *(INT *)SOS.ptr + *(INT *)FOS.ptr;
      res.ptr = new;
      res.type = INTEGER;
      load_const(vm, res);
      break;
      /* }}} */
    } case BINARY_SUB: {
      /* {{{ BINARY_SUB body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("sub\n");
#endif
      nvm_value FOS = pop(vm);
      nvm_value SOS = pop(vm);
      nvm_value res;
      /* create new INT */
      INT *new = vm->mallocer(sizeof(INT));
      if (!new){
        fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", sizeof(INT), __LINE__ - 2);
        exit(1);
      }
      *new = *(INT *)SOS.ptr - *(INT *)FOS.ptr;
      res.ptr = new;
      res.type = INTEGER;
      load_const(vm, res);
      break;
      /* }}} */
    } case BINARY_MUL: {
      /* {{{ BINARY_MUL body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("mul\n");
#endif
      nvm_value FOS = pop(vm);
      nvm_value SOS = pop(vm);
      nvm_value res;
      /* create new INT */
      INT *new = vm->mallocer(sizeof(INT));
      if (!new){
        fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", sizeof(INT), __LINE__ - 2);
        exit(1);
      }
      *new = (*(INT *)SOS.ptr) * (*(INT *)FOS.ptr);
      res.ptr = new;
      res.type = INTEGER;
      load_const(vm, res);
      break;
      /* }}} */
    } case BINARY_DIV: {
      /* {{{ BINARY_DIV body */
#if VERBOSE
      printf("%04x:", vm->ip);
      print_spaces();
      printf("div\n");
#endif
      nvm_value FOS = pop(vm);
      nvm_value SOS = pop(vm);
      nvm_value res;
      /* create new INT */
      INT *new = vm->mallocer(sizeof(INT));
      if (!new){
        fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", sizeof(INT), __LINE__ - 2);
        exit(1);
      }
      *new = (*(INT *)SOS.ptr) / (*(INT *)FOS.ptr);
      res.ptr = new;
      res.type = INTEGER;
      load_const(vm, res);
      break;
      /* }}} */
    } case CALL: {
      /* {{{ CALL body */
#if VERBOSE
      printf("%04x:", vm->ip);
#endif
      /* prevent too big function calls */
      /*if (vm->call_stack.ptr >= 700){*/
        /*fprintf(stderr, "nvm: error: exceeded limit of function calls (700 max)\n");*/
        /*exit(1);*/
      /*}*/
      /* next byte to CALL byte is the functions name */
      byte_one = vm->bytes[++vm->ip];
      string = vm->mallocer(byte_one + 1);
      /* skip over the length byte */
      vm->ip++;
      /* get the name */
      for (j = 0; j < byte_one; j++){
        string[j] = vm->bytes[vm->ip + j];
      }
      string[j] = '\0';
      /* skip over the bytes */
      vm->ip += byte_one + 1;
#if VERBOSE
      print_spaces();
      printf("call\t\t(%s)\n", string);
#endif
      nvm_func *func;
      int found = 0, old_ip;
      /* new frame for the call */
      nvm_call_frame *new_frame = vm->mallocer(sizeof(nvm_call_frame));
      if (!new_frame){
        fprintf(stderr, "nvm: error: malloc failed to allocate %lu bytes at line %d\n", sizeof(nvm_call_frame), __LINE__ - 2);
        exit(1);
      }
      /* store the old variables stack */
      nvm_vars_stack *old_vars_stack = vm->blocks->head->vars;
      /* search for the function */
      for (nvm_funcs_stack *p = vm->funcs; p != NULL; p = p->next){
        /* found it */
        if (!strcmp(string, p->func->name)){
          func = p->func;
          found = 1;
          break;
        }
      }

      if (!found){
        printf("nvm: error: function '%s' not found\n", string);
        exit(1);
      }

      /* set the frames name */
      new_frame->fn_name = strdup(vm, string);
      new_frame->vars = NULL;
      /* set the variables stack to the newly created one */
      vm->blocks->head->vars = new_frame->vars;
      /* store the old value of the instruction pointer */
      old_ip = vm->ip;
      /* set the instruction pointer to the body of the function */
      vm->ip = func->offset;
      /* append the call frame to the call stack */
      /*   the list is NOT empty */
      if (vm->call_stack->head && vm->call_stack->tail){
        new_frame->next = vm->call_stack->head->next;
        vm->call_stack->head->next = new_frame;
        new_frame->prev = vm->call_stack->head;
        vm->call_stack->head = new_frame;
      /*   appending to the empty list */
      } else {
        new_frame->next = vm->call_stack->head;
        new_frame->prev = vm->call_stack->tail;
        vm->call_stack->head = new_frame;
        vm->call_stack->tail = new_frame;
      }
#if VERBOSE
      shiftright();
#endif
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
      vm->blocks->head->vars = old_vars_stack;
      /* free the functions call stack */
      for (nvm_vars_stack *p = new_frame->vars; p != NULL; p = p->next){
        vm->freeer(p->var);
        p->var = NULL;
        vm->freeer(p);
        p = NULL;
      }
      /* free the string */
      vm->freeer(string);
      string = NULL;
      /* remove the call from the call stack */
      /*   there is only one element left */
      if (vm->call_stack->head == vm->call_stack->tail){
        vm->freeer(vm->call_stack->head);
        vm->call_stack->head = NULL;
        vm->call_stack->tail = NULL;
      /*   there is more than one element on the stack */
      } else {
        vm->call_stack->head->prev->next = vm->call_stack->head->next;
        nvm_call_frame *tmp = vm->call_stack->head->prev;
        vm->freeer(vm->call_stack->head);
        vm->call_stack->head = tmp;
      }
#if VERBOSE
      shiftleft();
#endif
      break;
      /* }}} */
    } case FN_START: {
      /* {{{ FN_START body */
      /* skip over the FN_START byte */
      vm->ip++;
      /* get the length */
      byte_one = vm->bytes[vm->ip];
      /* skip over the length */
      vm->ip++;
      /* skip over the name */
      vm->ip += byte_one;
      /* skip over the whole body */
      while (vm->bytes[vm->ip] != FN_END)
        vm->ip++;
      break;
      /* }}} */
    } case ENTER_BLOCK: {
      /* {{{ ENTER_BLOCK body */
      nvm_block *new = vm->mallocer(sizeof(nvm_block));
      if (!new){
        fprintf(stderr, "nvm: error: failed to allocate %lu bytes at line %d\n", sizeof(nvm_block), __LINE__ - 2);
        exit(1);
      }
      /* initialize the block */
      new->vars = NULL;
      /* the stack is empty */
      if (!vm->blocks->head && !vm->blocks->tail){
        new->next = vm->blocks->head;
        new->prev = vm->blocks->tail;
        vm->blocks->head = new;
        vm->blocks->tail = new;
      /* the stack is not empty */
      } else {
        new->next = vm->blocks->head->next;
        vm->blocks->head->next = new;
        new->prev = vm->blocks->head;
        vm->blocks->head = new;
      }
#if VERBOSE
      shiftright();
#endif
      break;
      /* }}} */
    } case LEAVE_BLOCK: {
      /* {{{ LEAVE_BLOCK body */
      /* there are no blocks on the stack */
      if (!vm->blocks->head && !vm->blocks->tail){
        fprintf(stderr, "nvm: error: trying to exit from a block, while not entering into one\n");
        exit(1);
      }
      /* fetch the last block on the blocks stack */
      nvm_block *last_block = vm->blocks->head;
      /* remove the variables that were hold in that block */
      for (nvm_vars_stack *p = vm->blocks->head->vars; p != NULL; p = p->next){
        vm->freeer(p->var);
        vm->freeer(p);
      }
      /* restore the previous variables stack to be in use */
      vm->blocks->head->vars = last_block->prev->vars;
      /* there is only one element on the stack */
      if (vm->blocks->head == vm->blocks->tail){
        free(last_block);
        vm->blocks->head = vm->blocks->tail = NULL;
      /* there is more than one element on the stack */
      } else {
        vm->blocks->head = last_block->prev;
        vm->blocks->head->next = last_block->next;
        free(last_block);
      }
#if VERBOSE
      shiftleft();
#endif
      break;
      /* }}} */
    } default: {
      /* {{{ unknown opcode */
      printf("nvm: error: unknown op 0x%02X at position 0x%02X\n", vm->bytes[vm->ip], vm->ip);
      /* you failed the game */
      exit(1);
      /* }}} */
    }
  }
  /* }}} dispatch end */
}

static char *strdup(nvm_t *vm, const char *p)
{
  /* {{{ strdup body */
  char *np = malloc(strlen(p) + 1);
  if (!np){
    fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", strlen(p) + 1, __LINE__ - 2);
    exit(1);
  }

  nvm_free_stack *new = malloc(sizeof(nvm_free_stack));
  if (!new){
    fprintf(stderr, "nvm: malloc failed to allocate %lu bytes at line %d\n", sizeof(nvm_free_stack), __LINE__ - 2);
    exit(1);
  }

  /* append to the stack */
  new->ptr = np;
  new->next = vm->free_stack;
  vm->free_stack = new;

  return np ? strcpy(np, p) : np;
  /* }}} */
}

/*
 * Helloween, Rhapsody of Fire, Avantasia, Edguy, Iron Savior
 * Running Wild, Michael Schenker Group, Testament
 * Judas Priest, Stratovarius
 *
 * The Office, Family Guy, Monty Python
 *
 */
