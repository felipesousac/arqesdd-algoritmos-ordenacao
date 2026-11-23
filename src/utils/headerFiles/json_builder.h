#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include <stddef.h>

#define JSON_MAX_SIZE (32 * 1024 * 1024)

typedef struct {
  char *data;
  size_t length;
  size_t capacity;
  int failed;
} JsonBuilder;

void json_builder_init(JsonBuilder *json);
void json_builder_append(JsonBuilder *json, const char *format, ...);
char *json_builder_detach(JsonBuilder *json);
void json_builder_free(JsonBuilder *json);

#endif
