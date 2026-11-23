#ifndef BATCH_RUNNER_H
#define BATCH_RUNNER_H

#include "../../tad/headerFiles/sequential_list.h"
#include <stdint.h>

typedef enum {
  BATCH_OK = 0,
  BATCH_ERR_ALLOC = 1,
  BATCH_ERR_ENQUEUE = 2,
  BATCH_ERR_SERIALIZE = 3
} BatchStatus;

BatchStatus batch_runner_execute(const int32_t *values, int count,
                                 const SequentialList *alg_ids,
                                 const SequentialList *struct_ids,
                                 int operation_limit, char **out_json);

#endif
