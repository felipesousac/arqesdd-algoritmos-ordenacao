# AI Usage & Engineering Adaptation Log (`AI_LOG.md`)

This document records the prompts, architectural decisions, and manual/guided adaptations made using AI assistance during the development of the **Ordena — Data Structures & Sorting Benchmark Platform** for the Data Structures course (`ARQESDD` — IFSP Araraquara).

---

## 1. Summary of Prompts & Development Stages

### Prompt 1 — Gap Analysis & Architecture Planning
> *"Analyze the current Ordena repository against the Final Assessment requirements and our reference ADT implementations (`lista.h`/`lista.c` and `tabela_hash.c`). Create an implementation plan (`plan.md`) covering dynamic memory allocation (`malloc`/`realloc`/`free`), Singly/Doubly/Circular Linked Lists, Stack for step-by-step Undo in JS, Queue for Batch Processing in C, and Separate-Chaining Hash Table in C."*

- **Outcome**: Produced `plan.md` mapping the gap between the initial static-array prototype and the complete data-structures platform.

### Prompt 2 — Dynamic Linear ADTs & Pointer-Based Sorting (`src/tad/`)
> *"Read `plan.md` at the project root and execute its instructions: refactor `listaSequencial` to `sequential_list` in English, implement opaque-type ADTs for `singly_linked_list`, `doubly_linked_list`, and `circular_linked_list`, and implement the 5 sorting algorithms manipulating node pointers (`next`/`prev`)."*

- **Adaptations Made to Match Course ADT Standards**:
  1. **Opaque Types (`typedef struct ...`)**: Following the course reference `lista.h`/`lista.c`, internal node structures (`SinglyNode`, `DoublyNode`, `CircularNode`) and list descriptors (`struct singly_linked_list`, etc.) are strictly encapsulated inside their respective `.c` files in `src/tad/sourceFiles/`, while headers in `src/tad/headerFiles/` expose only opaque types and public operations.
  2. **Pointer Manipulation Over Value Swapping**:
     - In `singly_linked_list.c`, `doubly_linked_list.c`, and `circular_linked_list.c`, node exchanges and insertions re-link actual heap nodes (`prev`, `next`, `head`, `tail`, and `tail->next = head` for circular lists).
     - In `doubly_linked_list.c`, **Insertion Sort** traverses backwards directly via `scan = scan->prev` and splices the detached node into place.
     - In all three linked list ADTs, **Merge Sort** splits sublists and merges them in-place by relinking node pointers (`dummy.next` / `tail->next`) with zero auxiliary heap array allocation, contrasting with the dynamic `merge_buffer` required by `SequentialList`.
  3. **Observer Callback Pattern (`ListSortObserver`)**: Added lightweight callbacks (`on_compare`, `on_swap`, `on_write`) so pointer operations inside encapsulated ADTs report step-by-step visualization events and metrics to `sort_engine.c` without breaking ADT encapsulation.

### Prompt 3 — Batch Queue ADT & Separate-Chaining Hash Table ADT in C
> *"Implement the dynamic `Queue` ADT (`queue.h/.c`) for batch benchmark processing and the dynamic `HashTable` ADT (`hash_table.h/.c`) with Separate Chaining indexed by `algorithmName`."*

- **Adaptations Made to Match Course ADT Standards**:
  1. **Queue ADT (`queue.c`)**: Implemented using dynamically allocated `QueueNode` structs (`front` and `rear` pointers), deep-copying each task's `input_values` via `malloc` on `queue_enqueue` and releasing node memory on `queue_dequeue`.
  2. **Hash Table ADT (`hash_table.c`)**: Adapted the modular hash table pattern (`tabela_hash.c`) from static open addressing to **100% dynamic Separate Chaining**:
     - Bucket array (`HashNode **buckets`) is allocated with `malloc` and dynamically resized with `realloc` (`hash_table_resize`) whenever the load factor $\alpha = \text{count} / M > 0.75$.
     - Collisions and multiple benchmark runs for the same `algorithmName` are chained in linked lists (`HashNode *next`) hanging off `buckets[hash(algorithm_name) % M]`.

### Prompt 4 — Frontend Stack ADT (`OperationStack`) & UI Integration
> *"Implement the `OperationStack` ADT in `web/app.js` for reversible step-by-step Undo/Redo playback, and add UI controls for selecting linear structures, viewing dynamic memory usage (`memoryBytes`), running batch benchmarks (`/api/batch`), and querying/replaying historical tests from the C Hash Table (`/api/history`)."*

- **Adaptations Made**:
  1. Replaced static frame-array indexing with two linked-node stack instances (`undoStack` and `redoStack` of class `OperationStack` using `StackNode`).
  2. Each forward step pushes a reversible operation record (`prevFirstValue`, `prevSecondValue`, previous metrics) onto `undoStack`; clicking **Step Back (`‹`)** pops from `undoStack`, surgically reverts the change in `displayedValues`, and pushes the record onto `redoStack`.

---

## 2. Verification & Quality Assurance
- Compiled with `gcc -O2 -std=c11 -D_GNU_SOURCE -Wall -Wextra` via `make rebuild` with **zero warnings**.
- Verified all 20 combinations of `(5 algorithms × 4 linear data structures)` for sorting correctness, operation trace accuracy, and dynamic memory cleanup (`free`).
