/*
 *
 * example.c
 *
 * Created at:  02/10/2013 04:29:53 PM
 *
 * Author:  Szymon Urbaś <szymon.urbas@aol.com>
 *
 * License: the MIT license
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvm.h"
#include "grammar.h"

FILE *fp;

typedef union {
  int i;
  char *s;
} TokenType;

/* Lemon stuff */
void *ParseAlloc(void *(*)(size_t));
void  Parse(void *, int, TokenType);
void  ParseFree(void *, void (*)(void*));

int main(int argc, char *argv[])
{
  /* don't complain about unused variable */
  (void)argc;

  if (argc > 1 && !strcmp(argv[1], "--write")){
    /* opening the testing file */
    fp = fopen("bytecode.nc", "wb");
    /* writing version numbers */
    BYTE major = NVM_VERSION_MAJOR;
    BYTE minor = NVM_VERSION_MINOR;
    BYTE patch = NVM_VERSION_PATCH;

    fwrite(&major, sizeof major, 1, fp);
    fwrite(&minor, sizeof minor, 1, fp);
    fwrite(&patch, sizeof patch, 1, fp);
    /* end */

    void *parser = ParseAlloc(malloc);

    /* input:
     *
     *   b = 6 + 4 + 59;
     *   a = 4 + 51;
     */
    TokenType token;
    token.s = "b";
    Parse(parser, STRING, token);
    token.i = 0;
    Parse(parser, EQ, token);
    token.i = 6;
    Parse(parser, NUMBER, token);
    token.i = 0;
    Parse(parser, PLUS, token);
    token.i = 4;
    Parse(parser, NUMBER, token);
    token.i = 0;
    Parse(parser, PLUS, token);
    token.i = 59;
    Parse(parser, NUMBER, token);
    token.i = 0;
    Parse(parser, SEMICOLON, token);
    token.s = "a";
    Parse(parser, STRING, token);
    token.i = 0;
    Parse(parser, EQ, token);
    token.i = 4;
    Parse(parser, NUMBER, token);
    token.i = 0;
    Parse(parser, PLUS, token);
    token.i = 51;
    Parse(parser, NUMBER, token);
    token.i = 0;
    Parse(parser, SEMICOLON, token);
    /* finish parsing */
    token.i = 0;
    Parse(parser, 0, token);
    fclose(fp);
    ParseFree(parser, free);
  }

  /* init the VM */
  nvm_t *vm = nvm_init(malloc, "bytecode.nc");

  if (!vm){
    fprintf(stderr, "error with initializing the VM :C\n");
    return 1;
  }

  /* starts off the reading from file and executing the ops process */
  nvm_blastoff(vm);
  /* print what's on the stack */
  nvm_print_stack(vm);
  /* clean after yourself */
  nvm_destroy(free, vm);

  return 0;
}

