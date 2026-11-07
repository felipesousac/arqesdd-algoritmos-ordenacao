#ifndef SORT_ENGINE_H
#define SORT_ENGINE_H
#include <stdint.h>
typedef struct {
  int32_t type, first_index, second_index, first_value, second_value;
} Operation;
typedef struct {
  int32_t algorithm, item_count;
  int64_t comparisons, swaps;
  double elapsed_ms;
  int32_t operation_count;
} Metrics;
enum { OPERATION_COMPARE = 0, OPERATION_SWAP = 1, OPERATION_WRITE = 2 };
enum { ALGORITHM_BUBBLE = 0, ALGORITHM_SELECTION = 1, ALGORITHM_INSERTION = 2, ALGORITHM_QUICK = 3, ALGORITHM_MERGE = 4 };
int sort_run(const int32_t *input, int item_count, int algorithm, int requested_operation_limit);
const int32_t *sort_values(void);
const Metrics *sort_metrics(void);
const Operation *sort_operations(void);
void sort_free(void);
#endif
