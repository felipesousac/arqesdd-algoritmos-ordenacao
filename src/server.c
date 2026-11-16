#include "mongoose.h"
#include "sort_engine.h"
#include "tad/headerFiles/hash_table.h"
#include "tad/headerFiles/queue.h"
#include "tad/headerFiles/sequential_list.h"
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define API_MAX_INPUT 128
#define MAX_QUERY 4096
#define MAX_JSON (32 * 1024 * 1024)

static HashTable *g_history_table = NULL;

typedef struct {
  char *data;
  size_t length;
  size_t capacity;
  int failed;
} Json;

static void json_append(Json *json, const char *format, ...) {
  if (json->failed) {
    return;
  }

  va_list args;
  va_start(args, format);

  va_list copy;
  va_copy(copy, args);
  int needed = vsnprintf(NULL, 0, format, copy);
  va_end(copy);

  if (needed < 0 || (size_t)needed > MAX_JSON - json->length) {
    json->failed = 1;
    va_end(args);
    return;
  }

  size_t required = json->length + (size_t)needed + 1;
  if (required > json->capacity) {
    size_t capacity = json->capacity ? json->capacity : 4096;
    while (capacity < required) {
      capacity *= 2;
    }

    char *data = (char *)realloc(json->data, capacity);
    if (!data) {
      json->failed = 1;
      va_end(args);
      return;
    }

    json->data = data;
    json->capacity = capacity;
  }

  vsnprintf(json->data + json->length, json->capacity - json->length, format,
            args);
  json->length += (size_t)needed;
  va_end(args);
}

/* Dynamically parses CSV/space-separated integers using SequentialList */
static int parse_values_dynamic(const char *text, int32_t **out_values,
                                int *out_count) {
  *out_values = NULL;
  *out_count = 0;

  if (text == NULL) {
    return 0;
  }

  SequentialList *list = seq_list_create(sizeof(int32_t));
  if (list == NULL) {
    return 0;
  }

  const char *cursor = text;
  while (*cursor) {
    while (*cursor == ' ' || *cursor == ',' || *cursor == ';' ||
           *cursor == '\n' || *cursor == '\r' || *cursor == '\t') {
      cursor++;
    }
    if (!*cursor) {
      break;
    }
    if (seq_list_size(list) >= API_MAX_INPUT) {
      seq_list_destroy(list);
      return 0;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(cursor, &end, 10);
    if (end == cursor || errno != 0 || value < 1 || value > 1000000) {
      seq_list_destroy(list);
      return 0;
    }

    int32_t item = (int32_t)value;
    if (!seq_list_insert_end(list, &item)) {
      seq_list_destroy(list);
      return 0;
    }

    if (*end && *end != ' ' && *end != ',' && *end != ';' && *end != '\n' &&
        *end != '\r' && *end != '\t') {
      seq_list_destroy(list);
      return 0;
    }
    cursor = end;
  }

  int count = seq_list_size(list);
  if (count < 2) {
    seq_list_destroy(list);
    return 0;
  }

  int32_t *values = (int32_t *)malloc((size_t)count * sizeof(int32_t));
  if (values == NULL) {
    seq_list_destroy(list);
    return 0;
  }

  memcpy(values, seq_list_data(list), (size_t)count * sizeof(int32_t));
  seq_list_destroy(list);

  *out_values = values;
  *out_count = count;
  return 1;
}

static int parse_id_list(const char *text, int min_id, int max_id,
                         SequentialList *out_ids) {
  if (text == NULL || text[0] == '\0' || strcmp(text, "all") == 0) {
    for (int id = min_id; id <= max_id; id++) {
      if (!seq_list_insert_end(out_ids, &id)) {
        return 0;
      }
    }
    return 1;
  }

  const char *cursor = text;
  while (*cursor) {
    while (*cursor == ' ' || *cursor == ',' || *cursor == ';') {
      cursor++;
    }
    if (!*cursor) {
      break;
    }

    errno = 0;
    char *end = NULL;
    long val = strtol(cursor, &end, 10);
    if (end == cursor || errno != 0 || val < min_id || val > max_id) {
      return 0;
    }

    int id = (int)val;
    int duplicate = 0;
    for (int i = 0; i < seq_list_size(out_ids); i++) {
      int existing = -1;
      seq_list_get(out_ids, i, &existing);
      if (existing == id) {
        duplicate = 1;
        break;
      }
    }
    if (!duplicate) {
      if (!seq_list_insert_end(out_ids, &id)) {
        return 0;
      }
    }
    cursor = end;
  }

  return seq_list_size(out_ids) > 0;
}

static uint64_t record_run_in_hash_table(const int32_t *input,
                                         const int32_t *sorted,
                                         const Metrics *metrics,
                                         const Operation *operations) {
  if (g_history_table == NULL) {
    return 0;
  }

  const char *alg_name = sort_algorithm_name(metrics->algorithm);
  const char *struct_name = sort_structure_name(metrics->structure_type);

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

static char *build_sort_json(uint64_t test_id, const int32_t *input,
                             const int32_t *sorted, const Metrics *metrics,
                             const Operation *operations) {
  Json json = {0};
  SequentialList *history = seq_list_create_with_capacity(
      sizeof(Operation), metrics->operation_count > 0 ? metrics->operation_count : 8);
  if (!history) {
    return NULL;
  }

  for (int i = 0; i < metrics->operation_count; i++) {
    if (!seq_list_insert_end(history, &operations[i])) {
      json.failed = 1;
      break;
    }
  }

  json_append(&json,
              "{\"testId\":%llu,\"algorithm\":%d,\"algorithmName\":\"%s\","
              "\"structure\":%d,\"structureName\":\"%s\",\"input\":[",
              (unsigned long long)test_id, metrics->algorithm,
              sort_algorithm_name(metrics->algorithm), metrics->structure_type,
              sort_structure_name(metrics->structure_type));
  for (int i = 0; i < metrics->item_count; i++) {
    json_append(&json, "%s%d", i ? "," : "", input[i]);
  }

  json_append(&json, "],\"sorted\":[");
  for (int i = 0; i < metrics->item_count; i++) {
    json_append(&json, "%s%d", i ? "," : "", sorted[i]);
  }

  json_append(&json,
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

    json_append(&json,
                "%s{\"type\":%d,\"firstIndex\":%d,"
                "\"secondIndex\":%d,\"firstValue\":%d,"
                "\"secondValue\":%d}",
                i ? "," : "", operation.type, operation.first_index,
                operation.second_index, operation.first_value,
                operation.second_value);
  }

  json_append(&json, "]}");
  seq_list_destroy(history);
  if (json.failed) {
    free(json.data);
    return NULL;
  }
  return json.data;
}

static void reply_error(struct mg_connection *c, int status,
                        const char *message) {
  mg_http_reply(c, status, "Content-Type: application/json; charset=utf-8\r\n",
                "{\"error\":\"%s\"}\n", message);
}

static void handle_api_sort(struct mg_connection *c,
                            struct mg_http_message *request) {
  if (mg_strcmp(request->method, mg_str("GET")) != 0) {
    reply_error(c, 405, "Method Not Allowed: use GET");
    return;
  }

  char algorithm_text[16] = {0};
  char structure_text[16] = {0};
  char values_text[MAX_QUERY] = {0};

  if (mg_http_get_var(&request->query, "algorithm", algorithm_text,
                      sizeof(algorithm_text)) <= 0 ||
      mg_http_get_var(&request->query, "values", values_text,
                      sizeof(values_text)) <= 0) {
    reply_error(c, 400, "Missing required parameters: algorithm and values");
    return;
  }

  long structure = STRUCT_SEQUENTIAL;
  if (mg_http_get_var(&request->query, "structure", structure_text,
                      sizeof(structure_text)) > 0) {
    char *s_end = NULL;
    structure = strtol(structure_text, &s_end, 10);
    if (*s_end || structure < STRUCT_SEQUENTIAL ||
        structure > STRUCT_CIRCULAR) {
      reply_error(c, 400, "Invalid structure parameter");
      return;
    }
  }

  char *end = NULL;
  long algorithm = strtol(algorithm_text, &end, 10);
  int32_t *values = NULL;
  int count = 0;

  if (*end || algorithm < ALGORITHM_BUBBLE || algorithm > ALGORITHM_MERGE ||
      !parse_values_dynamic(values_text, &values, &count)) {
    free(values);
    reply_error(c, 400, "Invalid parameters");
    return;
  }

  if (!sort_run(values, count, (int)algorithm, (int)structure, 2000000)) {
    free(values);
    reply_error(c, 500, "Failed to run sorting algorithm");
    return;
  }

  uint64_t test_id = record_run_in_hash_table(values, sort_values(),
                                              sort_metrics(), sort_operations());

  char *body = build_sort_json(test_id, values, sort_values(), sort_metrics(),
                               sort_operations());
  free(values);
  sort_free();

  if (!body) {
    reply_error(c, 500, "Failed to serialize sorting history");
    return;
  }

  mg_http_reply(c, 200, "Content-Type: application/json; charset=utf-8\r\n",
                "%s", body);
  free(body);
}

static void handle_api_batch(struct mg_connection *c,
                             struct mg_http_message *request) {
  if (mg_strcmp(request->method, mg_str("GET")) != 0 &&
      mg_strcmp(request->method, mg_str("POST")) != 0) {
    reply_error(c, 405, "Method Not Allowed: use GET or POST");
    return;
  }

  struct mg_str params_source =
      (request->query.len > 0) ? request->query : request->body;

  char algorithms_text[128] = {0};
  char structures_text[128] = {0};
  char values_text[MAX_QUERY] = {0};

  mg_http_get_var(&params_source, "algorithms", algorithms_text,
                  sizeof(algorithms_text));
  mg_http_get_var(&params_source, "structures", structures_text,
                  sizeof(structures_text));

  if (mg_http_get_var(&params_source, "values", values_text,
                      sizeof(values_text)) <= 0) {
    reply_error(c, 400, "Missing values parameter for batch execution");
    return;
  }

  int32_t *values = NULL;
  int count = 0;
  if (!parse_values_dynamic(values_text, &values, &count)) {
    reply_error(c, 400, "Invalid values sequence for batch execution");
    return;
  }

  SequentialList *alg_ids = seq_list_create(sizeof(int));
  SequentialList *struct_ids = seq_list_create(sizeof(int));
  if (!alg_ids || !struct_ids ||
      !parse_id_list(algorithms_text, ALGORITHM_BUBBLE, ALGORITHM_MERGE,
                     alg_ids) ||
      !parse_id_list(structures_text, STRUCT_SEQUENTIAL, STRUCT_CIRCULAR,
                     struct_ids)) {
    free(values);
    seq_list_destroy(alg_ids);
    seq_list_destroy(struct_ids);
    reply_error(c, 400, "Invalid algorithms or structures list");
    return;
  }

  Queue *batch_queue = queue_create();
  if (!batch_queue) {
    free(values);
    seq_list_destroy(alg_ids);
    seq_list_destroy(struct_ids);
    reply_error(c, 500, "Failed to allocate batch queue");
    return;
  }

  /* Enqueue all tasks in the Dynamic Queue TAD */
  for (int i = 0; i < seq_list_size(alg_ids); i++) {
    int alg = 0;
    seq_list_get(alg_ids, i, &alg);
    for (int j = 0; j < seq_list_size(struct_ids); j++) {
      int st = 0;
      seq_list_get(struct_ids, j, &st);

      BatchTask task = {.algorithm = alg,
                        .structure_type = st,
                        .input_values = values,
                        .item_count = count};
      if (!queue_enqueue(batch_queue, &task)) {
        queue_destroy(batch_queue);
        free(values);
        seq_list_destroy(alg_ids);
        seq_list_destroy(struct_ids);
        reply_error(c, 500, "Failed to enqueue batch task");
        return;
      }
    }
  }

  free(values);
  seq_list_destroy(alg_ids);
  seq_list_destroy(struct_ids);

  int total_tasks = queue_size(batch_queue);
  size_t queue_peak_memory = queue_memory_usage(batch_queue);

  Json json = {0};
  json_append(&json,
              "{\"queueTasksProcessed\":%d,\"queuePeakMemoryBytes\":%zu,"
              "\"itemCount\":%d,\"results\":[",
              total_tasks, queue_peak_memory, count);

  int result_index = 0;
  BatchTask task;
  while (queue_dequeue(batch_queue, &task)) {
    if (sort_run(task.input_values, task.item_count, task.algorithm,
                 task.structure_type, 2000000)) {
      const Metrics *m = sort_metrics();
      uint64_t test_id = record_run_in_hash_table(
          task.input_values, sort_values(), m, sort_operations());

      json_append(&json,
                  "%s{\"testId\":%llu,\"algorithm\":%d,\"algorithmName\":\"%s\","
                  "\"structure\":%d,\"structureName\":\"%s\","
                  "\"metrics\":{\"itemCount\":%d,\"comparisons\":%lld,"
                  "\"swaps\":%lld,\"elapsedMs\":%.6f,\"memoryBytes\":%zu,"
                  "\"operationCount\":%d}}",
                  result_index ? "," : "", (unsigned long long)test_id,
                  m->algorithm, sort_algorithm_name(m->algorithm),
                  m->structure_type, sort_structure_name(m->structure_type),
                  m->item_count, (long long)m->comparisons,
                  (long long)m->swaps, m->elapsed_ms, m->memory_bytes,
                  m->operation_count);
      result_index++;
      sort_free();
    }
    batch_task_free(&task);
  }

  queue_destroy(batch_queue);
  json_append(&json, "]}");

  if (json.failed || !json.data) {
    free(json.data);
    reply_error(c, 500, "Failed to serialize batch results");
    return;
  }

  mg_http_reply(c, 200, "Content-Type: application/json; charset=utf-8\r\n",
                "%s", json.data);
  free(json.data);
}

static void handle_api_history(struct mg_connection *c,
                               struct mg_http_message *request) {
  if (mg_strcmp(request->method, mg_str("GET")) != 0) {
    reply_error(c, 405, "Method Not Allowed: use GET");
    return;
  }

  char algorithm_filter[64] = {0};
  mg_http_get_var(&request->query, "algorithm", algorithm_filter,
                  sizeof(algorithm_filter));

  const char *lookup_key = algorithm_filter;
  if (algorithm_filter[0] >= '0' && algorithm_filter[0] <= '4' &&
      algorithm_filter[1] == '\0') {
    lookup_key = sort_algorithm_name(algorithm_filter[0] - '0');
  } else if (strcmp(algorithm_filter, "all") == 0) {
    lookup_key = "";
  }

  SequentialList *reports =
      hash_table_search_by_algorithm(g_history_table, lookup_key);
  if (!reports) {
    reply_error(c, 500, "Failed to query Hash Table");
    return;
  }

  Json json = {0};
  json_append(&json,
              "{\"tableStats\":{\"totalTests\":%zu,\"bucketCount\":%zu,"
              "\"memoryBytes\":%zu},\"reports\":[",
              hash_table_count(g_history_table),
              hash_table_bucket_count(g_history_table),
              hash_table_memory_usage(g_history_table));

  for (int i = 0; i < seq_list_size(reports); i++) {
    const TestReport *rep = NULL;
    seq_list_get(reports, i, &rep);
    if (!rep) {
      continue;
    }

    json_append(&json,
                "%s{\"testId\":%llu,\"algorithm\":%d,\"algorithmName\":\"%s\","
                "\"structure\":%d,\"structureName\":\"%s\","
                "\"metrics\":{\"itemCount\":%d,\"comparisons\":%lld,"
                "\"swaps\":%lld,\"elapsedMs\":%.6f,\"memoryBytes\":%zu,"
                "\"operationCount\":%d}}",
                i ? "," : "", (unsigned long long)rep->test_id,
                rep->algorithm_id, rep->algorithm_name, rep->structure_type,
                rep->structure_name, rep->metrics.item_count,
                (long long)rep->metrics.comparisons,
                (long long)rep->metrics.swaps, rep->metrics.elapsed_ms,
                rep->metrics.memory_bytes, rep->metrics.operation_count);
  }

  json_append(&json, "]}");
  seq_list_destroy(reports);

  if (json.failed || !json.data) {
    free(json.data);
    reply_error(c, 500, "Failed to serialize history response");
    return;
  }

  mg_http_reply(c, 200, "Content-Type: application/json; charset=utf-8\r\n",
                "%s", json.data);
  free(json.data);
}

static void handle_api_history_detail(struct mg_connection *c,
                                      struct mg_http_message *request) {
  if (mg_strcmp(request->method, mg_str("GET")) != 0) {
    reply_error(c, 405, "Method Not Allowed: use GET");
    return;
  }

  char id_text[32] = {0};
  if (mg_http_get_var(&request->query, "id", id_text, sizeof(id_text)) <= 0) {
    reply_error(c, 400, "Missing id parameter");
    return;
  }

  char *end = NULL;
  unsigned long long test_id = strtoull(id_text, &end, 10);
  if (*end || test_id == 0) {
    reply_error(c, 400, "Invalid test id");
    return;
  }

  const TestReport *rep =
      hash_table_get_by_id(g_history_table, (uint64_t)test_id);
  if (!rep) {
    reply_error(c, 404, "Test report not found in Hash Table");
    return;
  }

  char *body = build_sort_json(rep->test_id, rep->input_values,
                               rep->sorted_values, &rep->metrics,
                               rep->operations);
  if (!body) {
    reply_error(c, 500, "Failed to serialize test detail");
    return;
  }

  mg_http_reply(c, 200, "Content-Type: application/json; charset=utf-8\r\n",
                "%s", body);
  free(body);
}

static void handle_api_history_clear(struct mg_connection *c,
                                     struct mg_http_message *request) {
  if (mg_strcmp(request->method, mg_str("POST")) != 0 &&
      mg_strcmp(request->method, mg_str("DELETE")) != 0) {
    reply_error(c, 405, "Method Not Allowed: use POST or DELETE");
    return;
  }

  hash_table_clear(g_history_table);

  mg_http_reply(
      c, 200, "Content-Type: application/json; charset=utf-8\r\n",
      "{\"cleared\":true,\"tableStats\":{\"totalTests\":%zu,"
      "\"bucketCount\":%zu,\"memoryBytes\":%zu}}\n",
      hash_table_count(g_history_table),
      hash_table_bucket_count(g_history_table),
      hash_table_memory_usage(g_history_table));
}

static void event_handler(struct mg_connection *c, int event,
                          void *event_data) {
  if (event != MG_EV_HTTP_MSG) {
    return;
  }

  struct mg_http_message *request = (struct mg_http_message *)event_data;
  if (mg_match(request->uri, mg_str("/api/sort"), NULL)) {
    handle_api_sort(c, request);
    return;
  }
  if (mg_match(request->uri, mg_str("/api/batch"), NULL)) {
    handle_api_batch(c, request);
    return;
  }
  if (mg_match(request->uri, mg_str("/api/history/clear"), NULL)) {
    handle_api_history_clear(c, request);
    return;
  }
  if (mg_match(request->uri, mg_str("/api/history/detail"), NULL)) {
    handle_api_history_detail(c, request);
    return;
  }
  if (mg_match(request->uri, mg_str("/api/history"), NULL)) {
    handle_api_history(c, request);
    return;
  }

  if (mg_strcmp(request->method, mg_str("GET")) == 0) {
    const char *path = NULL;
    if (mg_match(request->uri, mg_str("/"), NULL) ||
        mg_match(request->uri, mg_str("/index.html"), NULL)) {
      path = "index.html";
    } else if (mg_match(request->uri, mg_str("/web/app.js"), NULL)) {
      path = "web/app.js";
    } else if (mg_match(request->uri, mg_str("/web/style.css"), NULL)) {
      path = "web/style.css";
    }

    if (path) {
      struct mg_http_serve_opts options = {.fs = &mg_fs_posix};
      mg_http_serve_file(c, request, path, &options);
    } else {
      reply_error(c, 404, "Route not found");
    }
  } else {
    reply_error(c, 404, "Route not found");
  }
}

int main(int argc, char **argv) {
  const char *address = argc > 1 ? argv[1] : "http://0.0.0.0:8000";

  g_history_table = hash_table_create(HASH_TABLE_DEFAULT_BUCKETS);
  if (!g_history_table) {
    fprintf(stderr, "Failed to initialize in-memory Hash Table\n");
    return 1;
  }

  struct mg_mgr manager;
  mg_mgr_init(&manager);
  if (!mg_http_listen(&manager, address, event_handler, NULL)) {
    fprintf(stderr, "Could not start server on %s\n", address);
    mg_mgr_free(&manager);
    hash_table_destroy(g_history_table);
    return 1;
  }

  printf("Ordena Server listening on %s\n", address);
  for (;;) {
    mg_mgr_poll(&manager, 1000);
  }

  hash_table_destroy(g_history_table);
  return 0;
}
