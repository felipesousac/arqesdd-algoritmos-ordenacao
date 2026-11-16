#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

#include <stddef.h>
#include <stdint.h>

#ifndef LIST_SORT_OBSERVER_DEFINED
#define LIST_SORT_OBSERVER_DEFINED
typedef struct {
  void (*on_compare)(int first_index, int second_index, int32_t first_value,
                     int32_t second_value, void *context);
  void (*on_swap)(int first_index, int second_index, int32_t new_first_value,
                  int32_t new_second_value, void *context);
  void (*on_write)(int index, int32_t value, int count_as_swap, void *context);
  void *context;
} ListSortObserver;
#endif

/* Opaque type for Dynamic Singly Linked List */
typedef struct singly_linked_list SinglyLinkedList;

SinglyLinkedList *singly_list_create(void);
int singly_list_insert_start(SinglyLinkedList *list, int32_t value);
int singly_list_insert_end(SinglyLinkedList *list, int32_t value);
int singly_list_remove_start(SinglyLinkedList *list);
int singly_list_remove_end(SinglyLinkedList *list);
int singly_list_remove_at(SinglyLinkedList *list, int index);
int singly_list_get(const SinglyLinkedList *list, int index,
                    int32_t *out_value);
int singly_list_set(SinglyLinkedList *list, int index, int32_t value);
int singly_list_from_array(SinglyLinkedList *list, const int32_t *values,
                           int count);
int singly_list_to_array(const SinglyLinkedList *list, int32_t *out_values,
                         int max_count);
int singly_list_size(const SinglyLinkedList *list);
int singly_list_is_empty(const SinglyLinkedList *list);
size_t singly_list_memory_usage(const SinglyLinkedList *list);
void singly_list_clear(SinglyLinkedList *list);
void singly_list_destroy(SinglyLinkedList *list);

/* Pointer-manipulating sorting algorithms on SinglyLinkedList */
int singly_list_sort(SinglyLinkedList *list, int algorithm,
                     const ListSortObserver *observer);

#endif
