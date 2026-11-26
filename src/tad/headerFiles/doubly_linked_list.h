#ifndef DOUBLY_LINKED_LIST_H
#define DOUBLY_LINKED_LIST_H

#include "list_sort_observer.h"
#include <stddef.h>
#include <stdint.h>

typedef struct doubly_linked_list DoublyLinkedList;

DoublyLinkedList *doubly_list_create(void);
int doubly_list_insert_start(DoublyLinkedList *list, int32_t value);
int doubly_list_insert_end(DoublyLinkedList *list, int32_t value);
int doubly_list_remove_start(DoublyLinkedList *list);
int doubly_list_remove_end(DoublyLinkedList *list);
int doubly_list_remove_at(DoublyLinkedList *list, int index);
int doubly_list_get(const DoublyLinkedList *list, int index, int32_t *out_value);
int doubly_list_set(DoublyLinkedList *list, int index, int32_t value);
int doubly_list_from_array(DoublyLinkedList *list, const int32_t *values, int count);
int doubly_list_to_array(const DoublyLinkedList *list, int32_t *out_values, int max_count);
int doubly_list_size(const DoublyLinkedList *list);
int doubly_list_is_empty(const DoublyLinkedList *list);
size_t doubly_list_memory_usage(const DoublyLinkedList *list);
void doubly_list_clear(DoublyLinkedList *list);
void doubly_list_destroy(DoublyLinkedList *list);
int doubly_list_sort(DoublyLinkedList *list, int algorithm, const ListSortObserver *observer);

#endif
