#include "../headerFiles/batch_runner.h"
#include "../../sort_engine.h"
#include "../../tad/headerFiles/queue.h"
#include "../headerFiles/history_store.h"
#include "../headerFiles/json_builder.h"
#include "../headerFiles/json_serializer.h"
#include <stddef.h>

BatchStatus batch_runner_execute(const int32_t *values, int count,
                                 const SequentialList *alg_ids,
                                 const SequentialList *struct_ids,
                                 int operation_limit, char **out_json) {
  *out_json = NULL;

  Queue *batch_queue = queue_create();

  if (!batch_queue) {
    return BATCH_ERR_ALLOC;
  }

  /* Enqueue all tasks in the Dynamic Queue TAD */
  for (int i = 0; i < seq_list_size(alg_ids); i++) {
    int alg = 0;

    seq_list_get(alg_ids, i, &alg); // Replace alg to current alg_id in iteration

    for (int j = 0; j < seq_list_size(struct_ids); j++) {
      int st = 0;
      seq_list_get(struct_ids, j, &st); // // Replace st to current struct_id in iteration

      BatchTask task = {.algorithm = alg,
                        .structure_type = st,
                        .input_values = (int32_t *) values,
                        .item_count = count};

      if (!queue_enqueue(batch_queue, &task)) {
        queue_destroy(batch_queue);
        return BATCH_ERR_ENQUEUE;
      }
    }
  }

  int total_tasks = queue_size(batch_queue);
  size_t queue_peak_memory = queue_memory_usage(batch_queue);

  JsonBuilder json;
  json_builder_init(&json);
  json_append_batch_header(&json, total_tasks, queue_peak_memory, count);

  int result_index = 0;

  BatchTask task;

  while (queue_dequeue(batch_queue, &task)) {
    if (sort_run(task.input_values, task.item_count, task.algorithm, task.structure_type, operation_limit)) {
      const Metrics *metrics = sort_metrics();

      uint64_t test_id = history_store_record(task.input_values, sort_values(), metrics, sort_operations());

      json_append_batch_item(&json, result_index, test_id, metrics);

      result_index++;

      sort_free();
    }

    batch_task_free(&task);
  }

  queue_destroy(batch_queue);
  json_append_batch_footer(&json);

  char *serialized = json_builder_detach(&json);

  if (!serialized) {
    return BATCH_ERR_SERIALIZE;
  }

  *out_json = serialized;

  return BATCH_OK;
}
