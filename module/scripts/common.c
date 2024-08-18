#include "common.h"

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
