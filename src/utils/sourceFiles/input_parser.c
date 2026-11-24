#include "../headerFiles/input_parser.h"
#include "../../sort_engine.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Dynamically parses CSV/space-separated integers using SequentialList */
int parse_values_dynamic(const char *text, int32_t **out_values,
                         int *out_count) {
  *out_values = NULL;
  *out_count = 0;

  if (text == NULL) {
    return 0;
  }

  SequentialList *list = seq_list_create(sizeof(int32_t));

  if (list == NULL) {
    return 0;
  }

  const char *cursor = text;

  while (*cursor) {
    while (*cursor == ' ' || *cursor == ',' || *cursor == ';' ||
           *cursor == '\n' || *cursor == '\r' || *cursor == '\t') {
      cursor++;
    }

    if (!*cursor) {
      break;
    }

    if (seq_list_size(list) >= API_MAX_INPUT) {
      seq_list_destroy(list);
      return 0;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(cursor, &end, 10);

    if (end == cursor || errno != 0 || value < 1 || value > 1000000) {
      seq_list_destroy(list);
      return 0;
    }

    int32_t item = (int32_t)value;

    if (!seq_list_insert_end(list, &item)) {
      seq_list_destroy(list);
      return 0;
    }

    if (*end && *end != ' ' && *end != ',' && *end != ';' && *end != '\n' &&
        *end != '\r' && *end != '\t') {
      seq_list_destroy(list);
      return 0;
    }

    cursor = end;
  }

  int count = seq_list_size(list);

  if (count < 2) {
    seq_list_destroy(list);
    return 0;
  }

  int32_t *parsed_numbers = (int32_t *)malloc((size_t)count * sizeof(int32_t));

  if (parsed_numbers == NULL) {
    seq_list_destroy(list);
    return 0;
  }

  memcpy(parsed_numbers, seq_list_data(list), (size_t)count * sizeof(int32_t));
  seq_list_destroy(list);

  *out_values = parsed_numbers;
  *out_count = count;

  return 1;
}

int parse_id_list(const char *text, int min_id, int max_id, SequentialList *out_ids) {
  if (text == NULL || text[0] == '\0') {
    return 0;
  }

  const char *cursor = text;

  while (*cursor) {
    while (*cursor == ' ' || *cursor == ',' || *cursor == ';') {
      cursor++;
    }

    if (!*cursor) {
      break;
    }

    errno = 0;
    char *end = NULL;
    long val = strtol(cursor, &end, 10); // String to long

    if (end == cursor || errno != 0 || val < min_id || val > max_id) {
      return 0;
    }

    int id = (int) val;

    int duplicate = 0;

    for (int i = 0; i < seq_list_size(out_ids); i++) {
      int existing = -1;

      seq_list_get(out_ids, i, &existing);

      if (existing == id) {
        duplicate = 1;
        break;
      }
    }

    if (!duplicate) {
      if (!seq_list_insert_end(out_ids, &id)) {
        return 0;
      }
    }
    
    cursor = end;
  }

  return seq_list_size(out_ids) > 0;
}

int parse_int_in_range(const char *text, int min_val, int max_val,
                       int *out_value) {
  if (text == NULL || text[0] == '\0' || out_value == NULL) {
    return 0;
  }

  errno = 0;
  char *end = NULL;
  long value = strtol(text, &end, 10);

  if (end == text || *end != '\0' || errno != 0 || value < min_val ||
      value > max_val) {
    return 0;
  }

  *out_value = (int)value;
  return 1;
}

int parse_test_id(const char *text, uint64_t *out_id) {
  if (text == NULL || text[0] == '\0' || text[0] == '-' || out_id == NULL) {
    return 0;
  }

  errno = 0;
  char *end = NULL;
  unsigned long long value = strtoull(text, &end, 10); // string to unsigned long long

  if (end == text || *end != '\0' || errno != 0 || value == 0) {
    return 0;
  }

  *out_id = (uint64_t) value;

  return 1;
}

const char *resolve_algorithm_filter(const char *algorithm_filter) {
  if (algorithm_filter == NULL) {
    return "";
  }

  if (algorithm_filter[0] >= '0' && algorithm_filter[0] <= '4' &&
      algorithm_filter[1] == '\0') {
    return get_sort_algorithm_name(algorithm_filter[0] - '0');
  }

  if (strcmp(algorithm_filter, "all") == 0) {
    return "";
  }

  return algorithm_filter;
}
