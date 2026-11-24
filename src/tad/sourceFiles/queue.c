#include "../headerFiles/queue.h"
#include <stdlib.h>
#include <string.h>

typedef struct queue_node {
  BatchTask task;
  struct queue_node *next;
} QueueNode;

struct queue {
  QueueNode *front;
  QueueNode *rear;
  int size;
  size_t memory_bytes;
};

Queue *queue_create(void) {
  Queue *queue = (Queue *)malloc(sizeof(Queue));
  if (queue == NULL) {
    return NULL;
  }

  queue->front = NULL;
  queue->rear = NULL;
  queue->size = 0;
  queue->memory_bytes = sizeof(Queue);
  return queue;
}

int queue_enqueue(Queue *queue, const BatchTask *task) {
  if (queue == NULL || task == NULL || task->input_values == NULL ||
      task->item_count < 1) {
    return 0;
  }

  QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));

  if (node == NULL) {
    return 0;
  }

  size_t values_bytes = (size_t)task->item_count * sizeof(int32_t);
  node->task.input_values = (int32_t *)malloc(values_bytes);

  if (node->task.input_values == NULL) {
    free(node);
    return 0;
  }

  memcpy(node->task.input_values, task->input_values, values_bytes);
  node->task.algorithm = task->algorithm;
  node->task.structure_type = task->structure_type;
  node->task.item_count = task->item_count;
  node->next = NULL;

  if (queue->rear == NULL) {
    queue->front = node;
    queue->rear = node;
  } else {
    queue->rear->next = node;
    queue->rear = node;
  }

  queue->size++;
  queue->memory_bytes += sizeof(QueueNode) + values_bytes;

  return 1;
}

int queue_dequeue(Queue *queue, BatchTask *out_task) {
  if (queue == NULL || out_task == NULL || queue->front == NULL) {
    return 0;
  }

  QueueNode *removed = queue->front;
  *out_task = removed->task;

  queue->front = removed->next;
  if (queue->front == NULL) {
    queue->rear = NULL;
  }

  size_t values_bytes = (size_t)removed->task.item_count * sizeof(int32_t);
  queue->size--;
  queue->memory_bytes -= (sizeof(QueueNode) + values_bytes);
  free(removed);
  return 1;
}

int queue_is_empty(const Queue *queue) {
  if (queue == NULL) {
    return 1;
  }
  return queue->front == NULL;
}

int queue_size(const Queue *queue) {
  if (queue == NULL) {
    return 0;
  }

  return queue->size;
}

size_t queue_memory_usage(const Queue *queue) {
  if (queue == NULL) {
    return 0;
  }
  
  return queue->memory_bytes;
}

void batch_task_free(BatchTask *task) {
  if (task != NULL) {
    free(task->input_values);
    task->input_values = NULL;
    task->item_count = 0;
  }
}

void queue_destroy(Queue *queue) {
  if (queue == NULL) {
    return;
  }

  BatchTask task;
  while (queue_dequeue(queue, &task)) {
    batch_task_free(&task);
  }
  free(queue);
}
