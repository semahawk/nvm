/*
 * nvm.h
 *
 */

#ifndef NVM_H
#define NVM_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include "opcodes.h"

/*
 * Obviously.
 */
#define NVM_VERSION_PATCH 1
#define NVM_VERSION_MINOR 0
#define NVM_VERSION_MAJOR 0

/*
 * Initial size of the stack.
 */
#define INITIAL_STACK_SIZE 40
/* Initial size of the variables stack */
#define INITIAL_VARS_STACK_SIZE 30
/* Initial size of the functions stack */
#define INITIAL_FUNCS_STACK_SIZE 30

/*
 * Used for verbosity/debugging purposes.
 */
#define VERBOSE 1

/*
 * Some handy types.
 */
typedef unsigned char BYTE;
typedef int32_t INT;

/*
 * NVM type for its variables.
 */
typedef struct {
  char *name;
  INT value;
} nvm_var;

/*
 * NVM type for its functions.
 */
typedef struct {
  char *name;
  /* how far from the BF byte it is */
  unsigned offset;
} nvm_func;

/*
 * NVM type for its Main Stack.
 */
typedef struct {
  /* the stack itself */
  INT *stack;
  /* 'pointer' to the current element */
  unsigned ptr;
  /* size of the stack */
  unsigned size;
} nvm_stack;

/*
 * NVM type for its variables stack.
 */
typedef struct {
  /* the stack itself */
  nvm_var *stack;
  /* 'pointer' to the current element */
  unsigned ptr;
  /* size of the stack */
  unsigned size;
} nvm_vars_stack;

/*
 * NVM type for its functions stack.
 */
typedef struct {
  /* the stack itself */
  nvm_func *stack;
  /* 'pointer' to the current element */
  unsigned ptr;
  /* size of the stack */
  unsigned size;
} nvm_funcs_stack;

/*
 * NVM type for its call stack frame.
 */
typedef struct _nvm_call_frame {
  /* name of the function that was called */
  char *fn_name;
  /* stack of local variables for the function call */
  nvm_vars_stack vars;
  /* a pointer to the next element of a linked list */
  struct _nvm_call_frame *next;
  /* a pointer to the previous element of a linked list */
  struct _nvm_call_frame *prev;
} nvm_call_frame;

/*
 * NVM type for its call stack.
 */
typedef struct _nvm_call_stack {
  /* a pointer to the first call frame in the list */
  nvm_call_frame *head;
  /* a pointer to the last call frame in the list */
  nvm_call_frame *tail;
} nvm_call_stack;

/*
 * NVM type for its stack of pointer to be freed at the end of execution.
 */
typedef struct _nvm_free_stack {
  void *ptr;
  struct _nvm_free_stack *next;
} nvm_free_stack;

/*
 * The main type for NVM.
 */
typedef struct {
  /* Files name */
  const char *filename;
  /* contents of the file */
  BYTE *bytes;
  /* number of the bytes */
  off_t bytes_count;
  /* a pointer to the mallocing function */
  void *(*mallocer)(size_t);
  /* a pointer to the freeing function */
  void (*freeer)(void *);
  /* instruction pointer */
  int ip;
  /* The Stack */
  nvm_stack stack;
  /* variables stack */
  nvm_vars_stack vars;
  /* functions offset (it's not unsigned because I'm using -1 later on for
     error-like purposes) */
  int functions_offset;
  /* functions stack */
  nvm_funcs_stack funcs;
  /* call stack, every function call goes here */
  nvm_call_stack *call_stack;
  /* pointer to the first element of free stack (with things to be free'd) */
  nvm_free_stack *free_stack;
} nvm_t;

/*
 * name:        nvm_init
 * description: creates a new `nvm_t` object, and returns pointer to it
 *
 * parameters:
 *
 *   malloccer: pointer to a function that would allocate the needed stuff
 *              (if NULL, will use malloc)
 *      freeer: pointer to a function that would free the allocated stuff.
 *              (if NULL, will use free)
 *    filename: name of a file into which the bytecode will be
 *              printed and from which the bytecode will be extracted in order
 *              to execute the operations.
 *
 * return:      pointer to a new malloced object or NULL if malloc failed
 */
nvm_t *nvm_init(const char *filename, void *(*malloccer)(size_t), void (*freeer)(void *));

/*
 * name:        nvm_blastoff
 * description: starts off the executing progress
 * return:      0 if everything went OK
 *              1 if not
 */
int nvm_blastoff(nvm_t *vm);

/*
 * name:        nvm_destroy
 * description: cleans up after everything (which includes fclosing the file and
 *              freeing the malloced pointer)
 */
void nvm_destroy(nvm_t *virtual_machine);

/*
 * name:        nvm_validate
 * description: validates the bytecode, if all the opcodes are correct and known
 *              and so-on
 * return:       0 - everything worked correctly
 *              -1 - there is an unknown opcode
 *              -2 - the virtual machine was not inited
 */
int nvm_validate(nvm_t *virtual_machine);

/*
 * name:        nvm_print_stack
 * description: prints what's left on stack
 */
void nvm_print_stack(nvm_t *virtual_machine);

#endif /* NVM_H */
