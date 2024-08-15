#ifndef PARSE_H_
#define PARSE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  parse_ok,
  parse_err_indef_in_primitive = 200,
  parse_err_no_end_marker,
} parse_err_t;

extern char parse_err_message[512];

parse_err_t parse(const uint8_t *data, size_t data_len);

#endif
