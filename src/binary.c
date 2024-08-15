#include "binary.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

binary_err_t binary_err;
char binary_err_message[];

static bool is_digit(char chr) {
  return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') ||
         (chr >= 'A' && chr <= 'F');
}

uint8_t *read_binary(const char *filename, alloc_t alloc, free_t free_,
                     size_t *len) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    binary_err = bin_err_fileopen;
    sprintf(binary_err_message, "Error opening file %s: %s", filename,
            strerror(errno));
    return NULL;
  }
  size_t nibble_len = 0;
  size_t cur_read_len = 0;
  for (;;) {
    uint8_t chr;
    if (!fread(&chr, 1, 1, file))
      break;
    if (!(isspace(chr) || is_digit(chr))) {
      binary_err = bin_err_encoding;
      sprintf(binary_err_message, "Forbidden character code %x at %zu.", chr,
              cur_read_len);
      return NULL;
    }
    cur_read_len++;
    if (!isspace(chr))
      nibble_len++;
  }
  if (nibble_len % 2) {
    binary_err = bin_err_encoding;
    sprintf(binary_err_message, "Number of nibbles read is odd (%zu).",
            nibble_len);
    return NULL;
  }
  const size_t byte_len = nibble_len / 2;

  uint8_t *buf = alloc(byte_len);
  if (!buf) {
    binary_err = bin_err_memory;
    sprintf(binary_err_message, "Cannot allocate enough memory.");
    return NULL;
  }

  if (-1 == fseek(file, 0, SEEK_SET)) {
    binary_err = bin_err_fileopen;
    sprintf(binary_err_message, "File error: %s", strerror(errno));
    free_(buf);
    return NULL;
  }

  for (size_t cnt = 0; cnt < byte_len; cnt++) {
    char byte[3] = "  ";
    while (fread(&byte[0], 1, 1, file) == 1 && isspace(byte[0]))
      ;
    while (fread(&byte[1], 1, 1, file) == 1 && isspace(byte[1]))
      ;
    buf[cnt] = strtol(byte, NULL, 16);
  }

  *len = byte_len;
  return buf;
}
