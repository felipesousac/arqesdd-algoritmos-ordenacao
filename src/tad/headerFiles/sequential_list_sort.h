#ifndef SEQUENTIAL_LIST_SORT_H
#define SEQUENTIAL_LIST_SORT_H

#include "list_sort_observer.h"
#include "sequential_list.h"
#include <stddef.h>

int seq_list_sort(SequentialList *list, int algorithm, const ListSortObserver *observer, size_t *out_aux_memory_bytes);

#endif
