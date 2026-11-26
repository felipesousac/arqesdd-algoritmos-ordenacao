#ifndef SORT_ENGINE_H
#define SORT_ENGINE_H

#include "tad/headerFiles/list_sort_observer.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  int32_t type;
  int32_t first_index;
  int32_t second_index;
  int32_t first_value;
  int32_t second_value;
} Operation;

typedef struct {
  int32_t algorithm;
  int32_t structure_type;
  int32_t item_count;
  int64_t comparisons;
  int64_t swaps;
  double elapsed_ms;
  size_t memory_bytes;
  int32_t operation_count;
} Metrics;

enum {
  OPERATION_COMPARE = 0,
  OPERATION_SWAP = 1,
  OPERATION_WRITE = 2
};

enum {
  STRUCT_SEQUENTIAL = 0,
  STRUCT_SINGLY = 1,
  STRUCT_DOUBLY = 2,
  STRUCT_CIRCULAR = 3
};

int sort_run(const int32_t *input, int item_count, int algorithm, int structure_type, int requested_operation_limit);
const int32_t *sort_values(void);
const Metrics *sort_metrics(void);
const Operation *sort_operations(void);
const char *get_sort_algorithm_name(int algorithm);
const char *get_sort_structure_name(int structure_type);
void sort_free(void);

#endif
