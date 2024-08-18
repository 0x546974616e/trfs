#include <math.h>
#include <stdlib.h>

#include "common.h"

unsigned int digits(unsigned int n) {
  // Performance is not important.
  return n == 0 ? 1 : (unsigned int) floor(log10(n)) + 1;
}

/** @param path Null-terminated string. */
char const* filename(char const* path) {
  char const* filename = path;

  while (*path != 0x0) {
    if (*path == '/' && *(path + 1) != '/') {
      filename = path + 1;
    }

    ++path;
  }

  return filename;
}
