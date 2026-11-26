#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H

#include "../../tad/headerFiles/sequential_list.h"
#include <stdint.h>

#define API_MAX_INPUT 300

int parse_values_dynamic(const char *text, int32_t **out_values,
                         int *out_count);
int parse_id_list(const char *text, int min_id, int max_id,
                  SequentialList *out_ids);
int parse_int_in_range(const char *text, int min_val, int max_val,
                       int *out_value);
int parse_test_id(const char *text, uint64_t *out_id);
const char *resolve_algorithm_filter(const char *algorithm_filter);

#endif
