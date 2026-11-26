#ifndef LIST_SORT_OBSERVER_H
#define LIST_SORT_OBSERVER_H

#include <stdint.h>

typedef enum {
  ALGORITHM_BUBBLE = 0,
  ALGORITHM_SELECTION = 1,
  ALGORITHM_INSERTION = 2,
  ALGORITHM_QUICK = 3,
  ALGORITHM_MERGE = 4
} SortAlgorithm;

typedef struct {
  void (*on_compare)(int first_index, int second_index, int32_t first_value, int32_t second_value);
  void (*on_swap)(int first_index, int second_index, int32_t new_first_value, int32_t new_second_value);
  void (*on_write)(int index, int32_t value, int count_as_swap);
} ListSortObserver;

#endif
