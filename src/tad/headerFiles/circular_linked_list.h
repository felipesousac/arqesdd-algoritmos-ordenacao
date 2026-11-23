#ifndef CIRCULAR_LINKED_LIST_H
#define CIRCULAR_LINKED_LIST_H

#include "list_sort_observer.h"
#include <stddef.h>
#include <stdint.h>

/* Opaque type for Dynamic Circular Linked List */
typedef struct circular_linked_list CircularLinkedList;

CircularLinkedList *circular_list_create(void);
int circular_list_insert_start(CircularLinkedList *list, int32_t value);
int circular_list_insert_end(CircularLinkedList *list, int32_t value);
int circular_list_remove_start(CircularLinkedList *list);
int circular_list_remove_end(CircularLinkedList *list);
int circular_list_remove_at(CircularLinkedList *list, int index);
int circular_list_get(const CircularLinkedList *list, int index,
                      int32_t *out_value);
int circular_list_set(CircularLinkedList *list, int index, int32_t value);
int circular_list_from_array(CircularLinkedList *list, const int32_t *values,
                             int count);
int circular_list_to_array(const CircularLinkedList *list, int32_t *out_values,
                           int max_count);
int circular_list_size(const CircularLinkedList *list);
int circular_list_is_empty(const CircularLinkedList *list);
size_t circular_list_memory_usage(const CircularLinkedList *list);
void circular_list_clear(CircularLinkedList *list);
void circular_list_destroy(CircularLinkedList *list);

/* Pointer-manipulating sorting algorithms on CircularLinkedList */
int circular_list_sort(CircularLinkedList *list, int algorithm,
                       const ListSortObserver *observer);

#endif
