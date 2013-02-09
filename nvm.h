/*
 * nvm.h
 *
 */

#ifndef NVM_H
#define NVM_H

#include <stdbool.h>

/*
 * Type of a binary operation.
 */
typedef enum {
  ADD,
  SUB,
  MUL,
  DIV
} BINOP;

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
void binop(BINOP operation);

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
 *              where <number> is current action number (variable `pc` in vm.c)
 */
void debug(const char *message, ...);

/* Lemon stuff */
void *ParseAlloc(void *(*)(size_t));
void  Parse(void *, int, int);
void  ParseFree(void *, void (*)(void*));

#endif /* NVM_H */
