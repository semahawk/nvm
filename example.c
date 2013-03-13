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

int main(int argc, char *argv[])
{
  /* don't complain about unused variable */
  (void)argc;

  if (!strcmp(argv[0], "--write")){
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
  nvm_destroy(vm);

  return 0;
}

