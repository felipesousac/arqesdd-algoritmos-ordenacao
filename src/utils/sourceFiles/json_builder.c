#include "../headerFiles/json_builder.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void json_builder_init(JsonBuilder *json) {
  if (!json) {
    return;
  }

  json->data = NULL;
  json->length = 0;
  json->capacity = 0;
  json->failed = 0;
}

void json_builder_append(JsonBuilder *json, const char *format, ...) {
  if (!json || json->failed) {
    return;
  }

  va_list args;
  va_start(args, format);

  va_list copy;
  va_copy(copy, args);

  int needed = vsnprintf(NULL, 0, format, copy);
  va_end(copy);

  if (needed < 0 || (size_t)needed > JSON_MAX_SIZE - json->length) {
    json->failed = 1;
    va_end(args);
    return;
  }

  size_t required = json->length + (size_t)needed + 1;

  if (required > json->capacity) {
    size_t capacity = json->capacity ? json->capacity : 4096;

    while (capacity < required) {
      capacity *= 2;
    }

    char *data = (char *)realloc(json->data, capacity);

    if (!data) {
      json->failed = 1;
      va_end(args);
      return;
    }

    json->data = data;
    json->capacity = capacity;
  }

  vsnprintf(json->data + json->length, json->capacity - json->length, format,
            args);
  json->length += (size_t)needed;
  va_end(args);
}

char *json_builder_detach(JsonBuilder *json) {
  if (!json) {
    return NULL;
  }

  if (json->failed || !json->data) {
    json_builder_free(json);
    return NULL;
  }

  char *result = json->data;
  json->data = NULL;
  json->length = 0;
  json->capacity = 0;
  json->failed = 0;

  return result;
}

void json_builder_free(JsonBuilder *json) {
  if (!json) {
    return;
  }

  free(json->data);
  json->data = NULL;
  json->length = 0;
  json->capacity = 0;
  json->failed = 0;
}
