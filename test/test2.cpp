#include <iostream>
#include <zlib.h>
#include "test2.hpp"


void print1() {
  std::cout << "#1: " << zlibVersion() << std::endl;
}

void print2() {
  std::cout << "#2: " << zlibVersion() << std::endl;
}

int main()
{
  gendlopen_zlib gen;

  if (!gen.load( {"libz.so.X", "libz.so.1", "libz.so"} ) ||
      !gen.sym())
  {
    std::cerr << gen.error() << std::endl;
    return 1;
  }

  std::cout << "loaded: " << gen.lib() << " (" << gen.origin() << ')' << std::endl;

  print1();
  print2();
  std::cout << "#3: " << zlibVersion() << std::endl;

  return 0;
}
