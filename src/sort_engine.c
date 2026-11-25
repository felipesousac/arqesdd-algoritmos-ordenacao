#include "sort_engine.h"
#include "tad/headerFiles/circular_linked_list.h"
#include "tad/headerFiles/doubly_linked_list.h"
#include "tad/headerFiles/sequential_list.h"
#include "tad/headerFiles/sequential_list_sort.h"
#include "tad/headerFiles/singly_linked_list.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ITEMS 4096
#define MAX_OPERATIONS 2000000

// Runtime variables being used by each sort endpoint
static int32_t *sorted_values_buffer = NULL;
static SequentialList *operation_history = NULL;
static int operation_limit = MAX_OPERATIONS;
static Metrics metrics;

static double current_time_ms(void) {
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    // Transform seconds and nano in ms them sum the result
    return (double) ts.tv_sec * 1000.0 + (double) ts.tv_nsec / 1000000.0;
  }

  return (double) clock() * 1000.0 / CLOCKS_PER_SEC;
}

const char *get_sort_algorithm_name(int algorithm) {
  static const char *names[] = {"Bubble Sort", "Selection Sort", "Insertion Sort", "Quick Sort", "Merge Sort"};

  return (algorithm >= ALGORITHM_BUBBLE && algorithm <= ALGORITHM_MERGE)
             ? names[algorithm]
             : "Unknown Algorithm";
}

const char *get_sort_structure_name(int structure_type) {
  static const char *names[] = {"Sequential List (Dynamic Array)", "Singly Linked List", "Doubly Linked List", "Circular Linked List"};
  
  return (structure_type >= STRUCT_SEQUENTIAL &&
          structure_type <= STRUCT_CIRCULAR)
             ? names[structure_type]
             : "Unknown Structure";
}

/* Operation history via SequentialList TAD ------------------------------- */
static void record_operation(int type, int first_index, int second_index, int32_t first_value, int32_t second_value) {
  if (operation_history == NULL || seq_list_size(operation_history) >= operation_limit) {
    return;
  }

  Operation operation = {type, first_index, second_index, first_value, second_value};

  seq_list_insert_end(operation_history, &operation);
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
                                int32_t first_value, int32_t second_value) {
  metrics.comparisons++;

  record_operation(OPERATION_COMPARE, first_index, second_index, first_value, second_value);
}

static void observer_on_swap(int first_index, int second_index, int32_t new_first_value, int32_t new_second_value) {
  metrics.swaps++;

  record_operation(OPERATION_SWAP, first_index, second_index, new_first_value, new_second_value);
}

static void observer_on_write(int index, int32_t value, int count_as_swap) {
  if (count_as_swap) {
    metrics.swaps++;
  }

  record_operation(OPERATION_WRITE, index, -1, value, count_as_swap);
}

/* Sequential List (Dynamic Contiguous Array) sorting --------------------- */
static int run_sequential_list_sort(const int32_t *input, int item_count,
                                    int algorithm,
                                    const ListSortObserver *observer) {
  SequentialList *list = seq_list_create_with_capacity(sizeof(int32_t), item_count);
  
  if (list == NULL) {
    return 0;
  }

  if (!seq_list_from_array(list, input, item_count)) {
    seq_list_destroy(list);

    return 0;
  }

  size_t aux_memory_bytes = 0;

  double start_ms = current_time_ms();

  int ok = seq_list_sort(list, algorithm, observer, &aux_memory_bytes);

  metrics.elapsed_ms = current_time_ms() - start_ms;

  if (!ok) {
    seq_list_destroy(list);

    return 0;
  }

  metrics.memory_bytes = seq_list_memory_usage(list) + aux_memory_bytes;

  const int32_t *raw_data = (const int32_t *) seq_list_data(list);

  memcpy(sorted_values_buffer, raw_data, (size_t)item_count * sizeof(int32_t));

  seq_list_destroy(list);
  
  return 1;
}

static int run_singly_list_sort(const int32_t *input, int item_count, int algorithm, const ListSortObserver *observer) {
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

static int run_doubly_list_sort(const int32_t *input, int item_count, int algorithm, const ListSortObserver *observer) {
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

static int run_circular_list_sort(const int32_t *input, int item_count, int algorithm, const ListSortObserver *observer) {
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
int sort_run(const int32_t *input, int item_count, int algorithm, int structure_type, int requested_operation_limit) {
  if (input == NULL || item_count < 1 || item_count > MAX_ITEMS ||
      algorithm < ALGORITHM_BUBBLE || algorithm > ALGORITHM_MERGE ||
      structure_type < STRUCT_SEQUENTIAL || structure_type > STRUCT_CIRCULAR) {
    return 0;
  }

  sort_free();

  sorted_values_buffer = (int32_t *) malloc((size_t) item_count * sizeof(int32_t));
  
  if (sorted_values_buffer == NULL) {
    return 0;
  }

  if (!reset_operation_history(requested_operation_limit)) {
    free(sorted_values_buffer);
    sorted_values_buffer = NULL;

    return 0;
  }

  memset(&metrics, 0, sizeof(metrics)); // Reset each metrics field to 0
  metrics.algorithm = algorithm;
  metrics.structure_type = structure_type;
  metrics.item_count = item_count;

  ListSortObserver observer = {.on_compare = observer_on_compare,
                               .on_swap = observer_on_swap,
                               .on_write = observer_on_write};

  int ok = 0;

  switch (structure_type) {
    case STRUCT_SEQUENTIAL:
      ok = run_sequential_list_sort(input, item_count, algorithm, &observer);
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
