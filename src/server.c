#include "mongoose.h"
#include "sort_engine.h"
#include "tad/headerFiles/listaSequencial.h"
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define API_MAX_INPUT 128
#define MAX_QUERY 4096
#define MAX_JSON (32 * 1024 * 1024)

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

    char *data = realloc(json->data, capacity);
    if (!data) {
      json->failed = 1;
      va_end(args);
      return;
    }

    json->data = data;
    json->capacity = capacity;
  }

  vsnprintf(json->data + json->length, json->capacity - json->length,
            format, args);
  json->length += (size_t)needed;
  va_end(args);
}

static int parse_values(char *text, int32_t *values, int *count) {
  *count = 0;
  char *cursor = text;

  while (*cursor) {
    while (*cursor == ' ' || *cursor == ',' || *cursor == ';') {
      cursor++;
    }
    if (!*cursor) {
      break;
    }
    if (*count >= API_MAX_INPUT) {
      return 0;
    }

    errno = 0;
    char *end;
    long value = strtol(cursor, &end, 10);
    if (end == cursor || errno || value < 1 || value > 1000000) {
      return 0;
    }

    values[(*count)++] = (int32_t)value;
    if (*end && *end != ' ' && *end != ',' && *end != ';') {
      return 0;
    }
    cursor = end;
  }

  return *count >= 2;
}

static const char *algorithm_name(int algorithm) {
  static const char *names[] = {
      "Bubble Sort", "Selection Sort", "Insertion Sort", "Quick Sort",
      "Merge Sort"};
  return algorithm >= 0 && algorithm <= 4 ? names[algorithm] : "";
}

static char *build_history(const int32_t *input, const int32_t *sorted,
                           const Metrics *metrics,
                           const Operation *operations) {
  Json json = {0};
  Lista *history = criaLista(sizeof(Operation));
  if (!history) {
    return NULL;
  }

  for (int i = 0; i < metrics->operation_count; i++) {
    if (!insereFinal(history, &operations[i])) {
      json.failed = 1;
      break;
    }
  }

  json_append(&json,
              "{\"algorithm\":%d,\"algorithmName\":\"%s\",\"input\":[",
              metrics->algorithm, algorithm_name(metrics->algorithm));
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
              "\"elapsedMs\":%.6f,\"operationCount\":%d},"
              "\"operations\":[",
              metrics->item_count, (long long)metrics->comparisons,
              (long long)metrics->swaps, metrics->elapsed_ms,
              metrics->operation_count);

  for (int i = 0; i < tamLista(history); i++) {
    Operation operation;
    if (!consultaPosicao(history, i, &operation)) {
      json.failed = 1;
      break;
    }

    Operation *op = &operation;
    json_append(
        &json,
        "%s{\"type\":%d,\"firstIndex\":%d,"
        "\"secondIndex\":%d,\"firstValue\":%d,"
        "\"secondValue\":%d}",
        i ? "," : "", op->type, op->first_index, op->second_index,
        op->first_value, op->second_value);
  }

  json_append(&json, "]}");
  apagaLista(history);
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

static void event_handler(struct mg_connection *c, int event, void *event_data) {
  if (event != MG_EV_HTTP_MSG) {
    return;
  }

  struct mg_http_message *request = event_data;
  if (mg_match(request->uri, mg_str("/api/sort"), NULL)) {
    if (mg_strcmp(request->method, mg_str("GET")) != 0) {
      reply_error(c, 405, "Use GET");
      return;
    }

    char algorithm_text[16];
    char values_text[MAX_QUERY];
    if (mg_http_get_var(&request->query, "algorithm", algorithm_text,
                        sizeof(algorithm_text)) <= 0 ||
        mg_http_get_var(&request->query, "values", values_text,
                        sizeof(values_text)) <= 0) {
      reply_error(c, 400, "Informe algorithm e values");
      return;
    }

    char *end;
    long algorithm = strtol(algorithm_text, &end, 10);
    int32_t values[API_MAX_INPUT];
    int count;
    if (*end || algorithm < 0 || algorithm > 4 ||
        !parse_values(values_text, values, &count)) {
      reply_error(c, 400, "Parâmetros inválidos");
      return;
    }

    if (!sort_run(values, count, (int)algorithm, 2000000)) {
      reply_error(c, 500, "Falha ao ordenar");
      return;
    }

    char *body = build_history(values, sort_values(), sort_metrics(),
                               sort_operations());
    sort_free();
    if (!body) {
      reply_error(c, 500, "Falha ao montar histórico");
      return;
    }

    mg_http_reply(c, 200, "Content-Type: application/json; charset=utf-8\r\n",
                  "%s", body);
    free(body);
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
      reply_error(c, 404, "Rota não encontrada");
    }
  } else {
    reply_error(c, 404, "Rota não encontrada");
  }
}

int main(int argc, char **argv) {
  const char *address = argc > 1 ? argv[1] : "http://0.0.0.0:8000";
  struct mg_mgr manager;
  mg_mgr_init(&manager);
  if (!mg_http_listen(&manager, address, event_handler, NULL)) {
    fprintf(stderr, "Não foi possível iniciar o servidor em %s\n", address);
    mg_mgr_free(&manager);
    return 1;
  }

  printf("Servidor Ordena em %s\n", address);
  for (;;) {
    mg_mgr_poll(&manager, 1000);
  }
}
