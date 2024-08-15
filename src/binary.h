#ifndef BINARY_H_
#define BINARY_H_

#include <stddef.h>
#include <stdint.h>

typedef enum {
  bin_ok,
  bin_err_fileopen = 100,
  bin_err_encoding,
  bin_err_memory
} binary_err_t;

extern binary_err_t binary_err;
extern char binary_err_message[512];

typedef void *alloc_t(size_t size);
typedef void free_t(void *);

/** returns allocated buffer, NULL on error */
uint8_t *read_binary(const char *filename, alloc_t alloc, free_t free,
                     size_t *len);

#endif
