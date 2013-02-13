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

  void write_push(int);
  void write_binop(BYTE);

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
}

expr ::= expr PLUS expr. {
  write_binop(BINARY_ADD);
}
expr ::= expr MINUS expr. {
  write_binop(BINARY_SUB);
}
expr ::= expr TIMES expr. {
  write_binop(BINARY_MUL);
}
/* normally there would be some 'zero division' checking, but screw, the tokens
 are hard-coded */
expr ::= expr DIVIDE expr. {
  write_binop(BINARY_DIV);
}
expr ::= NUMBER(number). {
  write_push(number);
}
expr(res) ::= LPAREN expr(inside) RPAREN. {
  res = inside;
}

%code {
  void write_push(int value){
    BYTE op = PUSH;
    fwrite(&pc, sizeof(pc), 1, fp);
    fwrite(&op, sizeof(op), 1, fp);
    fwrite(&value, sizeof(value), 1, fp);
    pc++;
  }

  void write_binop(BYTE op){
    fwrite(&pc, sizeof(pc), 1, fp);
    fwrite(&op, sizeof(op), 1, fp);
    pc++;
  }
}
