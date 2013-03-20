/*
 * nvm.h
 *
 */

#ifndef NVM_H
#define NVM_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/*
 * Obviously.
 */
#define NVM_VERSION_PATCH 1
#define NVM_VERSION_MINOR 0
#define NVM_VERSION_MAJOR 0

/*
 * Maximum size of the stack. Probably I should make it extendable in case of
 * overflow.
 */
#define STACK_SIZE 10
/* Variables stack max size */
#define VARS_STACK_SIZE 30
/* Functions stack max size */
#define FUNCS_STACK_SIZE 30

/*
 * Used for verbosity/debugging purposes.
 */
#define VERBOSE 1

/*
 * List of opcodes.
 */
#define NOP        0x00
#define LOAD_CONST 0x01
#define DISCARD    0x02
#define BINARY_ADD 0x03
#define BINARY_SUB 0x04
#define BINARY_MUL 0x05
#define BINARY_DIV 0x06
#define ROT_TWO    0x07
#define ROT_THREE  0x08
#define STORE      0x09
#define LOAD_NAME  0x0A
#define DUP        0x0B
#define FN_START   0x0C
#define FN_END     0x0D
#define CALL       0x0E
#define BEGIN_FN   0xBF

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
  /* The Stack */
  int stack[STACK_SIZE];
  /* stack 'pointer' */
  unsigned stack_ptr;
  /* variables stack */
  nvm_var vars[VARS_STACK_SIZE];
  /* variables 'pointer' */
  unsigned vars_ptr;
  /* functions offset (it's not unsigned because I'm using -1 later on for
     error-like purposes) */
  int functions_offset;
  /* functions stack */
  nvm_function funcs[FUNCS_STACK_SIZE];
  /* function 'pointer' */
  unsigned funcs_ptr;
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
