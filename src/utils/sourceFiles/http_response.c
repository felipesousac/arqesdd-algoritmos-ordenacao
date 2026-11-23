#include "../headerFiles/http_response.h"

#define JSON_HEADERS "Content-Type: application/json; charset=utf-8\r\n"

int http_is_method(const struct mg_http_message *request, const char *method) {
  return mg_strcmp(request->method, mg_str(method)) == 0;
}

void http_reply_json(struct mg_connection *c, int status, const char *body) {
  mg_http_reply(c, status, JSON_HEADERS, "%s", body);
}

void http_reply_error(struct mg_connection *c, int status, const char *message) {
  mg_http_reply(c, status, JSON_HEADERS, "{\"error\":\"%s\"}\n", message);
}

int http_serve_static_route(struct mg_connection *c,
                            struct mg_http_message *request) {
  if (!http_is_method(request, "GET")) {
    return 0;
  }

  const char *path = NULL;

  if (mg_match(request->uri, mg_str("/"), NULL) ||
      mg_match(request->uri, mg_str("/index.html"), NULL)) {
    path = "index.html";
  } else if (mg_match(request->uri, mg_str("/web/app.js"), NULL)) {
    path = "web/app.js";
  } else if (mg_match(request->uri, mg_str("/web/style.css"), NULL)) {
    path = "web/style.css";
  }

  if (!path) {
    return 0;
  }

  struct mg_http_serve_opts options = {.fs = &mg_fs_posix};
  mg_http_serve_file(c, request, path, &options);
  return 1;
}
