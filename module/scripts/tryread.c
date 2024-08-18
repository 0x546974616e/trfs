#include <stdlib.h>
#include <stdio.h>

#include "common.h"

// tryread /proc/trfs 2 6
// trywrite /proc/trfs dadafafa 2

int main(int const argc, char const* const argv[]) {
  if (argc < 2) {
    printf("Usage: %s FILENAME\n", filename(argv[0]));
    return EXIT_FAILURE;
  }

  return 0;
}
