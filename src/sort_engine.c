#include "sort_engine.h"
#include "tad/headerFiles/circular_linked_list.h"
#include "tad/headerFiles/doubly_linked_list.h"
#include "tad/headerFiles/sequential_list.h"
#include "tad/headerFiles/singly_linked_list.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ITEMS 4096
#define MAX_OPERATIONS 2000000

static int32_t *sorted_values_buffer = NULL;
static SequentialList *operation_history = NULL;
static int operation_limit = MAX_OPERATIONS;
static Metrics metrics;

static double current_time_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
  }
  return (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}

const char *sort_algorithm_name(int algorithm) {
  static const char *names[] = {"Bubble Sort", "Selection Sort",
                                "Insertion Sort", "Quick Sort", "Merge Sort"};
  return (algorithm >= ALGORITHM_BUBBLE && algorithm <= ALGORITHM_MERGE)
             ? names[algorithm]
             : "Unknown Algorithm";
}

const char *sort_structure_name(int structure_type) {
  static const char *names[] = {"Sequential List (Dynamic Array)",
                                "Singly Linked List", "Doubly Linked List",
                                "Circular Linked List"};
  return (structure_type >= STRUCT_SEQUENTIAL &&
          structure_type <= STRUCT_CIRCULAR)
             ? names[structure_type]
             : "Unknown Structure";
}

/* Operation history via SequentialList TAD ------------------------------- */

static void record_operation(int type, int first_index, int second_index,
                             int32_t first_value, int32_t second_value) {
  if (operation_history == NULL ||
      seq_list_size(operation_history) >= operation_limit) {
    return;
  }

  Operation op = {type, first_index, second_index, first_value, second_value};
  seq_list_insert_end(operation_history, &op);
}

static int reset_operation_history(int requested_limit) {
  if (operation_history != NULL) {
    seq_list_destroy(operation_history);
    operation_history = NULL;
  }

  operation_limit = (requested_limit > 0 && requested_limit < MAX_OPERATIONS)
                        ? requested_limit
                        : MAX_OPERATIONS;

  operation_history = seq_list_create_with_capacity(sizeof(Operation), 256);
  return operation_history != NULL;
}

/* Observer callbacks for linked list TADs -------------------------------- */

static void observer_on_compare(int first_index, int second_index,
                                int32_t first_value, int32_t second_value,
                                void *context) {
  (void)context;
  metrics.comparisons++;
  record_operation(OPERATION_COMPARE, first_index, second_index, first_value,
                   second_value);
}

static void observer_on_swap(int first_index, int second_index,
                             int32_t new_first_value, int32_t new_second_value,
                             void *context) {
  (void)context;
  metrics.swaps++;
  record_operation(OPERATION_SWAP, first_index, second_index, new_first_value,
                   new_second_value);
}

static void observer_on_write(int index, int32_t value, int count_as_swap,
                              void *context) {
  (void)context;
  if (count_as_swap) {
    metrics.swaps++;
  }
  record_operation(OPERATION_WRITE, index, -1, value, count_as_swap);
}

/* Sequential List (Dynamic Contiguous Array) sorting --------------------- */

static inline int32_t seq_val_at(const SequentialList *list, int index) {
  int32_t value = 0;
  seq_list_get(list, index, &value);
  return value;
}

static int seq_value_at_is_less(const SequentialList *list, int first_index,
                                int second_index) {
  int32_t first_val = seq_val_at(list, first_index);
  int32_t second_val = seq_val_at(list, second_index);
  observer_on_compare(first_index, second_index, first_val, second_val, NULL);
  return first_val < second_val;
}

static void seq_swap_values_at(SequentialList *list, int first_index,
                               int second_index) {
  if (first_index == second_index) {
    return;
  }

  int32_t first_val = seq_val_at(list, first_index);
  int32_t second_val = seq_val_at(list, second_index);

  seq_list_set(list, first_index, &second_val);
  seq_list_set(list, second_index, &first_val);

  observer_on_swap(first_index, second_index, second_val, first_val, NULL);
}

static void seq_write_value_at(SequentialList *list, int index, int32_t value,
                               int count_as_swap) {
  seq_list_set(list, index, &value);
  observer_on_write(index, value, count_as_swap, NULL);
}

static void seq_bubble_sort(SequentialList *list) {
  int made_swap;
  int n = seq_list_size(list);

  do {
    made_swap = 0;
    for (int index = 0; index < n - 1; index++) {
      if (seq_value_at_is_less(list, index + 1, index)) {
        seq_swap_values_at(list, index, index + 1);
        made_swap = 1;
      }
    }
  } while (made_swap);
}

static void seq_selection_sort(SequentialList *list) {
  int n = seq_list_size(list);
  for (int position = 0; position < n - 1; position++) {
    int smallest_index = position;
    for (int candidate = position + 1; candidate < n; candidate++) {
      if (seq_value_at_is_less(list, candidate, smallest_index)) {
        smallest_index = candidate;
      }
    }
    seq_swap_values_at(list, position, smallest_index);
  }
}

static void seq_insertion_sort(SequentialList *list) {
  int n = seq_list_size(list);
  for (int position = 1; position < n; position++) {
    int32_t value_to_insert = seq_val_at(list, position);
    int previous_position = position - 1;

    while (previous_position >= 0) {
      int32_t prev_val = seq_val_at(list, previous_position);
      observer_on_compare(previous_position, position, prev_val,
                          value_to_insert, NULL);

      if (prev_val <= value_to_insert) {
        break;
      }

      seq_write_value_at(list, previous_position + 1, prev_val, 1);
      previous_position--;
    }

    if (previous_position + 1 != position) {
      seq_write_value_at(list, previous_position + 1, value_to_insert, 0);
    }
  }
}

static int seq_partition_around_last_value(SequentialList *list, int low,
                                           int high) {
  int32_t pivot_value = seq_val_at(list, high);
  int last_smaller_index = low - 1;

  for (int scan_index = low; scan_index < high; scan_index++) {
    int32_t scan_val = seq_val_at(list, scan_index);
    observer_on_compare(scan_index, high, scan_val, pivot_value, NULL);

    if (scan_val < pivot_value) {
      last_smaller_index++;
      seq_swap_values_at(list, last_smaller_index, scan_index);
    }
  }

  int pivot_index = last_smaller_index + 1;
  seq_swap_values_at(list, pivot_index, high);
  return pivot_index;
}

static void seq_quick_sort_range(SequentialList *list, int low, int high) {
  if (low >= high) {
    return;
  }

  int pivot_index = seq_partition_around_last_value(list, low, high);
  seq_quick_sort_range(list, low, pivot_index - 1);
  seq_quick_sort_range(list, pivot_index + 1, high);
}

static void seq_merge_sorted_ranges(SequentialList *list, int32_t *merge_buffer,
                                    int low, int middle, int high) {
  int left_index = low;
  int right_index = middle + 1;
  int output_index = low;

  while (left_index <= middle && right_index <= high) {
    int32_t left_val = seq_val_at(list, left_index);
    int32_t right_val = seq_val_at(list, right_index);
    observer_on_compare(left_index, right_index, left_val, right_val, NULL);

    if (left_val <= right_val) {
      merge_buffer[output_index++] = left_val;
      left_index++;
    } else {
      merge_buffer[output_index++] = right_val;
      right_index++;
    }
  }

  while (left_index <= middle) {
    merge_buffer[output_index++] = seq_val_at(list, left_index++);
  }

  while (right_index <= high) {
    merge_buffer[output_index++] = seq_val_at(list, right_index++);
  }

  for (int index = low; index <= high; index++) {
    seq_write_value_at(list, index, merge_buffer[index], 1);
  }
}

static void seq_merge_sort_range(SequentialList *list, int32_t *merge_buffer,
                                 int low, int high) {
  if (low >= high) {
    return;
  }

  int middle = low + (high - low) / 2;
  seq_merge_sort_range(list, merge_buffer, low, middle);
  seq_merge_sort_range(list, merge_buffer, middle + 1, high);
  seq_merge_sorted_ranges(list, merge_buffer, low, middle, high);
}

static int run_sequential_list_sort(const int32_t *input, int item_count,
                                    int algorithm) {
  /* Use exact capacity item_count so dynamic contiguous array memory is clear
   */
  SequentialList *list =
      seq_list_create_with_capacity(sizeof(int32_t), item_count);
  if (list == NULL) {
    return 0;
  }

  for (int i = 0; i < item_count; i++) {
    if (!seq_list_insert_end(list, &input[i])) {
      seq_list_destroy(list);
      return 0;
    }
  }

  size_t aux_memory_bytes = 0;
  int32_t *merge_buffer = NULL;
  if (algorithm == ALGORITHM_MERGE) {
    merge_buffer = (int32_t *)malloc((size_t)item_count * sizeof(int32_t));
    if (merge_buffer == NULL) {
      seq_list_destroy(list);
      return 0;
    }
    aux_memory_bytes = (size_t)item_count * sizeof(int32_t);
  }

  double start_ms = current_time_ms();

  switch (algorithm) {
  case ALGORITHM_BUBBLE:
    seq_bubble_sort(list);
    break;
  case ALGORITHM_SELECTION:
    seq_selection_sort(list);
    break;
  case ALGORITHM_INSERTION:
    seq_insertion_sort(list);
    break;
  case ALGORITHM_QUICK:
    seq_quick_sort_range(list, 0, item_count - 1);
    break;
  case ALGORITHM_MERGE:
    seq_merge_sort_range(list, merge_buffer, 0, item_count - 1);
    break;
  }

  metrics.elapsed_ms = current_time_ms() - start_ms;
  metrics.memory_bytes = seq_list_memory_usage(list) + aux_memory_bytes;

  const int32_t *raw_data = (const int32_t *)seq_list_data(list);
  memcpy(sorted_values_buffer, raw_data, (size_t)item_count * sizeof(int32_t));

  free(merge_buffer);
  seq_list_destroy(list);
  return 1;
}

static int run_singly_list_sort(const int32_t *input, int item_count,
                                int algorithm,
                                const ListSortObserver *observer) {
  SinglyLinkedList *list = singly_list_create();
  if (list == NULL) {
    return 0;
  }
  if (!singly_list_from_array(list, input, item_count)) {
    singly_list_destroy(list);
    return 0;
  }

  double start_ms = current_time_ms();
  singly_list_sort(list, algorithm, observer);
  metrics.elapsed_ms = current_time_ms() - start_ms;
  metrics.memory_bytes = singly_list_memory_usage(list);

  singly_list_to_array(list, sorted_values_buffer, item_count);
  singly_list_destroy(list);
  return 1;
}

static int run_doubly_list_sort(const int32_t *input, int item_count,
                                int algorithm,
                                const ListSortObserver *observer) {
  DoublyLinkedList *list = doubly_list_create();
  if (list == NULL) {
    return 0;
  }
  if (!doubly_list_from_array(list, input, item_count)) {
    doubly_list_destroy(list);
    return 0;
  }

  double start_ms = current_time_ms();
  doubly_list_sort(list, algorithm, observer);
  metrics.elapsed_ms = current_time_ms() - start_ms;
  metrics.memory_bytes = doubly_list_memory_usage(list);

  doubly_list_to_array(list, sorted_values_buffer, item_count);
  doubly_list_destroy(list);
  return 1;
}

static int run_circular_list_sort(const int32_t *input, int item_count,
                                  int algorithm,
                                  const ListSortObserver *observer) {
  CircularLinkedList *list = circular_list_create();
  if (list == NULL) {
    return 0;
  }
  if (!circular_list_from_array(list, input, item_count)) {
    circular_list_destroy(list);
    return 0;
  }

  double start_ms = current_time_ms();
  circular_list_sort(list, algorithm, observer);
  metrics.elapsed_ms = current_time_ms() - start_ms;
  metrics.memory_bytes = circular_list_memory_usage(list);

  circular_list_to_array(list, sorted_values_buffer, item_count);
  circular_list_destroy(list);
  return 1;
}

/* Public sorting API ---------------------------------------------------- */

int sort_run(const int32_t *input, int item_count, int algorithm,
             int structure_type, int requested_operation_limit) {
  if (input == NULL || item_count < 1 || item_count > MAX_ITEMS ||
      algorithm < ALGORITHM_BUBBLE || algorithm > ALGORITHM_MERGE ||
      structure_type < STRUCT_SEQUENTIAL || structure_type > STRUCT_CIRCULAR) {
    return 0;
  }

  sort_free();

  sorted_values_buffer =
      (int32_t *)malloc((size_t)item_count * sizeof(int32_t));
  if (sorted_values_buffer == NULL) {
    return 0;
  }

  if (!reset_operation_history(requested_operation_limit)) {
    free(sorted_values_buffer);
    sorted_values_buffer = NULL;
    return 0;
  }

  memset(&metrics, 0, sizeof(metrics));
  metrics.algorithm = algorithm;
  metrics.structure_type = structure_type;
  metrics.item_count = item_count;

  ListSortObserver observer = {.on_compare = observer_on_compare,
                               .on_swap = observer_on_swap,
                               .on_write = observer_on_write,
                               .context = NULL};

  int ok = 0;
  switch (structure_type) {
  case STRUCT_SEQUENTIAL:
    ok = run_sequential_list_sort(input, item_count, algorithm);
    break;
  case STRUCT_SINGLY:
    ok = run_singly_list_sort(input, item_count, algorithm, &observer);
    break;
  case STRUCT_DOUBLY:
    ok = run_doubly_list_sort(input, item_count, algorithm, &observer);
    break;
  case STRUCT_CIRCULAR:
    ok = run_circular_list_sort(input, item_count, algorithm, &observer);
    break;
  }

  if (!ok) {
    sort_free();
    return 0;
  }

  metrics.operation_count = seq_list_size(operation_history);
  return 1;
}

const int32_t *sort_values(void) { return sorted_values_buffer; }

const Metrics *sort_metrics(void) { return &metrics; }

const Operation *sort_operations(void) {
  if (operation_history == NULL) {
    return NULL;
  }
  return (const Operation *)seq_list_data(operation_history);
}

void sort_free(void) {
  free(sorted_values_buffer);
  sorted_values_buffer = NULL;

  if (operation_history != NULL) {
    seq_list_destroy(operation_history);
    operation_history = NULL;
  }
}
