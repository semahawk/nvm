/*
 * nvm.h
 *
 */

#ifndef NVM_H
#define NVM_H

#include <stdbool.h>

/*
 * Obviously.
 */
#define NVM_VERSION_PATCH 1
#define NVM_VERSION_MINOR 2
#define NVM_VERSION_MAJOR 3

/*
 * Maximum size of the stack. Probably I should make it extendable in case of
 * overflow.
 */
#define STACK_SIZE 10

/*
 * Used for debugging purposes.
 */
#define DEBUG

/*
 * List of opcodes.
 */
#define PUSH       0x00
#define POP        0x01
#define BINARY_ADD 0x02
#define BINARY_SUB 0x03
#define BINARY_MUL 0x04
#define BINARY_DIV 0x05

/*
 * The main type for NVM.
 */
typedef struct {
  /* TODO: populate! */
} nvm_t;

/*
 * name:        push
 * description: pushes given <value> to the stack
 */
void push(int value);

/*
 * name:        pop
 * description: returns the top-most value from the stack, and reduces it's size
 */
int pop(void);

/*
 * name:        binop
 * description: pops a value twice, and performs a binary <operation> on those
 *              operands, and pushes the result
 */
void binop(unsigned operation);

/*
 * name:        print_stack
 * description: prints what's left on stack
 */
void print_stack(void);

/*
 * name:        debug
 * description: a handy function which prints <message> in the following format:
 *
 *              <number>: <message><newline>
 *
 *              where <number> is current action number (variable `pc` in nvm.c)
 */
void debug(const char *message, ...);

/*
 * name:        write_line
 * description: writes a line to `file_p` in a binary format (hopefully)
 */
void write_line(unsigned operation);

/*
 * name:        is_empty
 * description: return `true` if the stack is empty, otherwise `false`
 */
bool is_empty(void);

/*
 * name:        nvm_init
 * description: creates a new `nvm_t` object, and return pointer to it
 * parameter:   filename - name of a file into which the bytecode will be
 *              printed and from which the bytecode will be extracted in order
 *              to execute the operations.
 * return:      pointer to a new malloced object or NULL if malloc failed
 */
nvm_t *nvm_init(const char *filename);

/*
 * name:        nvm_destroy
 * description: cleans up after everything (which includes fclosing the file and
 *              freeing the malloced pointer)
 */
void nvm_destroy(nvm_t *virtual_machine);

/* Lemon stuff */
void *ParseAlloc(void *(*)(size_t));
void  Parse(void *, int, int);
void  ParseFree(void *, void (*)(void*));

#endif /* NVM_H */
