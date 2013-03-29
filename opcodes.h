/*
 *
 * opcodes.h
 *
 * Created at:  03/21/2013 02:38:28 PM
 *
 * Author:  Szymon Urbaś <szymon.urbas@aol.com>
 *
 * License: the MIT license
 *
 */

/*
 * List of opcodes supported by NVM
 */

#ifndef OPCODES_H
#define OPCODES_H

/* No operation */
#define NOP                                 0x00
/* Push a constant to the stack */
#define LOAD_CONST                          0x01
/* Dispard the FOS */
#define DISCARD                             0x02
/* Pop FOS and SOS, add them, push the result to the stack */
#define BINARY_ADD                          0x03
/* Same, but substract */
#define BINARY_SUB                          0x04
/* Same, but multiply */
#define BINARY_MUL                          0x05
/* Same, but divide */
#define BINARY_DIV                          0x06
/* Swap the FOS with SOS */
#define ROT_TWO                             0x07
/* Pop the FOS, lift SOS and TOS one position higher, and push FOS to the third
 * position*/
#define ROT_THREE                           0x08
/* Stores a variable, with a value of FOS */
#define STORE                               0x09
/* Pushes the given variables value to the stack */
#define LOAD_NAME                           0x0A
/* Duplicate the FOS */
#define DUP                                 0x0B
/* Indicates the beginning of a function body */
#define FN_START                            0x0C
/* Indicates the end of a function body */
#define FN_END                              0x0D
/* Call a function */
#define CALL                                0x0E
/* Entering a block */
#define ENTER_BLOCK                         0x0F
/* Leaving a block */
#define LEAVE_BLOCK                         0x10

#endif /* OPCODES_H */
