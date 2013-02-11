/*
 *
 * example.c
 *
 * Created at:  02/10/2013 04:29:53 PM
 *
 * Author:  Szymon Urbas <szymon.urbas@aol.com>
 *
 * License: the MIT license
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "nvm.h"
#include "grammar.h"

int main(void)
{
  void *parser = ParseAlloc(malloc);
  nvm_t *vm = nvm_init("bytecode.nc");

  if (!vm){
    fprintf(stderr, "error :C\n");
    return 1;
  }

  /* input: (2 + 2) * 2 / 4 * (10 + 4) + 9 */
  Parse(parser, LPAREN, 0);
  Parse(parser, NUMBER, 2);
  Parse(parser, PLUS, 0);
  Parse(parser, NUMBER, 2);
  Parse(parser, RPAREN, 0);
  Parse(parser, TIMES, 0);
  Parse(parser, NUMBER, 2);
  Parse(parser, DIVIDE, 0);
  Parse(parser, NUMBER, 4);
  Parse(parser, TIMES, 0);
  Parse(parser, LPAREN, 0);
  Parse(parser, NUMBER, 10);
  Parse(parser, PLUS, 0);
  Parse(parser, NUMBER, 4);
  Parse(parser, RPAREN, 0);
  Parse(parser, PLUS, 0);
  Parse(parser, NUMBER, 9);
  Parse(parser, 0, 0);

  /* starts off the reading from file and printing the ops */
  nvm_blastoff(vm);

  ParseFree(parser, free);
  nvm_destroy(vm);

  return 0;
}

