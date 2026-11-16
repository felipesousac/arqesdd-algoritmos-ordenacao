#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

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

/* Opaque type for Dynamic Doubly Linked List */
typedef struct doubly_linked_list DoublyLinkedList;

DoublyLinkedList *doubly_list_create(void);
int doubly_list_insert_start(DoublyLinkedList *list, int32_t value);
int doubly_list_insert_end(DoublyLinkedList *list, int32_t value);
int doubly_list_remove_start(DoublyLinkedList *list);
int doubly_list_remove_end(DoublyLinkedList *list);
int doubly_list_remove_at(DoublyLinkedList *list, int index);
int doubly_list_get(const DoublyLinkedList *list, int index,
                    int32_t *out_value);
int doubly_list_set(DoublyLinkedList *list, int index, int32_t value);
int doubly_list_from_array(DoublyLinkedList *list, const int32_t *values,
                           int count);
int doubly_list_to_array(const DoublyLinkedList *list, int32_t *out_values,
                         int max_count);
int doubly_list_size(const DoublyLinkedList *list);
int doubly_list_is_empty(const DoublyLinkedList *list);
size_t doubly_list_memory_usage(const DoublyLinkedList *list);
void doubly_list_clear(DoublyLinkedList *list);
void doubly_list_destroy(DoublyLinkedList *list);

/* Pointer-manipulating sorting algorithms on DoublyLinkedList */
int doubly_list_sort(DoublyLinkedList *list, int algorithm,
                     const ListSortObserver *observer);

#endif
