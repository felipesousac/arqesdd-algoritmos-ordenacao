# Ordena — Data Structures & Sorting Benchmark Platform

Web-based visual laboratory and benchmarking platform for linear data structures, sorting algorithms, dynamic memory management, batch processing queues, step-by-step undo/redo stacks, and separate-chaining hash tables.

All sorting algorithms, dynamic linear data structures, batch queues, and hash table storage are implemented in **C11** using explicit dynamic heap allocation (`malloc`, `realloc`, `free`) and opaque Abstract Data Types (ADTs / TADs). An embedded **Mongoose** HTTP server exposes a REST JSON API and serves the interactive browser visualization and benchmarking suite.

---

## 1. System Architecture & Directory Layout

```text
.
├── index.html                              # Web UI (Visualizer, Batch Queue & Hash History)
├── Makefile                                # Cross-platform GNU Make build configuration
├── README.md                               # Project documentation (English)
├── AI_LOG.md                               # Academic integrity & AI adaptation log
├── web/
│   ├── app.js                              # Frontend controller + JavaScript Stack ADT (Undo/Redo)
│   └── style.css                           # Responsive dark-theme styling
├── vendor/
│   ├── mongoose.h                          # Mongoose embedded HTTP library header
│   └── mongoose.c                          # Mongoose embedded HTTP library implementation
└── src/
    ├── server.c                            # HTTP router, JSON serializer, Queue & Hash Table endpoints
    ├── sort_engine.h                       # Public sorting & benchmarking engine interface
    ├── sort_engine.c                       # Unified benchmark engine & memory/time instrumentation
    └── tad/
        ├── headerFiles/
        │   ├── sequential_list.h           # Dynamic Contiguous Array ADT (malloc/realloc/free)
        │   ├── singly_linked_list.h        # Singly Linked List ADT with pointer-based sorting
        │   ├── doubly_linked_list.h        # Doubly Linked List ADT with bidirectional pointer sorting
        │   ├── circular_linked_list.h      # Circular Linked List ADT with ring pointer sorting
        │   ├── queue.h                     # Dynamic Linked Queue ADT for batch processing
        │   └── hash_table.h                # Dynamic Hash Table ADT (Separate Chaining)
        └── sourceFiles/
            ├── sequential_list.c
            ├── singly_linked_list.c
            ├── doubly_linked_list.c
            ├── circular_linked_list.c
            ├── queue.c
            └── hash_table.c
```

---

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
`POST /api/batch?algorithms=<csv|all>&structures=<csv|all>&values=<csv>` (also accepts `GET`)

Enqueues all combinations of `(algorithm, structure)` into the dynamic C `Queue`, dequeues and executes each benchmark in FIFO order, registers every report in the `HashTable`, and returns a comparative JSON summary including `queueTasksProcessed` and `queuePeakMemoryBytes`.

Example:
```sh
curl -X POST 'http://localhost:8000/api/batch?algorithms=0,4&structures=0,1,2,3&values=8,3,6,1,9,2'
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
