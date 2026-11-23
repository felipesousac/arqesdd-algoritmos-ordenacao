#include "../headerFiles/json_serializer.h"
#include "../../tad/headerFiles/hash_table.h"

char *json_serialize_sort_result(uint64_t test_id, const int32_t *input,
                                 const int32_t *sorted, const Metrics *metrics,
                                 const Operation *operations) {
  JsonBuilder json;
  json_builder_init(&json);

  SequentialList *history = seq_list_create_with_capacity(
      sizeof(Operation),
      metrics->operation_count > 0 ? metrics->operation_count : 8);

  if (!history) {
    return NULL;
  }

  for (int i = 0; i < metrics->operation_count; i++) {
    if (!seq_list_insert_end(history, &operations[i])) {
      json.failed = 1;
      break;
    }
  }

  json_builder_append(&json,
                      "{\"testId\":%llu,\"algorithm\":%d,\"algorithmName\":\"%s\","
                      "\"structure\":%d,\"structureName\":\"%s\",\"input\":[",
                      (unsigned long long)test_id, metrics->algorithm,
                      get_sort_algorithm_name(metrics->algorithm),
                      metrics->structure_type,
                      get_sort_structure_name(metrics->structure_type));

  for (int i = 0; i < metrics->item_count; i++) {
    json_builder_append(&json, "%s%d", i ? "," : "", input[i]);
  }

  json_builder_append(&json, "],\"sorted\":[");

  for (int i = 0; i < metrics->item_count; i++) {
    json_builder_append(&json, "%s%d", i ? "," : "", sorted[i]);
  }

  json_builder_append(&json,
                      "],\"metrics\":{\"itemCount\":%d,"
                      "\"comparisons\":%lld,\"swaps\":%lld,"
                      "\"elapsedMs\":%.6f,\"memoryBytes\":%zu,\"operationCount\":%d},"
                      "\"operations\":[",
                      metrics->item_count, (long long)metrics->comparisons,
                      (long long)metrics->swaps, metrics->elapsed_ms,
                      metrics->memory_bytes, metrics->operation_count);

  for (int i = 0; i < seq_list_size(history); i++) {
    Operation operation;

    if (!seq_list_get(history, i, &operation)) {
      json.failed = 1;
      break;
    }

    json_builder_append(&json,
                        "%s{\"type\":%d,\"firstIndex\":%d,"
                        "\"secondIndex\":%d,\"firstValue\":%d,"
                        "\"secondValue\":%d}",
                        i ? "," : "", operation.type, operation.first_index,
                        operation.second_index, operation.first_value,
                        operation.second_value);
  }

  json_builder_append(&json, "]}");
  seq_list_destroy(history);

  return json_builder_detach(&json);
}

char *json_serialize_history(const HistoryStats *stats,
                             const SequentialList *reports) {
  JsonBuilder json;
  json_builder_init(&json);

  json_builder_append(&json,
                      "{\"tableStats\":{\"totalTests\":%zu,\"bucketCount\":%zu,"
                      "\"memoryBytes\":%zu},\"reports\":[",
                      stats->total_tests, stats->bucket_count,
                      stats->memory_bytes);

  for (int i = 0; i < seq_list_size(reports); i++) {
    const TestReport *rep = NULL;
    seq_list_get(reports, i, &rep);
    if (!rep) {
      continue;
    }

    json_builder_append(&json,
                        "%s{\"testId\":%llu,\"algorithm\":%d,\"algorithmName\":\"%s\","
                        "\"structure\":%d,\"structureName\":\"%s\","
                        "\"metrics\":{\"itemCount\":%d,\"comparisons\":%lld,"
                        "\"swaps\":%lld,\"elapsedMs\":%.6f,\"memoryBytes\":%zu,"
                        "\"operationCount\":%d}}",
                        i ? "," : "", (unsigned long long)rep->test_id,
                        rep->algorithm_id, rep->algorithm_name,
                        rep->structure_type, rep->structure_name,
                        rep->metrics.item_count,
                        (long long)rep->metrics.comparisons,
                        (long long)rep->metrics.swaps, rep->metrics.elapsed_ms,
                        rep->metrics.memory_bytes,
                        rep->metrics.operation_count);
  }

  json_builder_append(&json, "]}");
  return json_builder_detach(&json);
}

char *json_serialize_history_clear(const HistoryStats *stats) {
  JsonBuilder json;
  json_builder_init(&json);

  json_builder_append(&json,
                      "{\"cleared\":true,\"tableStats\":{\"totalTests\":%zu,"
                      "\"bucketCount\":%zu,\"memoryBytes\":%zu}}\n",
                      stats->total_tests, stats->bucket_count,
                      stats->memory_bytes);

  return json_builder_detach(&json);
}

void json_append_batch_header(JsonBuilder *json, int total_tasks,
                              size_t queue_peak_memory, int item_count) {
  json_builder_append(json,
                      "{\"queueTasksProcessed\":%d,\"queuePeakMemoryBytes\":%zu,"
                      "\"itemCount\":%d,\"results\":[",
                      total_tasks, queue_peak_memory, item_count);
}

void json_append_batch_item(JsonBuilder *json, int index, uint64_t test_id,
                            const Metrics *metrics) {
  json_builder_append(json,
                      "%s{\"testId\":%llu,\"algorithm\":%d,\"algorithmName\":\"%s\","
                      "\"structure\":%d,\"structureName\":\"%s\","
                      "\"metrics\":{\"itemCount\":%d,\"comparisons\":%lld,"
                      "\"swaps\":%lld,\"elapsedMs\":%.6f,\"memoryBytes\":%zu,"
                      "\"operationCount\":%d}}",
                      index ? "," : "", (unsigned long long)test_id,
                      metrics->algorithm,
                      get_sort_algorithm_name(metrics->algorithm),
                      metrics->structure_type,
                      get_sort_structure_name(metrics->structure_type),
                      metrics->item_count, (long long)metrics->comparisons,
                      (long long)metrics->swaps, metrics->elapsed_ms,
                      metrics->memory_bytes, metrics->operation_count);
}

void json_append_batch_footer(JsonBuilder *json) {
  json_builder_append(json, "]}");
}
