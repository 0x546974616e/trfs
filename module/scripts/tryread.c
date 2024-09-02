#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

/** @pre string Non-NULL. */
unsigned int parse_block(char const* const string) {
  int block = atoi(string);
  // Allow block to be zero for tests.
  return block < 0 ? 0u : (unsigned int) block;
}

/** @pre string Non-NULL. */
unsigned int parse_count(char const* const string) {
  int count = atoi(string);
  return count <= 0 ? 1u : (unsigned int) count;
}

int main(int const argc, char const* const argv[]) {
  if (argc < 3) {
    printf("Usage: %s FILE BLOCK [COUNT]\n", filename(argv[0]));
    return EXIT_FAILURE;
  }

  int retcode = EXIT_SUCCESS;
  char const* const file = argv[1];
  unsigned int const block = parse_block(argv[2]);
  unsigned int const count = argc >= 4 ? parse_count(argv[3]) : 1;
  unsigned int const count_digits = digits(count);

  printf("Filename: %s\n", file);
  printf("Block size: %u\n", block);
  printf("Count: %u\n", count);

  int const fd = open(file, O_RDONLY);
  if (fd == -1) {
    perror("open(file) failed");
    // exit(errno); // Why not?
    return EXIT_FAILURE;
  }

  char* buffer = (char*) malloc(sizeof(char) * (block + 1));
  if (buffer == NULL) {
    perror("malloc(block) failed");
    retcode = EXIT_FAILURE;
    goto teardown;
  }

  for (unsigned int i = 0; i < count; ++i) {
    ssize_t size = read(fd, buffer, block);

    if (size == -1) {
      perror("read(file) failed");
      retcode = EXIT_FAILURE;
      break; // goto teardown;
    }

    if (size <= 0) {
      printf("Read[%0*u]: EOF", count_digits, i);
      break;
    }

    // Buffer size is block + 1.
    buffer[size] = 0x0;
    buffer[block] = 0x0;

    for (char* pointer = buffer; *pointer; ++pointer) {
      if (!isprint(*pointer)) {
        *pointer = '.';
      }
    }

    printf("Read[%0*u]: %ld \"%s\"\n", count_digits, i, size, buffer);
  }

teardown:
  if (buffer != NULL) {
    free(buffer);
  }

  if (close(fd) == -1) {
    perror("close(file) failed");
    return EXIT_FAILURE;
  }

  return retcode;
}
