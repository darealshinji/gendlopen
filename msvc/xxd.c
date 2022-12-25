/**
 * Simplified implementation of xxd.
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * For more information, please refer to <http://unlicense.org/
 */
#include <stdio.h>

int main()
{
  unsigned char buf[12];
  char first_line = 1;
  size_t n;

  while ((n = fread(&buf, 1, sizeof(buf), stdin)))
  {
    if (first_line) {
      first_line = 0;
    } else {
      printf(",\n");
    }

    printf("  0x%02x", buf[0]);

    if (n > 1) {
      for (size_t i = 1; i < n; i++) {
        printf(", 0x%02x", buf[i]);
      }
    }
  }
  printf("\n");

  return 0;
}
