#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  int algorithm;
  int structure_type;
  int32_t *input_values;
  int item_count;
} BatchTask;

/* Opaque type for Dynamic Linked Queue */
typedef struct queue Queue;

Queue *queue_create(void);
int queue_enqueue(Queue *queue, const BatchTask *task);
int queue_dequeue(Queue *queue, BatchTask *out_task);
int queue_is_empty(const Queue *queue);
int queue_size(const Queue *queue);
size_t queue_memory_usage(const Queue *queue);
void queue_destroy(Queue *queue);
void batch_task_free(BatchTask *task);

#endif
