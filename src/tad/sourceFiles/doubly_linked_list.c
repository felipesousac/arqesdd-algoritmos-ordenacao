#include "../headerFiles/doubly_linked_list.h"
#include <stdlib.h>

typedef struct doubly_node {
  int32_t data;
  struct doubly_node *prev;
  struct doubly_node *next;
} DoublyNode;

struct doubly_linked_list {
  DoublyNode *head;
  DoublyNode *tail;
  int size;
  size_t memory_bytes;
};

DoublyLinkedList *doubly_list_create(void) {
  DoublyLinkedList *list =
      (DoublyLinkedList *)malloc(sizeof(DoublyLinkedList));
  if (list == NULL) {
    return NULL;
  }

  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  list->memory_bytes = sizeof(DoublyLinkedList);
  return list;
}

int doubly_list_insert_start(DoublyLinkedList *list, int32_t value) {
  if (list == NULL) {
    return 0;
  }

  DoublyNode *node = (DoublyNode *)malloc(sizeof(DoublyNode));
  if (node == NULL) {
    return 0;
  }

  node->data = value;
  node->prev = NULL;
  node->next = list->head;

  if (list->head != NULL) {
    list->head->prev = node;
  } else {
    list->tail = node;
  }
  list->head = node;

  list->size++;
  list->memory_bytes += sizeof(DoublyNode);
  return 1;
}

int doubly_list_insert_end(DoublyLinkedList *list, int32_t value) {
  if (list == NULL) {
    return 0;
  }

  DoublyNode *node = (DoublyNode *)malloc(sizeof(DoublyNode));
  if (node == NULL) {
    return 0;
  }

  node->data = value;
  node->next = NULL;
  node->prev = list->tail;

  if (list->tail != NULL) {
    list->tail->next = node;
  } else {
    list->head = node;
  }
  list->tail = node;

  list->size++;
  list->memory_bytes += sizeof(DoublyNode);
  return 1;
}

int doubly_list_remove_start(DoublyLinkedList *list) {
  if (list == NULL || list->head == NULL) {
    return 0;
  }

  DoublyNode *removed = list->head;
  list->head = removed->next;
  if (list->head != NULL) {
    list->head->prev = NULL;
  } else {
    list->tail = NULL;
  }

  free(removed);
  list->size--;
  list->memory_bytes -= sizeof(DoublyNode);
  return 1;
}

int doubly_list_remove_end(DoublyLinkedList *list) {
  if (list == NULL || list->tail == NULL) {
    return 0;
  }

  DoublyNode *removed = list->tail;
  list->tail = removed->prev;
  if (list->tail != NULL) {
    list->tail->next = NULL;
  } else {
    list->head = NULL;
  }

  free(removed);
  list->size--;
  list->memory_bytes -= sizeof(DoublyNode);
  return 1;
}

static DoublyNode *doubly_node_at(const DoublyLinkedList *list, int index) {
  if (list == NULL || index < 0 || index >= list->size) {
    return NULL;
  }

  /* Optimize traversal using bidirectional links */
  if (index <= list->size / 2) {
    DoublyNode *curr = list->head;
    for (int i = 0; i < index; i++) {
      curr = curr->next;
    }
    return curr;
  } else {
    DoublyNode *curr = list->tail;
    for (int i = list->size - 1; i > index; i--) {
      curr = curr->prev;
    }
    return curr;
  }
}

int doubly_list_remove_at(DoublyLinkedList *list, int index) {
  if (list == NULL || index < 0 || index >= list->size) {
    return 0;
  }
  if (index == 0) {
    return doubly_list_remove_start(list);
  }
  if (index == list->size - 1) {
    return doubly_list_remove_end(list);
  }

  DoublyNode *removed = doubly_node_at(list, index);
  if (removed == NULL) {
    return 0;
  }

  removed->prev->next = removed->next;
  removed->next->prev = removed->prev;

  free(removed);
  list->size--;
  list->memory_bytes -= sizeof(DoublyNode);
  return 1;
}

int doubly_list_get(const DoublyLinkedList *list, int index,
                    int32_t *out_value) {
  if (out_value == NULL) {
    return 0;
  }
  DoublyNode *node = doubly_node_at(list, index);
  if (node == NULL) {
    return 0;
  }

  *out_value = node->data;
  return 1;
}

int doubly_list_set(DoublyLinkedList *list, int index, int32_t value) {
  DoublyNode *node = doubly_node_at(list, index);
  if (node == NULL) {
    return 0;
  }

  node->data = value;
  return 1;
}

int doubly_list_from_array(DoublyLinkedList *list, const int32_t *values,
                           int count) {
  if (list == NULL || values == NULL || count < 0) {
    return 0;
  }

  doubly_list_clear(list);
  for (int i = 0; i < count; i++) {
    if (!doubly_list_insert_end(list, values[i])) {
      return 0;
    }
  }
  return 1;
}

int doubly_list_to_array(const DoublyLinkedList *list, int32_t *out_values,
                         int max_count) {
  if (list == NULL || out_values == NULL || max_count < list->size) {
    return 0;
  }

  DoublyNode *curr = list->head;
  int index = 0;
  while (curr != NULL && index < max_count) {
    out_values[index++] = curr->data;
    curr = curr->next;
  }
  return index;
}

int doubly_list_size(const DoublyLinkedList *list) {
  if (list == NULL) {
    return -1;
  }
  return list->size;
}

int doubly_list_is_empty(const DoublyLinkedList *list) {
  if (list == NULL) {
    return -1;
  }
  return list->size == 0;
}

size_t doubly_list_memory_usage(const DoublyLinkedList *list) {
  if (list == NULL) {
    return 0;
  }
  return list->memory_bytes;
}

void doubly_list_clear(DoublyLinkedList *list) {
  if (list == NULL) {
    return;
  }

  DoublyNode *curr = list->head;
  while (curr != NULL) {
    DoublyNode *next = curr->next;
    free(curr);
    curr = next;
  }

  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  list->memory_bytes = sizeof(DoublyLinkedList);
}

void doubly_list_destroy(DoublyLinkedList *list) {
  if (list != NULL) {
    doubly_list_clear(list);
    free(list);
  }
}

/* Internal helpers for pointer-based sorting ----------------------------- */

static inline void emit_compare(const ListSortObserver *obs, int i, int j,
                                int32_t vi, int32_t vj) {
  if (obs != NULL && obs->on_compare != NULL) {
    obs->on_compare(i, j, vi, vj, obs->context);
  }
}

static inline void emit_swap(const ListSortObserver *obs, int i, int j,
                             int32_t new_vi, int32_t new_vj) {
  if (obs != NULL && obs->on_swap != NULL) {
    obs->on_swap(i, j, new_vi, new_vj, obs->context);
  }
}

static inline void emit_write(const ListSortObserver *obs, int idx, int32_t val,
                              int count_as_swap) {
  if (obs != NULL && obs->on_write != NULL) {
    obs->on_write(idx, val, count_as_swap, obs->context);
  }
}

/* Swaps two nodes in the doubly linked list by relinking prev/next pointers */
static void doubly_swap_nodes_by_index(DoublyLinkedList *list, int first_index,
                                       int second_index,
                                       const ListSortObserver *obs) {
  if (first_index == second_index) {
    return;
  }

  int left_idx = first_index < second_index ? first_index : second_index;
  int right_idx = first_index < second_index ? second_index : first_index;

  DoublyNode *left_node = doubly_node_at(list, left_idx);
  DoublyNode *right_node = doubly_node_at(list, right_idx);
  if (left_node == NULL || right_node == NULL) {
    return;
  }

  int32_t first_val =
      (first_index == left_idx) ? left_node->data : right_node->data;
  int32_t second_val =
      (first_index == left_idx) ? right_node->data : left_node->data;

  if (left_node->next == right_node) {
    /* Adjacent nodes: left_node <-> right_node */
    DoublyNode *before = left_node->prev;
    DoublyNode *after = right_node->next;

    if (before != NULL) {
      before->next = right_node;
    } else {
      list->head = right_node;
    }
    right_node->prev = before;
    right_node->next = left_node;
    left_node->prev = right_node;
    left_node->next = after;
    if (after != NULL) {
      after->prev = left_node;
    } else {
      list->tail = left_node;
    }
  } else {
    DoublyNode *left_prev = left_node->prev;
    DoublyNode *left_next = left_node->next;
    DoublyNode *right_prev = right_node->prev;
    DoublyNode *right_next = right_node->next;

    if (left_prev != NULL) {
      left_prev->next = right_node;
    } else {
      list->head = right_node;
    }
    if (left_next != NULL) {
      left_next->prev = right_node;
    }
    right_node->prev = left_prev;
    right_node->next = left_next;

    if (right_prev != NULL) {
      right_prev->next = left_node;
    }
    if (right_next != NULL) {
      right_next->prev = left_node;
    } else {
      list->tail = left_node;
    }
    left_node->prev = right_prev;
    left_node->next = right_next;
  }

  emit_swap(obs, first_index, second_index, second_val, first_val);
}

static void doubly_bubble_sort(DoublyLinkedList *list,
                               const ListSortObserver *obs) {
  if (list->size < 2) {
    return;
  }

  int made_swap;
  do {
    made_swap = 0;
    DoublyNode *curr = list->head;

    for (int index = 0;
         index < list->size - 1 && curr != NULL && curr->next != NULL;
         index++) {
      DoublyNode *next_node = curr->next;
      emit_compare(obs, index + 1, index, next_node->data, curr->data);

      if (next_node->data < curr->data) {
        int32_t first_val = curr->data;
        int32_t second_val = next_node->data;

        DoublyNode *before = curr->prev;
        DoublyNode *after = next_node->next;

        if (before != NULL) {
          before->next = next_node;
        } else {
          list->head = next_node;
        }
        next_node->prev = before;
        next_node->next = curr;
        curr->prev = next_node;
        curr->next = after;
        if (after != NULL) {
          after->prev = curr;
        } else {
          list->tail = curr;
        }

        emit_swap(obs, index, index + 1, second_val, first_val);
        made_swap = 1;
      } else {
        curr = curr->next;
      }
    }
  } while (made_swap);
}

static void doubly_selection_sort(DoublyLinkedList *list,
                                  const ListSortObserver *obs) {
  for (int position = 0; position < list->size - 1; position++) {
    DoublyNode *pos_node = doubly_node_at(list, position);
    int smallest_index = position;
    DoublyNode *smallest_node = pos_node;
    DoublyNode *candidate_node = pos_node->next;

    for (int candidate = position + 1;
         candidate < list->size && candidate_node != NULL; candidate++) {
      emit_compare(obs, candidate, smallest_index, candidate_node->data,
                   smallest_node->data);
      if (candidate_node->data < smallest_node->data) {
        smallest_index = candidate;
        smallest_node = candidate_node;
      }
      candidate_node = candidate_node->next;
    }

    if (smallest_index != position) {
      doubly_swap_nodes_by_index(list, position, smallest_index, obs);
    }
  }
}

static void doubly_insertion_sort(DoublyLinkedList *list,
                                  const ListSortObserver *obs) {
  if (list->size < 2) {
    return;
  }

  DoublyNode *last_sorted = list->head;
  for (int position = 1; position < list->size; position++) {
    DoublyNode *curr = last_sorted->next;
    int32_t value_to_insert = curr->data;

    /* Traverse backwards directly using doubly-linked prev pointers */
    DoublyNode *scan = last_sorted;
    int previous_position = position - 1;

    while (previous_position >= 0 && scan != NULL) {
      emit_compare(obs, previous_position, position, scan->data,
                   value_to_insert);
      if (scan->data <= value_to_insert) {
        break;
      }

      emit_write(obs, previous_position + 1, scan->data, 1);
      scan = scan->prev;
      previous_position--;
    }

    int target_index = previous_position + 1;
    if (target_index != position) {
      /* Detach curr from its position */
      last_sorted->next = curr->next;
      if (curr->next != NULL) {
        curr->next->prev = last_sorted;
      } else {
        list->tail = last_sorted;
      }

      /* Splice curr after scan (or at head if scan == NULL) */
      if (scan == NULL) {
        curr->prev = NULL;
        curr->next = list->head;
        if (list->head != NULL) {
          list->head->prev = curr;
        }
        list->head = curr;
      } else {
        curr->next = scan->next;
        curr->prev = scan;
        if (scan->next != NULL) {
          scan->next->prev = curr;
        }
        scan->next = curr;
      }

      emit_write(obs, target_index, value_to_insert, 0);
    } else {
      last_sorted = curr;
    }
  }
}

static int doubly_partition(DoublyLinkedList *list, int low, int high,
                            const ListSortObserver *obs) {
  DoublyNode *high_node = doubly_node_at(list, high);
  int32_t pivot_value = high_node->data;
  int last_smaller_index = low - 1;

  for (int scan_index = low; scan_index < high; scan_index++) {
    DoublyNode *scan_node = doubly_node_at(list, scan_index);
    emit_compare(obs, scan_index, high, scan_node->data, pivot_value);

    if (scan_node->data < pivot_value) {
      last_smaller_index++;
      doubly_swap_nodes_by_index(list, last_smaller_index, scan_index, obs);
    }
  }

  int pivot_index = last_smaller_index + 1;
  doubly_swap_nodes_by_index(list, pivot_index, high, obs);
  return pivot_index;
}

static void doubly_quick_sort_range(DoublyLinkedList *list, int low, int high,
                                    const ListSortObserver *obs) {
  if (low >= high) {
    return;
  }

  int pivot_index = doubly_partition(list, low, high, obs);
  doubly_quick_sort_range(list, low, pivot_index - 1, obs);
  doubly_quick_sort_range(list, pivot_index + 1, high, obs);
}

static DoublyNode *doubly_merge_sort_nodes(DoublyNode *head, int low, int high,
                                           const ListSortObserver *obs) {
  if (head == NULL || head->next == NULL || low >= high) {
    return head;
  }

  int middle = low + (high - low) / 2;
  int left_len = middle - low + 1;

  DoublyNode *mid_node = head;
  for (int i = 1; i < left_len && mid_node->next != NULL; i++) {
    mid_node = mid_node->next;
  }

  DoublyNode *right_head = mid_node->next;
  mid_node->next = NULL;
  if (right_head != NULL) {
    right_head->prev = NULL;
  }

  DoublyNode *left_sorted =
      doubly_merge_sort_nodes(head, low, middle, obs);
  DoublyNode *right_sorted =
      doubly_merge_sort_nodes(right_head, middle + 1, high, obs);

  DoublyNode dummy;
  dummy.next = NULL;
  dummy.prev = NULL;
  DoublyNode *tail = &dummy;

  int left_index = low;
  int right_index = middle + 1;
  DoublyNode *left_curr = left_sorted;
  DoublyNode *right_curr = right_sorted;

  while (left_curr != NULL && right_curr != NULL) {
    emit_compare(obs, left_index, right_index, left_curr->data,
                 right_curr->data);
    if (left_curr->data <= right_curr->data) {
      tail->next = left_curr;
      left_curr->prev = tail;
      left_curr = left_curr->next;
      left_index++;
    } else {
      tail->next = right_curr;
      right_curr->prev = tail;
      right_curr = right_curr->next;
      right_index++;
    }
    tail = tail->next;
  }

  DoublyNode *remainder = (left_curr != NULL) ? left_curr : right_curr;
  tail->next = remainder;
  if (remainder != NULL) {
    remainder->prev = tail;
  }

  if (dummy.next != NULL) {
    dummy.next->prev = NULL;
  }

  DoublyNode *scan = dummy.next;
  for (int idx = low; idx <= high && scan != NULL; idx++) {
    emit_write(obs, idx, scan->data, 1);
    scan = scan->next;
  }

  return dummy.next;
}

static void doubly_merge_sort(DoublyLinkedList *list,
                              const ListSortObserver *obs) {
  if (list->size < 2) {
    return;
  }

  list->head = doubly_merge_sort_nodes(list->head, 0, list->size - 1, obs);
  DoublyNode *curr = list->head;
  while (curr != NULL && curr->next != NULL) {
    curr = curr->next;
  }
  list->tail = curr;
}

int doubly_list_sort(DoublyLinkedList *list, int algorithm,
                     const ListSortObserver *observer) {
  if (list == NULL || list->size < 1) {
    return 0;
  }

  switch (algorithm) {
  case 0:
    doubly_bubble_sort(list, observer);
    break;
  case 1:
    doubly_selection_sort(list, observer);
    break;
  case 2:
    doubly_insertion_sort(list, observer);
    break;
  case 3:
    doubly_quick_sort_range(list, 0, list->size - 1, observer);
    break;
  case 4:
    doubly_merge_sort(list, observer);
    break;
  default:
    return 0;
  }

  return 1;
}
