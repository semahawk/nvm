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

  #include "nvm.h"
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
  /*print_stack();*/
}

expr ::= NUMBER.

/*expr(res) ::= expr(left) PLUS   expr(right). { binop(BINARY_ADD); res = left + right; }*/
/*expr(res) ::= expr(left) MINUS  expr(right). { binop(BINARY_SUB); res = left - right; }*/
/*expr(res) ::= expr(left) TIMES  expr(right). { binop(BINARY_MUL); res = left * right; }*/
/* normally there would be some 'zero division' checking, but screw, the tokens
 * are hard-coded */
/*expr(res) ::= expr(left) DIVIDE expr(right). { binop(BINARY_DIV); res = left / right; }*/
/*expr(res) ::= NUMBER(number).                { push(number); res = number; }*/
/*expr(res) ::= LPAREN expr(inside) RPAREN.    { res = inside; }*/

