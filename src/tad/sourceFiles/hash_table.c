#include "../headerFiles/hash_table.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct hash_node {
  TestReport report;
  struct hash_node *next;
} HashNode;

struct hash_table {
  HashNode **buckets;
  size_t bucket_count;
  size_t item_count;
  uint64_t next_test_id;
  size_t memory_bytes;
};

static char *duplicate_string(const char *str) {
  if (str == NULL) {
    return NULL;
  }
  size_t len = strlen(str) + 1;
  char *copy = (char *)malloc(len);
  if (copy != NULL) {
    memcpy(copy, str, len);
  }
  return copy;
}

/* Polynomial / DJB2 string hash function modulo M */
static size_t hash_function(const char *key, size_t bucket_count) {
  if (key == NULL || bucket_count == 0) {
    return 0;
  }

  unsigned long hash = 5381;
  while (*key) {
    unsigned char ch = (unsigned char)tolower((unsigned char)*key++);
    hash = ((hash << 5) + hash) + ch;
  }
  return (size_t)(hash % bucket_count);
}

static int strings_equal_ignore_case(const char *a, const char *b) {
  if (a == NULL || b == NULL) {
    return 0;
  }
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
      return 0;
    }
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static size_t report_dynamic_bytes(const TestReport *report) {
  if (report == NULL) {
    return 0;
  }
  size_t bytes = 0;
  if (report->algorithm_name != NULL) {
    bytes += strlen(report->algorithm_name) + 1;
  }
  if (report->structure_name != NULL) {
    bytes += strlen(report->structure_name) + 1;
  }
  bytes += (size_t)report->metrics.item_count * sizeof(int32_t) * 2;
  bytes += (size_t)report->metrics.operation_count * sizeof(Operation);
  return bytes;
}

static void free_report_contents(TestReport *report) {
  if (report == NULL) {
    return;
  }

  free(report->algorithm_name);
  free(report->structure_name);
  free(report->input_values);
  free(report->sorted_values);
  free(report->operations);
  memset(report, 0, sizeof(TestReport));
}

HashTable *hash_table_create(size_t initial_bucket_count) {
  if (initial_bucket_count < 3) {
    initial_bucket_count = HASH_TABLE_DEFAULT_BUCKETS;
  }

  HashTable *table = (HashTable *) malloc(sizeof(HashTable));

  if (table == NULL) {
    return NULL;
  }

  table->buckets =
      (HashNode **)malloc(initial_bucket_count * sizeof(HashNode *));

  if (table->buckets == NULL) {
    free(table);

    return NULL;
  }

  memset(table->buckets, 0, initial_bucket_count * sizeof(HashNode *));

  table->bucket_count = initial_bucket_count;
  table->item_count = 0;
  table->next_test_id = 1;
  table->memory_bytes =
      sizeof(HashTable) + initial_bucket_count * sizeof(HashNode *);
      
  return table;
}

static int hash_table_resize(HashTable *table, size_t new_bucket_count) {
  if (table == NULL || new_bucket_count <= table->bucket_count) {
    return 0;
  }

  /* Collect all existing chained nodes before resizing the bucket pointer array
   */
  HashNode *all_nodes = NULL;
  for (size_t i = 0; i < table->bucket_count; i++) {
    HashNode *curr = table->buckets[i];
    while (curr != NULL) {
      HashNode *next = curr->next;
      curr->next = all_nodes;
      all_nodes = curr;
      curr = next;
    }
    table->buckets[i] = NULL;
  }

  HashNode **resized_buckets = (HashNode **)realloc(
      table->buckets, new_bucket_count * sizeof(HashNode *));
  if (resized_buckets == NULL) {
    /* Restore nodes to old buckets if realloc fails */
    while (all_nodes != NULL) {
      HashNode *next = all_nodes->next;
      size_t idx = hash_function(all_nodes->report.algorithm_name,
                                 table->bucket_count);
      all_nodes->next = table->buckets[idx];
      table->buckets[idx] = all_nodes;
      all_nodes = next;
    }
    return 0;
  }

  memset(resized_buckets, 0, new_bucket_count * sizeof(HashNode *));
  table->memory_bytes +=
      (new_bucket_count - table->bucket_count) * sizeof(HashNode *);
  table->buckets = resized_buckets;
  table->bucket_count = new_bucket_count;

  /* Rehash all nodes into the new bucket array (Separate Chaining) */
  while (all_nodes != NULL) {
    HashNode *next = all_nodes->next;
    size_t idx =
        hash_function(all_nodes->report.algorithm_name, table->bucket_count);
    all_nodes->next = table->buckets[idx];
    table->buckets[idx] = all_nodes;
    all_nodes = next;
  }

  return 1;
}

uint64_t hash_table_insert(HashTable *table, const char *algorithm_name, const TestReport *report) {
  if (table == NULL || algorithm_name == NULL || report == NULL ||
      report->input_values == NULL || report->sorted_values == NULL ||
      report->metrics.item_count < 1) {
    return 0;
  }

  /* Resize bucket array when load factor exceeds 0.75 */
  if ((double)(table->item_count + 1) / (double)table->bucket_count > 0.75) {
    hash_table_resize(table, table->bucket_count * 2 + 1);
  }

  HashNode *node = (HashNode *)malloc(sizeof(HashNode));

  if (node == NULL) {
    return 0;
  }

  memset(node, 0, sizeof(HashNode));

  node->report.test_id = table->next_test_id++;
  node->report.algorithm_id = report->algorithm_id;
  node->report.structure_type = report->structure_type;
  node->report.metrics = report->metrics;

  node->report.algorithm_name = duplicate_string(algorithm_name);
  node->report.structure_name = duplicate_string(
      report->structure_name ? report->structure_name
                             : get_sort_structure_name(report->structure_type));

  size_t values_bytes = (size_t)report->metrics.item_count * sizeof(int32_t);
  node->report.input_values = (int32_t *)malloc(values_bytes);
  node->report.sorted_values = (int32_t *)malloc(values_bytes);

  if (node->report.algorithm_name == NULL ||
      node->report.structure_name == NULL ||
      node->report.input_values == NULL ||
      node->report.sorted_values == NULL) {
    free_report_contents(&node->report);
    free(node);

    return 0;
  }

  memcpy(node->report.input_values, report->input_values, values_bytes);
  memcpy(node->report.sorted_values, report->sorted_values, values_bytes);

  if (report->metrics.operation_count > 0 && report->operations != NULL) {
    size_t ops_bytes = (size_t)report->metrics.operation_count * sizeof(Operation);

    node->report.operations = (Operation *)malloc(ops_bytes);

    if (node->report.operations == NULL) {
      free_report_contents(&node->report);
      free(node);

      return 0;
    }

    memcpy(node->report.operations, report->operations, ops_bytes);
  } else {
    node->report.operations = NULL;
    node->report.metrics.operation_count = 0;
  }

  /* Separate Chaining insertion at the bucket corresponding to algorithm_name
   */
  size_t bucket_index = hash_function(algorithm_name, table->bucket_count);
  node->next = table->buckets[bucket_index];
  table->buckets[bucket_index] = node;

  table->item_count++;
  table->memory_bytes += sizeof(HashNode) + report_dynamic_bytes(&node->report);
  
  return node->report.test_id;
}

SequentialList *hash_table_search_by_algorithm(const HashTable *table,
                                               const char *algorithm_name) {
  if (table == NULL) {
    return NULL;
  }

  SequentialList *results = seq_list_create(sizeof(const TestReport *));
  if (results == NULL) {
    return NULL;
  }

  if (algorithm_name != NULL && algorithm_name[0] != '\0') {
    /* Lookup directly in the hashed bucket chain */
    size_t bucket_index = hash_function(algorithm_name, table->bucket_count);
    const HashNode *curr = table->buckets[bucket_index];
    while (curr != NULL) {
      if (strings_equal_ignore_case(curr->report.algorithm_name,
                                    algorithm_name)) {
        const TestReport *ptr = &curr->report;
        seq_list_insert_end(results, &ptr);
      }
      curr = curr->next;
    }
  } else {
    /* Return all stored reports across all buckets */
    for (size_t i = 0; i < table->bucket_count; i++) {
      const HashNode *curr = table->buckets[i];
      while (curr != NULL) {
        const TestReport *ptr = &curr->report;
        seq_list_insert_end(results, &ptr);
        curr = curr->next;
      }
    }
  }

  return results;
}

const TestReport *hash_table_get_by_id(const HashTable *table,
                                       uint64_t test_id) {
  if (table == NULL || test_id == 0) {
    return NULL;
  }

  for (size_t i = 0; i < table->bucket_count; i++) {
    const HashNode *curr = table->buckets[i];
    while (curr != NULL) {
      if (curr->report.test_id == test_id) {
        return &curr->report;
      }
      curr = curr->next;
    }
  }
  return NULL;
}

size_t hash_table_count(const HashTable *table) {
  if (table == NULL) {
    return 0;
  }
  return table->item_count;
}

size_t hash_table_bucket_count(const HashTable *table) {
  if (table == NULL) {
    return 0;
  }
  return table->bucket_count;
}

size_t hash_table_memory_usage(const HashTable *table) {
  if (table == NULL) {
    return 0;
  }
  return table->memory_bytes;
}

void hash_table_clear(HashTable *table) {
  if (table == NULL) {
    return;
  }

  for (size_t i = 0; i < table->bucket_count; i++) {
    HashNode *curr = table->buckets[i];
    while (curr != NULL) {
      HashNode *next = curr->next;
      free_report_contents(&curr->report);
      free(curr);
      curr = next;
    }
    table->buckets[i] = NULL;
  }

  table->item_count = 0;
  table->next_test_id = 1;
  table->memory_bytes =
      sizeof(HashTable) + table->bucket_count * sizeof(HashNode *);
}

void hash_table_destroy(HashTable *table) {
  if (table != NULL) {
    hash_table_clear(table);
    free(table->buckets);
    free(table);
  }
}
