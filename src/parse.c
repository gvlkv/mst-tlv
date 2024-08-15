#include "parse.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char parse_err_message[];

#define CLASS_TBL(X)                                                           \
  X(class_universal, "U")                                                      \
  X(class_app, "A")                                                            \
  X(class_context, "C")                                                        \
  X(class_private, "P")

typedef enum {
#define X(name, repr) name,
  CLASS_TBL(X)
#undef X
} class_t;

#define X(name, repr) [name] = repr,
const char *class_repr[] = {CLASS_TBL(X)};
#undef X

#define TYPE_TBL(X)                                                            \
  X(type_primitive, "P")                                                       \
  X(type_constructed, "C")

typedef enum {
#define X(name, repr) name,
  TYPE_TBL(X)
#undef X
} type_t;

#define X(name, repr) [name] = repr,
const char *type_repr[] = {TYPE_TBL(X)};
#undef X

typedef int id_t;

typedef int len_t;
const len_t length_indefinite_c = -1;

typedef struct {
  class_t class;
  type_t type;
  id_t id;
  len_t len;
  size_t header_size;
  size_t len_size;
} header_t;

static header_t read_header(const uint8_t *data, size_t data_len) {
  assert(data_len >= 1);
  id_t id = data[0] & 0x1f;
  type_t type = (data[0] & 0x20) >> 5;
  class_t class = (data[0] & 0xc0) >> 6;
  size_t header_size = 1;
  size_t len_size = 1;
  len_t len = 0;
  if (id == 31) {
    assert(data_len >= 2);
    id = 0;
    int i = 1;
    while (data[i++] & 0x80) {
      id = (id << 7) | (data[i] & 0x7f);
      header_size++;
    }

    id = (id << 7) | (data[i - 1] & 0x7f);
    header_size++;
  }

  int first_len = data[header_size];
  header_size++;
  if (!(first_len & 0x80)) {
    len = first_len & 0x7f;
  } else {
    int number_of_bytes = first_len & 0x7f;
    assert(number_of_bytes != 127 && "reserved");
    if (number_of_bytes == 0) {
      len = length_indefinite_c;
    } else {
      for (int i = 0; i < number_of_bytes; i++) {
        len = (len << 8) | data[header_size];
        len_size++;
        header_size++;
      }
    }
  }

  return (header_t){
      .class = class,
      .type = type,
      .id = id,
      .len = len,
      .header_size = header_size,
      .len_size = len_size,
  };
}

static void indent(int depth) {
  for (int i = 0; i < depth * 2; i++) {
    putc(' ', stdout);
  }
}

static void print_tag(class_t class, type_t type, id_t id) {
  printf("Tag (class: %s, kind: %s, id: %d)", class_repr[class],
         type_repr[type], id);
}

static void print_hex(const uint8_t *data, size_t size) {
  printf("[");
  for (size_t i = 0; i < size; i++)
    printf("%02x", data[i]);
  printf("]");
}

static void print_len(len_t len) {
  printf("Length: ");
  if (len == length_indefinite_c)
    printf("INDEFINITE");
  else
    printf("%d", len);
}

static void print_primitive(const uint8_t *data, size_t size) {
  printf("Value: ");
  print_hex(data, size);
}

static parse_err_t parse_inner(int depth, const uint8_t *data, size_t data_len,
                               const uint8_t **parse_end, bool *marker) {
  header_t header = read_header(data, data_len);
  class_t class = header.class;
  type_t type = header.type;
  id_t id = header.id;
  len_t len = header.len;
  size_t header_size = header.header_size;
  size_t len_size = header.len_size;
  indent(depth);
  print_tag(class, type, id);
  printf(" ");
  print_hex(data, header_size - len_size);
  printf("\n");
  indent(depth);
  print_len(len);
  printf(" ");
  print_hex(data + header_size - len_size, len_size);
  printf("\n");
  if (type == type_primitive) {
    if (len == length_indefinite_c) {
      sprintf(parse_err_message, "Indefinite length in primitive");
      return parse_err_indef_in_primitive;
    }
    indent(depth);
    print_primitive(data + header_size, len);
    printf("\n");
    *parse_end = data + header_size + len;
    *marker = class == class_universal && type == type_primitive && id == 0 &&
              len == 0;
  } else if (type == type_constructed) {
    if (len == length_indefinite_c) {
      int count = 1;
      for (const uint8_t *data_start = data + header_size;
           data_start < data + data_len;) {
        const uint8_t *end = NULL;
        bool inner_marker = false;
        indent(depth);
        printf("TLV #%d\n", count++);
        parse_err_t err = parse_inner(
            depth + 1, data_start, data_len - header_size, &end, &inner_marker);
        if (err) {
          return err;
        }
        if (inner_marker) {
          *parse_end = end;
          *marker = false;
          break;
        }
        data_start = end;
      }
      if (marker) {
        sprintf(parse_err_message, "No end marker for indefinite length");
        return parse_err_no_end_marker;
      }
    } else {
      int count = 1;
      for (const uint8_t *data_start = data + header_size;
           data_start < data + header_size + len;) {

        const uint8_t *end = NULL;
        bool inner_marker = false;
        indent(depth);
        printf("TLV #%d\n", count++);
        parse_err_t err = parse_inner(depth + 1, data_start,
                                      data + header_size + len - data_start,
                                      &end, &inner_marker);
        if (err) {
          return err;
        }
        data_start = end;
        *parse_end = end;
      }
    }
  }
  return parse_ok;
}

parse_err_t parse(const uint8_t *data, size_t data_len) {
  const uint8_t *current = data;
  int count = 1;
  do {
    bool marker;
    const uint8_t *end = NULL;
    printf("TLV #%d\n", count++);
    parse_err_t err = parse_inner(1, current, data_len, &end, &marker);
    if (err) {
      return err;
    }
    current = end;
  } while (current - data != (ptrdiff_t)data_len);
  return parse_ok;
}
