#include "server.h"
#include "../vendor/mongoose.h"
#include "sort_engine.h"
#include "tad/headerFiles/sequential_list.h"
#include "utils/headerFiles/batch_runner.h"
#include "utils/headerFiles/history_store.h"
#include "utils/headerFiles/http_response.h"
#include "utils/headerFiles/input_parser.h"
#include "utils/headerFiles/json_serializer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_QUERY 4096
#define SORT_OPERATION_LIMIT 2000000

static void handle_api_sort(struct mg_connection *c, struct mg_http_message *request) {
  if (!http_is_method(request, "GET")) {
    http_reply_error(c, 405, "Method Not Allowed: use GET");

    return;
  }

  char algorithm_text[16] = {0};
  char structure_text[16] = {0};
  char values_text[MAX_QUERY] = {0};

  if (mg_http_get_var(&request->query, "algorithm", algorithm_text, sizeof(algorithm_text)) <= 0 ||
      mg_http_get_var(&request->query, "values", values_text, sizeof(values_text)) <= 0) {
    http_reply_error(c, 400, "Missing required parameters: algorithm and values");
    return;
  }

  int structure = STRUCT_SEQUENTIAL;
  if (mg_http_get_var(&request->query, "structure", structure_text, sizeof(structure_text)) > 0) {
    if (!parse_int_in_range(structure_text, STRUCT_SEQUENTIAL, STRUCT_CIRCULAR, &structure)) {
      http_reply_error(c, 400, "Invalid structure parameter");
      return;
    }
  }

  int algorithm = 0;
  int32_t *values = NULL;
  int count = 0;

  if (!parse_int_in_range(algorithm_text, ALGORITHM_BUBBLE, ALGORITHM_MERGE, &algorithm) ||
      !parse_values_dynamic(values_text, &values, &count)) {
    free(values);
    http_reply_error(c, 400, "Invalid parameters");

    return;
  }

  if (!sort_run(values, count, algorithm, structure, SORT_OPERATION_LIMIT)) {
    free(values);
    http_reply_error(c, 500, "Failed to run sorting algorithm");

    return;
  }

  uint64_t test_id = history_store_record(values, sort_values(), sort_metrics(), sort_operations());
  char *body = json_serialize_sort_result(test_id, values, sort_values(), sort_metrics(), sort_operations());

  free(values);
  sort_free();

  if (!body) {
    http_reply_error(c, 500, "Failed to serialize sorting history");

    return;
  }

  http_reply_json(c, 200, body);
  free(body);
}

static void handle_api_batch(struct mg_connection *c, struct mg_http_message *request) {
  if (!http_is_method(request, "GET")) {
    http_reply_error(c, 405, "Method Not Allowed: use GET");
    return;
  }

  char algorithms_text[128] = {0};
  char structures_text[128] = {0};
  char values_text[MAX_QUERY] = {0};

  mg_http_get_var(&request->query, "algorithms", algorithms_text, sizeof(algorithms_text));
  
  mg_http_get_var(&request->query, "structures", structures_text, sizeof(structures_text));

  if (mg_http_get_var(&request->query, "values", values_text, sizeof(values_text)) <= 0) {
    http_reply_error(c, 400, "Missing values parameter for batch execution");
    return;
  }

  int32_t *values = NULL;

  int count = 0;

  if (!parse_values_dynamic(values_text, &values, &count)) {
    http_reply_error(c, 400, "Invalid values sequence for batch execution");
    return;
  }

  SequentialList *algorithm_ids = seq_list_create(sizeof(int));
  SequentialList *structure_ids = seq_list_create(sizeof(int));

  if (!algorithm_ids || !structure_ids ||
      !parse_id_list(algorithms_text, ALGORITHM_BUBBLE, ALGORITHM_MERGE, algorithm_ids) ||
      !parse_id_list(structures_text, STRUCT_SEQUENTIAL, STRUCT_CIRCULAR, structure_ids)) {
    free(values);
    seq_list_destroy(algorithm_ids);
    seq_list_destroy(structure_ids);
    http_reply_error(c, 400, "Invalid algorithms or structures list");
    return;
  }

  char *body = NULL;

  BatchStatus status = batch_runner_execute(values, count, algorithm_ids, structure_ids, SORT_OPERATION_LIMIT, &body);

  free(values);
  seq_list_destroy(algorithm_ids);
  seq_list_destroy(structure_ids);

  if (status == BATCH_ERR_ALLOC) {
    http_reply_error(c, 500, "Failed to allocate batch queue");
    return;
  }

  if (status == BATCH_ERR_ENQUEUE) {
    http_reply_error(c, 500, "Failed to enqueue batch task");
    return;
  }

  if (status != BATCH_OK || !body) {
    http_reply_error(c, 500, "Failed to serialize batch results");
    return;
  }

  http_reply_json(c, 200, body);
  free(body);
}

static void handle_api_history(struct mg_connection *c, struct mg_http_message *request) {
  if (!http_is_method(request, "GET")) {
    http_reply_error(c, 405, "Method Not Allowed: use GET");
    return;
  }

  char algorithm_filter[64] = {0};

  mg_http_get_var(&request->query, "algorithm", algorithm_filter, sizeof(algorithm_filter));

  const char *lookup_key = resolve_algorithm_filter(algorithm_filter);

  SequentialList *reports = history_store_search(lookup_key);

  if (!reports) {
    http_reply_error(c, 500, "Failed to query Hash Table");
    return;
  }

  HistoryStats stats = history_store_stats();

  char *body = json_serialize_history(&stats, reports);

  seq_list_destroy(reports);

  if (!body) {
    http_reply_error(c, 500, "Failed to serialize history response");
    return;
  }

  http_reply_json(c, 200, body);
  free(body);
}

static void handle_api_history_detail(struct mg_connection *c, struct mg_http_message *request) {
  if (!http_is_method(request, "GET")) {
    http_reply_error(c, 405, "Method Not Allowed: use GET");
    return;
  }

  char id_text[32] = {0};

  if (mg_http_get_var(&request->query, "id", id_text, sizeof(id_text)) <= 0) {
    http_reply_error(c, 400, "Missing id parameter");
    return;
  }

  uint64_t test_id = 0;

  if (!parse_test_id(id_text, &test_id)) {
    http_reply_error(c, 400, "Invalid test id");
    return;
  }

  const TestReport *report = history_store_get_by_id(test_id);

  if (!report) {
    http_reply_error(c, 404, "Test report not found in Hash Table");
    return;
  }

  char *body = json_serialize_sort_result(report->test_id, report->input_values, report->sorted_values, &report->metrics, report->operations);
  
  if (!body) {
    http_reply_error(c, 500, "Failed to serialize test detail");
    return;
  }

  http_reply_json(c, 200, body);
  free(body);
}

static void handle_api_history_clear(struct mg_connection *c, struct mg_http_message *request) {
  if (!http_is_method(request, "DELETE")) {
    http_reply_error(c, 405, "Method Not Allowed: use DELETE");
    return;
  }

  history_store_clear();

  HistoryStats stats = history_store_stats();

  char *body = json_serialize_history_clear(&stats);

  if (!body) {
    http_reply_error(c, 500, "Failed to serialize history clear response");
    return;
  }

  http_reply_json(c, 200, body);
  free(body);
}

static void event_handler(struct mg_connection *c, int event, void *event_data) {
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

  if (!http_serve_static_route(c, request)) {
    http_reply_error(c, 404, "Route not found");
  }
}

int server_run(const char *address) {
  const char *listen_address = (address && address[0] != '\0') ? address : SERVER_DEFAULT_ADDRESS;

  if (!history_store_init()) {
    fprintf(stderr, "Failed to initialize in-memory Hash Table\n");
    return 1;
  }

  struct mg_mgr manager;
  mg_mgr_init(&manager);

  if (!mg_http_listen(&manager, listen_address, event_handler, NULL)) {
    fprintf(stderr, "Could not start server on %s\n", listen_address);
    mg_mgr_free(&manager);
    history_store_destroy();
    return 1;
  }

  printf("Ordena Server listening on %s\n", listen_address);

  for (;;) {
    mg_mgr_poll(&manager, 1000);
  }

  history_store_destroy();
  return 0;
}
