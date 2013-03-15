/*
 * grammar.y
 *
 * A simple grammar, just to make it work.
 *
 */

%token_type { TokenType }

%type NUMBER { TokenType }
%type STRING { TokenType }

%right EQ.
%left  PLUS MINUS.
%left  TIMES DIVIDE.
%left  LPAREN.

%include {
  #include <assert.h>
  #include <stdlib.h>
  #include <stdint.h>
  #include <string.h>

  #include "nvm.h"

  void write_push(int);
  void write_binop(BYTE);
  void write_store(BYTE, char *);
  void write_get(BYTE, char *);

  typedef union {
    int i;
    char *s;
  } TokenType;

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

source ::= stmts . {}

stmts ::= expr . {}
stmts ::= stmts SEMICOLON expr . {}

expr ::= STRING(name) EQ expr . {
  write_store(STORE, name.s);
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
  write_push(number.i);
}
expr ::= STRING(var). {
  write_get(GET, var.s);
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

  void write_store(BYTE op, char *name){
    size_t size = strlen(name);
    fwrite(&pc, sizeof(pc), 1, fp);
    fwrite(&op, sizeof(op), 1, fp);
    fwrite(&size, sizeof(unsigned char), 1, fp);
    fwrite(name, strlen(name) * sizeof(char), 1, fp);
    pc++;
  }

  void write_get(BYTE op, char *name){
    size_t size = strlen(name);
    fwrite(&pc, sizeof(pc), 1, fp);
    fwrite(&op, sizeof(op), 1, fp);
    fwrite(&size, sizeof(unsigned char), 1, fp);
    fwrite(name, strlen(name) * sizeof(char), 1, fp);
    pc++;
  }
}
