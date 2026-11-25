#include "../headerFiles/singly_linked_list.h"
#include <stdlib.h>

typedef struct singly_node {
  int32_t data;
  struct singly_node *next;
} SinglyNode;

struct singly_linked_list {
  SinglyNode *head;
  SinglyNode *tail;
  int size;
  size_t memory_bytes;
};

SinglyLinkedList *singly_list_create(void) {
  SinglyLinkedList *list = (SinglyLinkedList *) malloc(sizeof(SinglyLinkedList));
  
  if (list == NULL) {
    return NULL;
  }

  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  list->memory_bytes = sizeof(SinglyLinkedList);

  return list;
}

int singly_list_insert_start(SinglyLinkedList *list, int32_t value) {
  if (list == NULL) {
    return 0;
  }

  SinglyNode *node = (SinglyNode *)malloc(sizeof(SinglyNode));
  if (node == NULL) {
    return 0;
  }

  node->data = value;
  node->next = list->head;
  list->head = node;
  if (list->tail == NULL) {
    list->tail = node;
  }

  list->size++;
  list->memory_bytes += sizeof(SinglyNode);
  return 1;
}

int singly_list_insert_end(SinglyLinkedList *list, int32_t value) {
  if (list == NULL) {
    return 0;
  }

  SinglyNode *node = (SinglyNode *) malloc(sizeof(SinglyNode));

  if (node == NULL) {
    return 0;
  }

  node->data = value;
  node->next = NULL;

  if (list->tail == NULL) {
    list->head = node;
    list->tail = node;
  } else {
    list->tail->next = node;
    list->tail = node;
  }

  list->size++;
  list->memory_bytes += sizeof(SinglyNode);

  return 1;
}

int singly_list_remove_start(SinglyLinkedList *list) {
  if (list == NULL || list->head == NULL) {
    return 0;
  }

  SinglyNode *removed = list->head;
  list->head = removed->next;
  if (list->head == NULL) {
    list->tail = NULL;
  }

  free(removed);
  list->size--;
  list->memory_bytes -= sizeof(SinglyNode);
  return 1;
}

int singly_list_remove_end(SinglyLinkedList *list) {
  if (list == NULL || list->head == NULL) {
    return 0;
  }

  if (list->head == list->tail) {
    return singly_list_remove_start(list);
  }

  SinglyNode *prev = list->head;
  while (prev->next != list->tail) {
    prev = prev->next;
  }

  free(list->tail);
  prev->next = NULL;
  list->tail = prev;
  list->size--;
  list->memory_bytes -= sizeof(SinglyNode);
  return 1;
}

int singly_list_remove_at(SinglyLinkedList *list, int index) {
  if (list == NULL || index < 0 || index >= list->size) {
    return 0;
  }
  if (index == 0) {
    return singly_list_remove_start(list);
  }

  SinglyNode *prev = list->head;
  for (int i = 0; i < index - 1; i++) {
    prev = prev->next;
  }

  SinglyNode *removed = prev->next;
  prev->next = removed->next;
  if (removed == list->tail) {
    list->tail = prev;
  }

  free(removed);
  list->size--;
  list->memory_bytes -= sizeof(SinglyNode);
  return 1;
}

int singly_list_get(const SinglyLinkedList *list, int index,
                    int32_t *out_value) {
  if (list == NULL || out_value == NULL || index < 0 || index >= list->size) {
    return 0;
  }

  SinglyNode *curr = list->head;
  for (int i = 0; i < index; i++) {
    curr = curr->next;
  }

  *out_value = curr->data;
  return 1;
}

int singly_list_set(SinglyLinkedList *list, int index, int32_t value) {
  if (list == NULL || index < 0 || index >= list->size) {
    return 0;
  }

  SinglyNode *curr = list->head;
  for (int i = 0; i < index; i++) {
    curr = curr->next;
  }

  curr->data = value;
  return 1;
}

int singly_list_from_array(SinglyLinkedList *list, const int32_t *values, int count) {
  if (list == NULL || values == NULL || count < 0) {
    return 0;
  }

  singly_list_clear(list);

  for (int i = 0; i < count; i++) {
    if (!singly_list_insert_end(list, values[i])) {
      return 0;
    }
  }

  return 1;
}

int singly_list_to_array(const SinglyLinkedList *list, int32_t *out_values,
                         int max_count) {
  if (list == NULL || out_values == NULL || max_count < list->size) {
    return 0;
  }

  SinglyNode *curr = list->head;
  int index = 0;
  while (curr != NULL && index < max_count) {
    out_values[index++] = curr->data;
    curr = curr->next;
  }
  return index;
}

int singly_list_size(const SinglyLinkedList *list) {
  if (list == NULL) {
    return -1;
  }
  return list->size;
}

int singly_list_is_empty(const SinglyLinkedList *list) {
  if (list == NULL) {
    return -1;
  }
  return list->size == 0;
}

size_t singly_list_memory_usage(const SinglyLinkedList *list) {
  if (list == NULL) {
    return 0;
  }
  return list->memory_bytes;
}

void singly_list_clear(SinglyLinkedList *list) {
  if (list == NULL) {
    return;
  }

  SinglyNode *curr = list->head;
  while (curr != NULL) {
    SinglyNode *next = curr->next;
    free(curr);
    curr = next;
  }

  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  list->memory_bytes = sizeof(SinglyLinkedList);
}

void singly_list_destroy(SinglyLinkedList *list) {
  if (list != NULL) {
    singly_list_clear(list);
    free(list);
  }
}

/* Internal helpers for pointer-based sorting ----------------------------- */

static inline void emit_compare(const ListSortObserver *obs, int i, int j,
                                int32_t vi, int32_t vj) {
  if (obs != NULL && obs->on_compare != NULL) {
    obs->on_compare(i, j, vi, vj);
  }
}

static inline void emit_swap(const ListSortObserver *obs, int i, int j,
                             int32_t new_vi, int32_t new_vj) {
  if (obs != NULL && obs->on_swap != NULL) {
    obs->on_swap(i, j, new_vi, new_vj);
  }
}

static inline void emit_write(const ListSortObserver *obs, int idx, int32_t val,
                              int count_as_swap) {
  if (obs != NULL && obs->on_write != NULL) {
    obs->on_write(idx, val, count_as_swap);
  }
}

/* Swaps two nodes in the singly linked list by relinking their next pointers */
static void singly_swap_nodes_by_index(SinglyLinkedList *list, int first_index, int second_index, const ListSortObserver *obs) {
  if (first_index == second_index) {
    return;
  }

  int left_idx = first_index < second_index ? first_index : second_index;
  int right_idx = first_index < second_index ? second_index : first_index;

  SinglyNode *prev_left = NULL;
  SinglyNode *left_node = list->head;

  for (int i = 0; i < left_idx; i++) {
    prev_left = left_node;
    left_node = left_node->next;
  }

  SinglyNode *prev_right = prev_left;
  SinglyNode *right_node = left_node;

  for (int i = left_idx; i < right_idx; i++) {
    prev_right = right_node;
    right_node = right_node->next;
  }

  int32_t first_val = (first_index == left_idx) ? left_node->data : right_node->data;
  int32_t second_val = (first_index == left_idx) ? right_node->data : left_node->data;

  if (prev_left != NULL) {
    prev_left->next = right_node;
  } else {
    list->head = right_node;
  }

  if (prev_right == left_node) {
    /* Adjacent nodes */
    left_node->next = right_node->next;
    right_node->next = left_node;
  } else {
    SinglyNode *temp_next = right_node->next;

    prev_right->next = left_node;
    right_node->next = left_node->next;
    left_node->next = temp_next;
  }

  if (left_node->next == NULL) {
    list->tail = left_node;
  } else if (right_node->next == NULL) {
    list->tail = right_node;
  }

  emit_swap(obs, first_index, second_index, second_val, first_val);
}

static void singly_bubble_sort(SinglyLinkedList *list, const ListSortObserver *obs) {
  int made_swap;

  do {
    made_swap = 0;

    SinglyNode *prev = NULL;
    SinglyNode *curr = list->head;

    for (int index = 0; index < list->size - 1 && curr != NULL && curr->next != NULL; index++) {
      SinglyNode *next_node = curr->next;

      emit_compare(obs, index + 1, index, next_node->data, curr->data);

      if (next_node->data < curr->data) {
        int32_t first_val = curr->data;
        int32_t second_val = next_node->data;

        /* Relink adjacent nodes */
        curr->next = next_node->next;
        next_node->next = curr;

        if (prev != NULL) {
          prev->next = next_node;
        } else {
          list->head = next_node;
        }

        if (curr->next == NULL) {
          list->tail = curr;
        }

        emit_swap(obs, index, index + 1, second_val, first_val);

        prev = next_node;
        made_swap = 1;
      } else {
        prev = curr;
        curr = curr->next;
      }
    }
  } while (made_swap);
}

static void singly_selection_sort(SinglyLinkedList *list, const ListSortObserver *obs) {
  for (int position = 0; position < list->size - 1; position++) {
    SinglyNode *pos_node = list->head;

    for (int i = 0; i < position; i++) {
      pos_node = pos_node->next;
    }

    int smallest_index = position;
    SinglyNode *smallest_node = pos_node;
    SinglyNode *candidate_node = pos_node->next;

    for (int candidate = position + 1; candidate < list->size && candidate_node != NULL; candidate++) {
      emit_compare(obs, candidate, smallest_index, candidate_node->data, smallest_node->data);
      
      if (candidate_node->data < smallest_node->data) {
        smallest_index = candidate;
        smallest_node = candidate_node;
      }

      candidate_node = candidate_node->next;
    }

    if (smallest_index != position) {
      singly_swap_nodes_by_index(list, position, smallest_index, obs);
    }
  }
}

static void singly_insertion_sort(SinglyLinkedList *list, const ListSortObserver *obs) {
  SinglyNode *last_sorted = list->head;

  for (int position = 1; position < list->size; position++) {
    SinglyNode *curr = last_sorted->next;
    int32_t value_to_insert = curr->data;

    /* Find preceding nodes in sorted prefix to emit exact comparisons */
    int previous_position = position - 1;

    while (previous_position >= 0) {
      SinglyNode *prev_node = list->head;

      for (int i = 0; i < previous_position; i++) {
        prev_node = prev_node->next;
      }

      emit_compare(obs, previous_position, position, prev_node->data, value_to_insert);
      
      if (prev_node->data <= value_to_insert) {
        break;
      }

      emit_write(obs, previous_position + 1, prev_node->data, 1);
      previous_position--;
    }

    int target_index = previous_position + 1;

    if (target_index != position) {
      /* Detach curr from after last_sorted and re-insert at target_index via
       * pointers */
      last_sorted->next = curr->next;

      if (curr == list->tail) {
        list->tail = last_sorted;
      }

      if (target_index == 0) {
        curr->next = list->head;
        list->head = curr;
      } else {
        SinglyNode *ins_prev = list->head;

        for (int i = 0; i < target_index - 1; i++) {
          ins_prev = ins_prev->next;
        }

        curr->next = ins_prev->next;
        ins_prev->next = curr;
      }

      emit_write(obs, target_index, value_to_insert, 0);
    } else {
      last_sorted = curr;
    }
  }
}

static int singly_partition(SinglyLinkedList *list, int low, int high, const ListSortObserver *obs) {
  SinglyNode *high_node = list->head;

  for (int i = 0; i < high; i++) {
    high_node = high_node->next;
  }

  int32_t pivot_value = high_node->data;
  int last_smaller_index = low - 1;

  for (int scan_index = low; scan_index < high; scan_index++) {
    SinglyNode *scan_node = list->head;

    for (int i = 0; i < scan_index; i++) {
      scan_node = scan_node->next;
    }

    emit_compare(obs, scan_index, high, scan_node->data, pivot_value);

    if (scan_node->data < pivot_value) {
      last_smaller_index++;
      singly_swap_nodes_by_index(list, last_smaller_index, scan_index, obs);
    }
  }

  int pivot_index = last_smaller_index + 1;
  singly_swap_nodes_by_index(list, pivot_index, high, obs);

  return pivot_index;
}

static void singly_quick_sort_range(SinglyLinkedList *list, int low, int high, const ListSortObserver *obs) {
  if (low >= high) {
    return;
  }

  int pivot_index = singly_partition(list, low, high, obs);
  singly_quick_sort_range(list, low, pivot_index - 1, obs);
  singly_quick_sort_range(list, pivot_index + 1, high, obs);
}

/* Merge Sort by pointer manipulation (slow/fast split + in-place node relink)
 */
static SinglyNode *singly_merge_sort_nodes(SinglyNode *head, int low, int high, const ListSortObserver *obs) {
  if (head == NULL || head->next == NULL || low >= high) {
    return head;
  }

  int middle = low + (high - low) / 2;
  int left_len = middle - low + 1;

  SinglyNode *mid_node = head;

  for (int i = 1; i < left_len && mid_node->next != NULL; i++) {
    mid_node = mid_node->next;
  }

  SinglyNode *right_head = mid_node->next;
  mid_node->next = NULL;

  SinglyNode *left_sorted = singly_merge_sort_nodes(head, low, middle, obs);
  SinglyNode *right_sorted = singly_merge_sort_nodes(right_head, middle + 1, high, obs);

  SinglyNode temp_head;
  temp_head.next = NULL;
  SinglyNode *tail = &temp_head;

  int left_index = low;
  int right_index = middle + 1;

  SinglyNode *left_curr = left_sorted;
  SinglyNode *right_curr = right_sorted;

  while (left_curr != NULL && right_curr != NULL) {
    emit_compare(obs, left_index, right_index, left_curr->data, right_curr->data);
    
    if (left_curr->data <= right_curr->data) {
      tail->next = left_curr;
      left_curr = left_curr->next;

      left_index++;
    } else {
      tail->next = right_curr;
      right_curr = right_curr->next;

      right_index++;
    }

    tail = tail->next;
  }

  if (left_curr != NULL) {
    tail->next = left_curr;
  } else {
    tail->next = right_curr;
  }

  SinglyNode *scan = temp_head.next;

  for (int idx = low; idx <= high && scan != NULL; idx++) {
    emit_write(obs, idx, scan->data, 1);
    scan = scan->next;
  }

  return temp_head.next;
}

static void singly_merge_sort(SinglyLinkedList *list, const ListSortObserver *obs) {
  list->head = singly_merge_sort_nodes(list->head, 0, list->size - 1, obs);

  SinglyNode *curr = list->head;

  while (curr != NULL && curr->next != NULL) {
    curr = curr->next;
  }
  
  list->tail = curr;
}

int singly_list_sort(SinglyLinkedList *list, int algorithm, const ListSortObserver *observer) {
  if (list == NULL || list->size < 1) {
    return 0;
  }

  if (list->size < 2) {
    return 1;
  }

  switch (algorithm) {
    case 0:
      singly_bubble_sort(list, observer);
      break;
    case 1:
      singly_selection_sort(list, observer);
      break;
    case 2:
      singly_insertion_sort(list, observer);
      break;
    case 3:
      singly_quick_sort_range(list, 0, list->size - 1, observer);
      break;
    case 4:
      singly_merge_sort(list, observer);
      break;
    default:
      return 0;
  }

  return 1;
}
