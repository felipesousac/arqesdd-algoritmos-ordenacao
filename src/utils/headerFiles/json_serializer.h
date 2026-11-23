#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include "../../sort_engine.h"
#include "../../tad/headerFiles/sequential_list.h"
#include "history_store.h"
#include "json_builder.h"
#include <stddef.h>
#include <stdint.h>

char *json_serialize_sort_result(uint64_t test_id, const int32_t *input,
                                 const int32_t *sorted, const Metrics *metrics,
                                 const Operation *operations);
char *json_serialize_history(const HistoryStats *stats,
                             const SequentialList *reports);
char *json_serialize_history_clear(const HistoryStats *stats);

void json_append_batch_header(JsonBuilder *json, int total_tasks,
                              size_t queue_peak_memory, int item_count);
void json_append_batch_item(JsonBuilder *json, int index, uint64_t test_id,
                            const Metrics *metrics);
void json_append_batch_footer(JsonBuilder *json);

#endif
