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
 * NVM type for its values type.
 */
typedef enum {
  INTEGER
} nvm_value_type;

/*
 * NVM type for its values.
 */
typedef struct {
  /* a pointer to the value */
  void *ptr;
  /* type of the value */
  nvm_value_type type;
} nvm_value;

/*
 * NVM type for its variables.
 */
typedef struct {
  char *name;
  nvm_value value;
} nvm_var;

/*
 * NVM type for its functions.
 */
typedef struct {
  /* name of the function */
  char *name;
  /* where does functions body begins */
  unsigned offset;
} nvm_func;

/*
 * NVM type for its Main Stack element.
 */
typedef struct _nvm_stack_element {
  /* value of the element */
  nvm_value value;
  /* a pointer to the next element on the stack list */
  struct _nvm_stack_element *next;
  /* a pointer to the previous element on the stack list */
  struct _nvm_stack_element *prev;
} nvm_stack_element;

/*
 * NVM type for its Main Stack.
 */
typedef struct {
  /* a pointer to the first element in the stack list */
  nvm_stack_element *head;
  /* a pointer to the last element in the stack list */
  nvm_stack_element *tail;
} nvm_stack;

/*
 * NVM type for its variables stack.
 */
typedef struct _nvm_vars_stack {
  /* a pointer to the var itself */
  nvm_var *var;
  /* a pointer to the next element on the list */
  struct _nvm_vars_stack *next;
} nvm_vars_stack;

/*
 * NVM type for its functions stack.
 */
typedef struct _nvm_funcs_stack {
  /* pointer to the function */
  nvm_func *func;
  /* pointer to the next element on the stack */
  struct _nvm_funcs_stack *next;
} nvm_funcs_stack;

/*
 * NVM type for its call stack frame.
 */
typedef struct _nvm_call_frame {
  /* name of the function that was called */
  char *fn_name;
  /* stack of local variables for the function call */
  nvm_vars_stack *vars;
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
 * NVM type for its block.
 */
typedef struct _nvm_block {
  /* stack of variables for that block */
  nvm_vars_stack *vars;
  /* pointer to the next element on the stack */
  struct _nvm_block *next;
  /* pointer to the previous element on the stack */
  struct _nvm_block *prev;
} nvm_block;

/*
 * NVM type for its stack holding blocks.
 */
typedef struct {
  /* a pointer to the first block on the stack */
  nvm_block *head;
  /* a pointer to the last block on the stack */
  nvm_block *tail;
} nvm_blocks_stack;

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
  nvm_stack *stack;
  /* pointer to the first element of the variables stack */
  nvm_vars_stack *vars;
  /* pointer to the first element of the functions stack */
  nvm_funcs_stack *funcs;
  /* pointer to the blocks stack */
  nvm_blocks_stack *blocks;
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
