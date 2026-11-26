#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "../../../vendor/mongoose.h"

int http_is_method(const struct mg_http_message *request, const char *method);
void http_reply_json(struct mg_connection *c, int status, const char *body);
void http_reply_error(struct mg_connection *c, int status, const char *message);
int http_serve_static_route(struct mg_connection *c, struct mg_http_message *request);

#endif
