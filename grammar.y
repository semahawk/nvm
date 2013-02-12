/*
 * grammar.y
 *
 * A simple grammar, just to make it work.
 *
 */

%token_type { int }

%left PLUS MINUS.
%left TIMES DIVIDE.
%left LPAREN.

%include {
  #include <assert.h>
  #include <stdlib.h>
  #include <stdint.h>

  #include "nvm.h"

  static uint16_t pc = 0;

  extern FILE *fp;
}

%token_destructor {
  /* NULL */
}

%stack_overflow {
  /* NULL */
}

%syntax_error {
  /* NULL */
}

source ::= expr .
{
  print_stack();
}

expr ::= expr PLUS expr. {
  BYTE op = BINARY_ADD;
  /*printf("grammar: %02x: add\n", pc);*/
  fwrite(&pc, sizeof(pc), 1, fp);
  fwrite(&op, sizeof(op), 1, fp);
  pc++;
}
expr ::= expr MINUS expr. {
  BYTE op = BINARY_SUB;
  /*printf("grammar: %02x: sub\n", pc);*/
  fwrite(&pc, sizeof(pc), 1, fp);
  fwrite(&op, sizeof(op), 1, fp);
  pc++;
}
expr ::= expr TIMES expr. {
  BYTE op = BINARY_MUL;
  /*printf("grammar: %02x: mul\n", pc);*/
  fwrite(&pc, sizeof(pc), 1, fp);
  fwrite(&op, sizeof(op), 1, fp);
  pc++;
}
/* normally there would be some 'zero division' checking, but screw, the tokens
 are hard-coded */
expr ::= expr DIVIDE expr. {
  BYTE op = BINARY_DIV;
  /*printf("grammar: %02x: div\n", pc);*/
  fwrite(&pc, sizeof(pc), 1, fp);
  fwrite(&op, sizeof(op), 1, fp);
  pc++;
}
expr ::= NUMBER(number). {
  BYTE op = PUSH;
  /*printf("grammar: %02x: push %d\n", pc, number);*/
  fwrite(&pc, sizeof(pc), 1, fp);
  fwrite(&op, sizeof(op), 1, fp);
  fwrite(&number, sizeof(number), 1, fp);
  pc++;
}
expr(res) ::= LPAREN expr(inside) RPAREN. {
  res = inside;
}

