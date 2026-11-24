# Ordena — Data Structures & Sorting Benchmark Platform

Web-based visual laboratory and benchmarking platform for linear data structures, sorting algorithms, dynamic memory management, batch processing queues, step-by-step undo/redo stacks, and separate-chaining hash tables.

All sorting algorithms, dynamic linear data structures, batch queues, and hash table storage are implemented in **C11** using explicit dynamic heap allocation (`malloc`, `realloc`, `free`) and opaque Abstract Data Types (ADTs / TADs). An embedded **Mongoose** HTTP server exposes a REST JSON API and serves the interactive browser visualization and benchmarking suite.

---

## 1. System Architecture & Directory Layout

The system is a single native binary: a C11 backend (HTTP server, sorting engine, ADTs) that also serves a browser frontend. See [ARCHITECTURE.md](ARCHITECTURE.md) for diagrams and design decisions, and [REQUIREMENTS.md](REQUIREMENTS.md) for the requirements specification.

```text
.
├── index.html                              # Web UI (Visualizer, Batch Queue & Hash History)
├── Makefile                                # Cross-platform GNU Make build configuration
├── README.md                               # Project documentation (English)
├── REQUIREMENTS.md                         # Functional and non-functional requirements
├── ARCHITECTURE.md                         # Layers, modules, data flow, design decisions
├── AI_LOG.md                               # Academic integrity & AI adaptation log
├── web/
│   ├── app.js                              # Frontend controller + JavaScript Stack ADT (Undo/Redo)
│   └── style.css                           # Responsive dark-theme styling
├── vendor/
│   ├── mongoose.h                          # Mongoose embedded HTTP library header
│   └── mongoose.c                          # Mongoose embedded HTTP library implementation
└── src/
    ├── main.c                              # Minimal application entrypoint
    ├── server.h / server.c                 # HTTP router and endpoint handlers
    ├── sort_engine.h / sort_engine.c       # Benchmark engine, time/memory metrics, operation log
    ├── tad/
    │   ├── headerFiles/                    # Public ADT interfaces (opaque types)
    │   │   ├── sequential_list.h           # Dynamic contiguous array (malloc/realloc/free)
    │   │   ├── sequential_list_sort.h      # The 5 sorting algorithms for the array
    │   │   ├── singly_linked_list.h        # Singly linked list with pointer-based sorting
    │   │   ├── doubly_linked_list.h        # Doubly linked list with prev/next pointer sorting
    │   │   ├── circular_linked_list.h      # Circular linked list with ring pointer sorting
    │   │   ├── list_sort_observer.h        # Callbacks (compare/swap/write) used by every sort
    │   │   ├── queue.h                     # Dynamic linked queue for batch processing
    │   │   └── hash_table.h                # Dynamic hash table (separate chaining)
    │   └── sourceFiles/                    # ADT implementations (.c)
    └── utils/
        ├── headerFiles/                    # Helper interfaces
        │   ├── input_parser.h              # Query parameter parsing and validation
        │   ├── http_response.h             # JSON/error replies and static file routes
        │   ├── json_builder.h              # Growable JSON string buffer
        │   ├── json_serializer.h           # Results, history and batch JSON output
        │   ├── history_store.h             # Facade over the global hash table
        │   └── batch_runner.h              # Queue-based batch orchestration
        └── sourceFiles/                    # Helper implementations (.c)
```

## 2. Implemented Data Structures (ADTs / TADs)

Every C ADT in `src/tad/` follows the course encapsulation standard: `struct` definitions are private inside `.c` files and exposed in `.h` files via opaque pointers (`typedef struct ...`).

| Data Structure | Implementation File(s) | Memory Layout & Role |
| :--- | :--- | :--- |
| **Sequential List (Dynamic Array)** | `src/tad/sourceFiles/sequential_list.c` | Contiguous heap block allocated with `malloc` and expanded via `realloc`. Used both as a sortable contiguous structure and as the dynamic operation history buffer. |
| **Singly Linked List** | `src/tad/sourceFiles/singly_linked_list.c` | Dynamically allocated `SinglyNode` chain (`data`, `next`). Implements all 5 sorting algorithms by relinking node `next` pointers. |
| **Doubly Linked List** | `src/tad/sourceFiles/doubly_linked_list.c` | Dynamically allocated `DoublyNode` chain (`data`, `prev`, `next`). Implements all 5 sorting algorithms manipulating `prev`/`next` links (including backward `prev` traversal in Insertion Sort). |
| **Circular Linked List** | `src/tad/sourceFiles/circular_linked_list.c` | Dynamically allocated `CircularNode` ring maintaining the invariant `tail->next == head` across insertions, removals, and pointer-based sorting. |
| **Stack (Undo / Redo)** | `web/app.js` (`OperationStack`) | Linked `StackNode` ADT in JavaScript (`push`, `pop`, `peek`, `isEmpty`, `size`, `clear`). Powers reversible step-by-step playback (`undoStack` and `redoStack`) without requiring full-array snapshots per step. |
| **Queue (Batch Processing)** | `src/tad/sourceFiles/queue.c` | Dynamic FIFO queue (`QueueNode` allocated with `malloc`/`free`, `front`/`rear` pointers). Enqueues multiple `BatchTask` scenarios and processes them sequentially in `/api/batch`. |
| **Hash Table (Test History)** | `src/tad/sourceFiles/hash_table.c` | Dynamic bucket array (`malloc`/`realloc` when load factor $\alpha > 0.75$) with **Separate Chaining** (`HashNode` linked lists) indexed by `algorithmName` using a DJB2/modulo hash function. Stores all executed `TestReport` records in RAM for fast lookup and replay. |

---

## 3. Sorting Algorithms & Complexity Analysis

All five sorting algorithms are implemented across all four linear structures:

| ID | Algorithm | Time (Best) | Time (Average) | Time (Worst) | Auxiliary Memory | Pointer / Structural Notes |
| :---: | :--- | :---: | :---: | :---: | :---: | :--- |
| `0` | **Bubble Sort** | $O(n)$ | $O(n^2)$ | $O(n^2)$ | $O(1)$ | Adjacent elements swapped by index (`SequentialList`) or adjacent node pointer relinking (`Linked Lists`). |
| `1` | **Selection Sort** | $O(n^2)$ | $O(n^2)$ | $O(n^2)$ | $O(1)$ | Scans for minimum element and swaps/relinks the minimum node into position. |
| `2` | **Insertion Sort** | $O(n)$ | $O(n^2)$ | $O(n^2)$ | $O(1)$ | Shifts elements in contiguous arrays or detaches and splices nodes directly into the sorted linked prefix. |
| `3` | **Quick Sort** | $O(n \log n)$ | $O(n \log n)$ | $O(n^2)$ | $O(\log n)$ | Partitions around the last element and recursively sorts subranges. |
| `4` | **Merge Sort** | $O(n \log n)$ | $O(n \log n)$ | $O(n \log n)$ | $O(n)$ (Array) / $O(1)$ heap (Lists) | `SequentialList` allocates a dynamic `merge_buffer` (`n * sizeof(int32_t)`), whereas Linked Lists split sublists and merge in-place by relinking node pointers with zero auxiliary heap array. |

### Memory Usage Tracking (`memoryBytes`)
- **Sequential List**: `sizeof(SequentialList) + capacity * sizeof(int32_t)` (+ `n * sizeof(int32_t)` auxiliary buffer during Merge Sort).
- **Singly Linked List**: `sizeof(SinglyLinkedList) + n * sizeof(SinglyNode)`.
- **Doubly Linked List**: `sizeof(DoublyLinkedList) + n * sizeof(DoublyNode)`.
- **Circular Linked List**: `sizeof(CircularLinkedList) + n * sizeof(CircularNode)`.

---

## 4. Build & Run Instructions

### Prerequisites
- C compiler with C11 support (`gcc` or `clang`)
- GNU Make

### Compiling and Starting the Server

```sh
make rebuild
make serve
```

Open [http://localhost:8000](http://localhost:8000) in your browser.
To listen on a custom address/port, run:

```sh
./build/bin/ordena-server http://127.0.0.1:9000
```

Clean build artifacts with:

```sh
make clean
```

---

## 5. HTTP JSON API Reference

### 5.1. Single Sort & Benchmark
`GET /api/sort?algorithm=<0..4>&structure=<0..3>&values=<csv>`

Executes the selected sorting algorithm on the chosen linear structure, stores the complete report in the in-memory **Hash Table**, and returns the execution trace.
- `algorithm`: `0` Bubble, `1` Selection, `2` Insertion, `3` Quick, `4` Merge
- `structure`: `0` Sequential List, `1` Singly Linked List, `2` Doubly Linked List, `3` Circular Linked List (defaults to `0` if omitted)
- `values`: 2 to 128 positive integers (`1..1000000`) separated by comma, space, or semicolon

Example:
```sh
curl 'http://localhost:8000/api/sort?algorithm=4&structure=2&values=8,3,6,1,9,2'
```

### 5.2. Batch Benchmark Processing (Queue ADT)
`GET /api/batch?algorithms=<csv>&structures=<csv>&values=<csv>`

Enqueues all combinations of `(algorithm, structure)` into the dynamic C `Queue`, dequeues and executes each benchmark in FIFO order, registers every report in the `HashTable`, and returns a comparative JSON summary including `queueTasksProcessed` and `queuePeakMemoryBytes`.

Example:
```sh
curl 'http://localhost:8000/api/batch?algorithms=0,4&structures=0,1,2,3&values=8,3,6,1,9,2'
```

### 5.3. Query Stored History (Hash Table ADT)
`GET /api/history?algorithm=<algorithmName|all>`

Searches the C `HashTable` by `algorithmName` (e.g., `Bubble Sort`, `Merge Sort`, or `all`), returning `tableStats` (`totalTests`, `bucketCount`, `memoryBytes`) and the list of matching test reports.

Example:
```sh
curl 'http://localhost:8000/api/history?algorithm=Merge%20Sort'
```

### 5.4. Retrieve Full Test Report for Replay
`GET /api/history/detail?id=<testId>`

Retrieves a specific `TestReport` by `testId` from the C `HashTable` including its full `operations` array so it can be replayed step-by-step in the visualizer.

### 5.5. Clear Stored History
`DELETE /api/history/clear`

Empties the in-memory `HashTable` and returns the updated `tableStats`. The history is volatile: it is also lost whenever the server stops.

Example:
```sh
curl -X DELETE 'http://localhost:8000/api/history/clear'
```

### 5.6. Errors
`400` invalid or missing parameters, `404` unknown route or test id, `405` wrong HTTP method, `500` allocation or serialization failure.

---

## 6. Project Documentation

| Document | Content |
| :--- | :--- |
| [REQUIREMENTS.md](REQUIREMENTS.md) | Functional and non-functional requirements, constraints, known gaps, traceability to the course description. |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Layered design, module reference, sequence diagrams, design decisions, how to extend the system. |
| [AI_LOG.md](AI_LOG.md) | Main AI prompts and how each answer was adapted or corrected. |
