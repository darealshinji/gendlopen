#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "test1.h"


void err_exit()
{
  // print error, free library, call exit()
  fprintf(stderr, "%s\n", gendlopen_zlib_last_error);
  gendlopen_zlib_free_lib;
  exit(1);
}

void print1()
{
  // reassign symbols in this scope
  gendlopen_zlib_load_sym(1, err_exit());

  printf("#1: %s\n", zlibVersion());
}

void print2()
{
  // reassign symbols in this scope
  gendlopen_zlib_load_sym(1, err_exit());

  printf("#2: %s\n", zlibVersion());
}

int main()
{
  // load library and symbols
  gendlopen_zlib_load_all_fn(LIB(z,1), err_exit());

  print1();

  // forces print2() to assert
  //gendlopen_zlib_free_lib;

  print2();
  printf("#3: %s\n", zlibVersion());

  // free library
  gendlopen_zlib_free_lib;

  return 0;
}

