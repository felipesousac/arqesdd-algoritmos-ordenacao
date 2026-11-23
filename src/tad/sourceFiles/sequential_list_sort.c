#include "../headerFiles/sequential_list_sort.h"
#include <stdint.h>
#include <stdlib.h>

static inline void emit_compare(const ListSortObserver *obs, int first_index,
                                int second_index, int32_t first_value,
                                int32_t second_value) {
  if (obs != NULL && obs->on_compare != NULL) {
    obs->on_compare(first_index, second_index, first_value, second_value, obs->context);
  }
}

static inline void emit_swap(const ListSortObserver *obs, int first_index,
                             int second_index, int32_t new_first_value,
                             int32_t new_second_value) {
  if (obs != NULL && obs->on_swap != NULL) {
    obs->on_swap(first_index, second_index, new_first_value, new_second_value, obs->context);
  }
}

static inline void emit_write(const ListSortObserver *obs, int index,
                              int32_t value, int count_as_swap) {
  if (obs != NULL && obs->on_write != NULL) {
    obs->on_write(index, value, count_as_swap, obs->context);
  }
}

static inline int32_t seq_val_at(const SequentialList *list, int index) {
  int32_t value = 0;
  seq_list_get(list, index, &value);

  return value;
}

static int seq_value_at_is_less(const SequentialList *list, int first_index,
                                int second_index, const ListSortObserver *obs) {
  int32_t first_val = seq_val_at(list, first_index);
  int32_t second_val = seq_val_at(list, second_index);
  emit_compare(obs, first_index, second_index, first_val, second_val);

  return first_val < second_val;
}

static void seq_swap_values_at(SequentialList *list, int first_index, int second_index, const ListSortObserver *obs) {
  if (first_index == second_index) {
    return;
  }

  int32_t first_val = seq_val_at(list, first_index);
  int32_t second_val = seq_val_at(list, second_index);

  seq_list_set(list, first_index, &second_val);
  seq_list_set(list, second_index, &first_val);

  emit_swap(obs, first_index, second_index, second_val, first_val);
}

static void seq_write_value_at(SequentialList *list, int index, int32_t value, int count_as_swap, const ListSortObserver *obs) {
  seq_list_set(list, index, &value);
  emit_write(obs, index, value, count_as_swap);
}

static void seq_bubble_sort(SequentialList *list, const ListSortObserver *obs) {
  int made_swap;
  int n = seq_list_size(list);

  do {
    made_swap = 0;

    for (int index = 0; index < n - 1; index++) {
      if (seq_value_at_is_less(list, index + 1, index, obs)) {
        seq_swap_values_at(list, index, index + 1, obs);

        made_swap = 1;
      }
    }
  } while (made_swap);
}

static void seq_selection_sort(SequentialList *list, const ListSortObserver *obs) {
  int n = seq_list_size(list);

  for (int position = 0; position < n - 1; position++) {
    int smallest_index = position;

    for (int candidate = position + 1; candidate < n; candidate++) {
      if (seq_value_at_is_less(list, candidate, smallest_index, obs)) {
        smallest_index = candidate;
      }
    }

    seq_swap_values_at(list, position, smallest_index, obs);
  }
}

static void seq_insertion_sort(SequentialList *list, const ListSortObserver *obs) {
  int n = seq_list_size(list);

  for (int position = 1; position < n; position++) {
    int32_t value_to_insert = seq_val_at(list, position);

    int previous_position = position - 1;

    while (previous_position >= 0) {
      int32_t prev_val = seq_val_at(list, previous_position);

      emit_compare(obs, previous_position, position, prev_val, value_to_insert);

      if (prev_val <= value_to_insert) {
        break;
      }

      seq_write_value_at(list, previous_position + 1, prev_val, 1, obs);
      previous_position--;
    }

    if (previous_position + 1 != position) {
      seq_write_value_at(list, previous_position + 1, value_to_insert, 0, obs);
    }
  }
}

static int seq_partition_around_last_value(SequentialList *list, int low, int high, const ListSortObserver *obs) {
  int32_t pivot_value = seq_val_at(list, high);
  int last_smaller_index = low - 1;

  for (int scan_index = low; scan_index < high; scan_index++) {
    int32_t scan_val = seq_val_at(list, scan_index);
    emit_compare(obs, scan_index, high, scan_val, pivot_value);

    if (scan_val < pivot_value) {
      last_smaller_index++;
      seq_swap_values_at(list, last_smaller_index, scan_index, obs);
    }
  }

  int pivot_index = last_smaller_index + 1;
  seq_swap_values_at(list, pivot_index, high, obs);

  return pivot_index;
}

static void seq_quick_sort_range(SequentialList *list, int low, int high, const ListSortObserver *obs) {
  if (low >= high) {
    return;
  }

  int pivot_index = seq_partition_around_last_value(list, low, high, obs);
  seq_quick_sort_range(list, low, pivot_index - 1, obs);
  seq_quick_sort_range(list, pivot_index + 1, high, obs);
}

static void seq_merge_sorted_ranges(SequentialList *list, int32_t *merge_buffer, int low, int middle, int high, const ListSortObserver *obs) {
  int left_index = low;
  int right_index = middle + 1;
  int output_index = low;

  while (left_index <= middle && right_index <= high) {
    int32_t left_val = seq_val_at(list, left_index);
    int32_t right_val = seq_val_at(list, right_index);
    emit_compare(obs, left_index, right_index, left_val, right_val);

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
    seq_write_value_at(list, index, merge_buffer[index], 1, obs);
  }
}

static void seq_merge_sort_range(SequentialList *list, int32_t *merge_buffer,int low, int high, const ListSortObserver *obs) {
  if (low >= high) {
    return;
  }

  int middle = low + (high - low) / 2;
  seq_merge_sort_range(list, merge_buffer, low, middle, obs);
  seq_merge_sort_range(list, merge_buffer, middle + 1, high, obs);
  seq_merge_sorted_ranges(list, merge_buffer, low, middle, high, obs);
}

int seq_list_sort(SequentialList *list, int algorithm, const ListSortObserver *observer, size_t *out_aux_memory_bytes) {
  if (out_aux_memory_bytes != NULL) {
    *out_aux_memory_bytes = 0;
  }

  if (list == NULL || seq_list_size(list) < 1 || seq_list_element_size(list) != sizeof(int32_t)) {
    return 0;
  }

  int item_count = seq_list_size(list);

  switch (algorithm) {
    case 0:
      seq_bubble_sort(list, observer);
      break;
    case 1:
      seq_selection_sort(list, observer);
      break;
    case 2:
      seq_insertion_sort(list, observer);
      break;
    case 3:
      seq_quick_sort_range(list, 0, item_count - 1, observer);
      break;
    case 4: {
      size_t aux_bytes = (size_t) item_count * sizeof(int32_t);
      int32_t *merge_buffer = (int32_t *) malloc(aux_bytes);

      if (merge_buffer == NULL) {
        return 0;
      }

      if (out_aux_memory_bytes != NULL) {
        *out_aux_memory_bytes = aux_bytes;
      }

      seq_merge_sort_range(list, merge_buffer, 0, item_count - 1, observer);
      free(merge_buffer);

      break;
    }
    default:
      return 0;
  }

  return 1;
}
