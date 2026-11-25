#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "../../sort_engine.h"
#include "sequential_list.h"
#include <stddef.h>
#include <stdint.h>

#define HASH_TABLE_DEFAULT_BUCKETS 11

typedef struct {
  uint64_t test_id;
  char *algorithm_name;
  char *structure_name;
  int algorithm_id;
  int structure_type;
  Metrics metrics;
  int32_t *input_values;
  int32_t *sorted_values;
  Operation *operations;
} TestReport;

/* Opaque type for Dynamic Hash Table with Separate Chaining */
typedef struct hash_table HashTable;

HashTable *hash_table_create(size_t initial_bucket_count);
uint64_t hash_table_insert(HashTable *table, const char *algorithm_name,
                           const TestReport *report);
SequentialList *hash_table_search_by_algorithm(const HashTable *table,
                                               const char *algorithm_name);
const TestReport *hash_table_get_by_id(const HashTable *table,
                                       uint64_t test_id);
size_t hash_table_count(const HashTable *table);
size_t hash_table_bucket_count(const HashTable *table);
size_t hash_table_occupied_bucket_count(const HashTable *table);
size_t hash_table_memory_usage(const HashTable *table);
void hash_table_clear(HashTable *table);
void hash_table_destroy(HashTable *table);

#endif
