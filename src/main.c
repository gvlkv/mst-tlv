#include "binary.h"
#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
static void dbg_dump(size_t len, uint8_t *data) {
  for (size_t l = 0; l < len; l++) {
    printf("%02X", data[l]);
  }
  printf("\n");
}
#endif

static void help_msg(char *progname) { printf("%s <filename>\n", progname); }

int main(int argc, char **argv) {
  if (argc != 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    help_msg(argv[0]);
    return EXIT_SUCCESS;
  }

  const char *filename = argv[1];

  size_t len = 0;
  uint8_t *bytes = read_binary(filename, malloc, free, &len);

  if (!bytes) {
    fprintf(stderr, "ERROR: %s.\n", binary_err_message);
    return EXIT_FAILURE;
  }

  parse_err_t err = parse(bytes, len);
  if (err) {
    fprintf(stderr, "ERROR: %s.\n", parse_err_message);
    return EXIT_FAILURE;
  }

  free(bytes);

  return EXIT_SUCCESS;
}
