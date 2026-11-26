#ifndef SEQUENTIAL_LIST_H
#define SEQUENTIAL_LIST_H

#include <stddef.h>

#define SEQ_LIST_INITIAL_CAPACITY 8

typedef struct sequential_list SequentialList;

SequentialList *seq_list_create(size_t element_size);
SequentialList *seq_list_create_with_capacity(size_t element_size, int initial_capacity);
int seq_list_insert_end(SequentialList *list, const void *element);
int seq_list_insert_start(SequentialList *list, const void *element);
int seq_list_remove_at(SequentialList *list, int index);
int seq_list_remove_start(SequentialList *list);
int seq_list_remove_end(SequentialList *list);
int seq_list_get(const SequentialList *list, int index, void *out_element);
int seq_list_set(SequentialList *list, int index, const void *element);
int seq_list_from_array(SequentialList *list, const void *elements, int count);
void *seq_list_at(const SequentialList *list, int index);
void *seq_list_data(const SequentialList *list);
int seq_list_size(const SequentialList *list);
int seq_list_capacity(const SequentialList *list);
size_t seq_list_element_size(const SequentialList *list);
int seq_list_is_empty(const SequentialList *list);
size_t seq_list_memory_usage(const SequentialList *list);
void seq_list_clear(SequentialList *list);
void seq_list_destroy(SequentialList *list);

#endif
