#ifndef HISTORY_STORE_H
#define HISTORY_STORE_H

#include "../../sort_engine.h"
#include "../../tad/headerFiles/hash_table.h"
#include "../../tad/headerFiles/sequential_list.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t total_tests;
  size_t bucket_count;
  size_t memory_bytes;
} HistoryStats;

int history_store_init(void);
void history_store_destroy(void);
uint64_t history_store_record(const int32_t *input, const int32_t *sorted,
                              const Metrics *metrics,
                              const Operation *operations);
SequentialList *history_store_search(const char *algorithm_key);
const TestReport *history_store_get_by_id(uint64_t test_id);
void history_store_clear(void);
HistoryStats history_store_stats(void);

#endif
