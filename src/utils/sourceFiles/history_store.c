#include "../headerFiles/history_store.h"
#include <string.h>

static HashTable *g_history_table = NULL;

int history_store_init(void) {
  if (g_history_table != NULL) {
    return 1;
  }

  g_history_table = hash_table_create(HASH_TABLE_DEFAULT_BUCKETS);
  return g_history_table != NULL;
}

void history_store_destroy(void) {
  if (g_history_table != NULL) {
    hash_table_destroy(g_history_table);
    g_history_table = NULL;
  }
}

uint64_t history_store_record(const int32_t *input, const int32_t *sorted,
                              const Metrics *metrics,
                              const Operation *operations) {
  if (g_history_table == NULL || metrics == NULL) {
    return 0;
  }

  const char *alg_name = get_sort_algorithm_name(metrics->algorithm);
  const char *struct_name = get_sort_structure_name(metrics->structure_type);

  TestReport report;
  memset(&report, 0, sizeof(report));
  report.algorithm_id = metrics->algorithm;
  report.structure_type = metrics->structure_type;
  report.algorithm_name = (char *)alg_name;
  report.structure_name = (char *)struct_name;
  report.metrics = *metrics;
  report.input_values = (int32_t *)input;
  report.sorted_values = (int32_t *)sorted;
  report.operations = (Operation *)operations;

  return hash_table_insert(g_history_table, alg_name, &report);
}

SequentialList *history_store_search(const char *algorithm_key) {
  if (g_history_table == NULL) {
    return NULL;
  }

  return hash_table_search_by_algorithm(g_history_table, algorithm_key);
}

const TestReport *history_store_get_by_id(uint64_t test_id) {
  if (g_history_table == NULL) {
    return NULL;
  }

  return hash_table_get_by_id(g_history_table, test_id);
}

void history_store_clear(void) {
  if (g_history_table != NULL) {
    hash_table_clear(g_history_table);
  }
}

HistoryStats history_store_stats(void) {
  HistoryStats stats = {0, 0, 0};

  if (g_history_table != NULL) {
    stats.total_tests = hash_table_count(g_history_table);
    stats.bucket_count = hash_table_bucket_count(g_history_table);
    stats.memory_bytes = hash_table_memory_usage(g_history_table);
  }

  return stats;
}
