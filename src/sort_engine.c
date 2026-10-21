#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
static double now_ms(void) { return emscripten_get_now(); }
#else
#define EXPORT
static double now_ms(void) { return (double)clock() * 1000.0 / CLOCKS_PER_SEC; }
#endif

enum { COMPARE = 0, SWAP = 1, WRITE = 2 };
typedef struct { int32_t type, i, j, a, b; } Operation;
typedef struct {
  int32_t algorithm, n;
  int64_t comparisons, swaps;
  double elapsed_ms;
  int32_t op_count;
} Metrics;

#define MAX_ITEMS 4096
#define MAX_OPS 2000000
static int32_t values[MAX_ITEMS], scratch[MAX_ITEMS];
static Operation *ops;
static int op_count, op_capacity;
static Metrics metrics;
static int op_limit = MAX_OPS;

static void record(int type, int i, int j, int a, int b) {
  if (op_count >= op_limit) return;
  if (op_count == op_capacity) {
    int next = op_capacity ? op_capacity * 2 : 1024;
    if (next > op_limit) next = op_limit;
    Operation *p = (Operation *)realloc(ops, (size_t)next * sizeof(Operation));
    if (!p) return;
    ops = p; op_capacity = next;
  }
  ops[op_count++] = (Operation){type, i, j, a, b};
}
static int less_at(int i, int j) {
  metrics.comparisons++;
  record(COMPARE, i, j, values[i], values[j]);
  return values[i] < values[j];
}
static void swap_at(int i, int j) {
  if (i == j) return;
  int a = values[i], b = values[j];
  values[i] = b; values[j] = a; metrics.swaps++;
  record(SWAP, i, j, b, a);
}
static void write_at(int i, int value) {
  values[i] = value;
  record(WRITE, i, -1, value, 0);
}

static void bubble(void) {
  for (int end = metrics.n - 1; end > 0; --end) {
    int changed = 0;
    for (int i = 0; i < end; ++i) if (less_at(i + 1, i)) { swap_at(i, i + 1); changed = 1; }
    if (!changed) break;
  }
}
static void selection(void) {
  for (int i = 0; i < metrics.n - 1; ++i) {
    int min = i;
    for (int j = i + 1; j < metrics.n; ++j) if (less_at(j, min)) min = j;
    swap_at(i, min);
  }
}
static void insertion(void) {
  for (int i = 1; i < metrics.n; ++i) {
    int key = values[i], j = i - 1;
    while (j >= 0) { metrics.comparisons++; record(COMPARE, j, i, values[j], key); if (values[j] <= key) break; write_at(j + 1, values[j]); --j; }
    if (j + 1 != i) write_at(j + 1, key);
  }
}
static int partition(int low, int high) {
  int pivot = values[high], i = low;
  for (int j = low; j < high; ++j) {
    metrics.comparisons++; record(COMPARE, j, high, values[j], pivot);
    if (values[j] <= pivot) swap_at(i++, j);
  }
  swap_at(i, high); return i;
}
static void quick(int low, int high) {
  while (low < high) {
    int p = partition(low, high);
    if (p - low < high - p) { quick(low, p - 1); low = p + 1; }
    else { quick(p + 1, high); high = p - 1; }
  }
}
static void merge_range(int lo, int mid, int hi) {
  int i = lo, j = mid, k = lo;
  while (i < mid && j < hi) {
    metrics.comparisons++; record(COMPARE, i, j, values[i], values[j]);
    scratch[k++] = values[i] <= values[j] ? values[i++] : values[j++];
  }
  while (i < mid) scratch[k++] = values[i++];
  while (j < hi) scratch[k++] = values[j++];
  for (k = lo; k < hi; ++k) write_at(k, scratch[k]);
}
static void merge_sort(int lo, int hi) {
  if (hi - lo < 2) return;
  int mid = lo + (hi - lo) / 2;
  merge_sort(lo, mid); merge_sort(mid, hi); merge_range(lo, mid, hi);
}

EXPORT int sort_run(const int32_t *input, int n, int algorithm, int limit) {
  if (!input || n < 1 || n > MAX_ITEMS || algorithm < 0 || algorithm > 4) return 0;
  memcpy(values, input, (size_t)n * sizeof(int32_t));
  free(ops); ops = NULL; op_count = op_capacity = 0;
  op_limit = limit > 0 && limit < MAX_OPS ? limit : MAX_OPS;
  memset(&metrics, 0, sizeof(metrics)); metrics.algorithm = algorithm; metrics.n = n;
  double start = now_ms();
  switch (algorithm) { case 0: bubble(); break; case 1: selection(); break; case 2: insertion(); break; case 3: quick(0, n - 1); break; case 4: merge_sort(0, n); break; }
  metrics.elapsed_ms = now_ms() - start; metrics.op_count = op_count;
  return 1;
}
EXPORT int32_t *sort_values(void) { return values; }
EXPORT Metrics *sort_metrics(void) { return &metrics; }
EXPORT Operation *sort_operations(void) { return ops; }
EXPORT void sort_free(void) { free(ops); ops = NULL; op_count = op_capacity = 0; }
