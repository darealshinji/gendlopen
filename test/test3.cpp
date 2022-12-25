#include <iostream>
#include <zlib.h>
#include "test3.hpp"


void print1(gendlopen_zlib *gen) {
  GENDLOPEN_ZLIB_RELOAD_SYMBOLS(gen->);
  std::cout << "#1: " << zlibVersion() << std::endl;
}

void print2(gendlopen_zlib &gen) {
  GENDLOPEN_ZLIB_RELOAD_SYMBOLS(gen.);
  std::cout << "#2: " << zlibVersion() << std::endl;
}

int main()
{
  GENDLOPEN_ZLIB_LOCAL_DECL;
  gendlopen_zlib gen(LIB(z,1));

  if (!gen.load() || !GENDLOPEN_ZLIB_LOAD_SYMBOLS(gen.)) {
    std::cerr << gen.error() << std::endl;
    return 1;
  }

  print1(&gen);
  print2(gen);
  std::cout << "#3: " << zlibVersion() << std::endl;

  return 0;
}
