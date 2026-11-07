#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sort_engine.h"

static double current_time_ms(void) {
  return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

#define MAX_ITEMS 4096
#define MAX_OPERATIONS 2000000
#define INITIAL_OPERATION_CAPACITY 1024

static int32_t values[MAX_ITEMS];
static int32_t merge_buffer[MAX_ITEMS];
static Operation *operations;
static int operation_count;
static int operation_capacity;
static int operation_limit = MAX_OPERATIONS;
static Metrics metrics;

/* Operation history ------------------------------------------------------ */

static void record_operation(int type, int first_index, int second_index,
                             int first_value, int second_value) {
  if (operation_count >= operation_limit) {
    return;
  }

  if (operation_count == operation_capacity) {
    int new_capacity = operation_capacity == 0
                           ? INITIAL_OPERATION_CAPACITY
                           : operation_capacity * 2;
    if (new_capacity > operation_limit) {
      new_capacity = operation_limit;
    }

    Operation *resized_operations =
        (Operation *)realloc(operations,
                             (size_t)new_capacity * sizeof(Operation));
    if (resized_operations == NULL) {
      return;
    }

    operations = resized_operations;
    operation_capacity = new_capacity;
  }

  operations[operation_count++] = (Operation){
      type, first_index, second_index, first_value, second_value};
}

static void reset_operation_history(int requested_limit) {
  free(operations);
  operations = NULL;
  operation_count = 0;
  operation_capacity = 0;

  operation_limit = requested_limit > 0 && requested_limit < MAX_OPERATIONS
                        ? requested_limit
                        : MAX_OPERATIONS;
}

/* Comparisons and array changes ----------------------------------------- */

static void record_comparison(int first_index, int second_index,
                              int32_t first_value, int32_t second_value) {
  metrics.comparisons++;
  record_operation(OPERATION_COMPARE, first_index, second_index,
                   first_value, second_value);
}

static int value_at_is_less(int first_index, int second_index) {
  record_comparison(first_index, second_index, values[first_index],
                    values[second_index]);
  return values[first_index] < values[second_index];
}

static void swap_values_at(int first_index, int second_index) {
  if (first_index == second_index) {
    return;
  }

  int32_t first_value = values[first_index];
  int32_t second_value = values[second_index];

  values[first_index] = second_value;
  values[second_index] = first_value;
  metrics.swaps++;

  record_operation(OPERATION_SWAP, first_index, second_index,
                   second_value, first_value);
}

/* `count_as_swap` keeps the metric compatible with the reference project:
 * insertion shifts and merge writes count as movements, but placing the
 * insertion key in its final slot does not. */
static void write_value_at(int index, int32_t value, int count_as_swap) {
  values[index] = value;

  if (count_as_swap) {
    metrics.swaps++;
  }

  record_operation(OPERATION_WRITE, index, -1, value, count_as_swap);
}

/* Sorting algorithms ----------------------------------------------------- */

static void bubble_sort(void) {
  int made_swap;

  do {
    made_swap = 0;

    for (int index = 0; index < metrics.item_count - 1; index++) {
      if (value_at_is_less(index + 1, index)) {
        swap_values_at(index, index + 1);
        made_swap = 1;
      }
    }
  } while (made_swap);
}

static void selection_sort(void) {
  for (int position = 0; position < metrics.item_count - 1; position++) {
    int smallest_index = position;

    for (int candidate = position + 1; candidate < metrics.item_count;
         candidate++) {
      if (value_at_is_less(candidate, smallest_index)) {
        smallest_index = candidate;
      }
    }

    swap_values_at(position, smallest_index);
  }
}

static void insertion_sort(void) {
  for (int position = 1; position < metrics.item_count; position++) {
    int32_t value_to_insert = values[position];
    int previous_position = position - 1;

    while (previous_position >= 0) {
      record_comparison(previous_position, position,
                        values[previous_position], value_to_insert);

      if (values[previous_position] <= value_to_insert) {
        break;
      }

      write_value_at(previous_position + 1, values[previous_position], 1);
      previous_position--;
    }

    if (previous_position + 1 != position) {
      write_value_at(previous_position + 1, value_to_insert, 0);
    }
  }
}

static int partition_around_last_value(int low, int high) {
  int32_t pivot_value = values[high];
  int last_smaller_index = low - 1;

  for (int scan_index = low; scan_index < high; scan_index++) {
    record_comparison(scan_index, high, values[scan_index], pivot_value);

    if (values[scan_index] < pivot_value) {
      last_smaller_index++;
      swap_values_at(last_smaller_index, scan_index);
    }
  }

  int pivot_index = last_smaller_index + 1;
  swap_values_at(pivot_index, high);
  return pivot_index;
}

static void quick_sort_range(int low, int high) {
  if (low >= high) {
    return;
  }

  int pivot_index = partition_around_last_value(low, high);
  quick_sort_range(low, pivot_index - 1);
  quick_sort_range(pivot_index + 1, high);
}

static void merge_sorted_ranges(int low, int middle, int high) {
  int left_index = low;
  int right_index = middle + 1;
  int output_index = low;

  while (left_index <= middle && right_index <= high) {
    record_comparison(left_index, right_index, values[left_index],
                      values[right_index]);

    if (values[left_index] <= values[right_index]) {
      merge_buffer[output_index++] = values[left_index++];
    } else {
      merge_buffer[output_index++] = values[right_index++];
    }
  }

  while (left_index <= middle) {
    merge_buffer[output_index++] = values[left_index++];
  }

  while (right_index <= high) {
    merge_buffer[output_index++] = values[right_index++];
  }

  for (int index = low; index <= high; index++) {
    write_value_at(index, merge_buffer[index], 1);
  }
}

static void merge_sort_range(int low, int high) {
  if (low >= high) {
    return;
  }

  int middle = low + (high - low) / 2;
  merge_sort_range(low, middle);
  merge_sort_range(middle + 1, high);
  merge_sorted_ranges(low, middle, high);
}

static void run_selected_algorithm(int algorithm) {
  switch (algorithm) {
  case ALGORITHM_BUBBLE:
    bubble_sort();
    break;
  case ALGORITHM_SELECTION:
    selection_sort();
    break;
  case ALGORITHM_INSERTION:
    insertion_sort();
    break;
  case ALGORITHM_QUICK:
    quick_sort_range(0, metrics.item_count - 1);
    break;
  case ALGORITHM_MERGE:
    merge_sort_range(0, metrics.item_count - 1);
    break;
  }
}

/* Public sorting API ---------------------------------------------------- */

int sort_run(const int32_t *input, int item_count, int algorithm,
                    int requested_operation_limit) {
  if (input == NULL || item_count < 1 || item_count > MAX_ITEMS ||
      algorithm < ALGORITHM_BUBBLE || algorithm > ALGORITHM_MERGE) {
    return 0;
  }

  memcpy(values, input, (size_t)item_count * sizeof(int32_t));
  reset_operation_history(requested_operation_limit);

  memset(&metrics, 0, sizeof(metrics));
  metrics.algorithm = algorithm;
  metrics.item_count = item_count;

  double start_time_ms = current_time_ms();
  run_selected_algorithm(algorithm);
  metrics.elapsed_ms = current_time_ms() - start_time_ms;
  metrics.operation_count = operation_count;

  return 1;
}

const int32_t *sort_values(void) {
  return values;
}

const Metrics *sort_metrics(void) {
  return &metrics;
}

const Operation *sort_operations(void) {
  return operations;
}

void sort_free(void) {
  free(operations);
  operations = NULL;
  operation_count = 0;
  operation_capacity = 0;
}
