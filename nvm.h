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
#define INITIAL_STACK_SIZE 10
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
  unsigned offset;
} nvm_function;

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
  /* instruction pointer */
  int ip;
  /* a pointer to the malloced Stack */
  INT *stack;
  /* stack 'pointer' */
  unsigned stack_ptr;
  /* size of the stack */
  unsigned stack_size;
  /* a pointer to the malloced variables stack */
  nvm_var *vars;
  /* variables 'pointer' */
  unsigned vars_ptr;
  /* size of the variables stack */
  unsigned vars_size;
  /* functions offset (it's not unsigned because I'm using -1 later on for
     error-like purposes) */
  int functions_offset;
  /* a pointer to the malloced functions stack */
  nvm_function *funcs;
  /* function 'pointer' */
  unsigned funcs_ptr;
  /* size of the functions stack */
  unsigned funcs_size;
} nvm_t;

/*
 * name:        nvm_init
 * description: creates a new `nvm_t` object, and returns pointer to it
 *
 * parameters:
 *
 *   malloccer: pointer to a function that would allocate the needed stuff.
 *    filename: name of a file into which the bytecode will be
 *              printed and from which the bytecode will be extracted in order
 *              to execute the operations.
 *
 * return:      pointer to a new malloced object or NULL if malloc failed
 */
nvm_t *nvm_init(void *(*malloccer)(size_t), const char *filename);

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
 * parameter:   freeer - pointer to a function that would 'free' the allocced
 *                       data NVM used.
 */
void nvm_destroy(void (*freeer)(void *), nvm_t *virtual_machine);

/*
 * name:        nvm_print_stack
 * description: prints what's left on stack
 */
void nvm_print_stack(nvm_t *virtual_machine);

#endif /* NVM_H */
