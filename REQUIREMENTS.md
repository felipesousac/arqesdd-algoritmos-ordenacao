# Ordena — Software Requirements Specification

This document lists the functional and non-functional requirements of **Ordena**, the Data Structures & Sorting Benchmark Platform built for the course final project. Each requirement has a stable ID so it can be referenced in commits, issues and pull requests.

- **Status** values: `Done` (implemented and present in the repository) or `Gap` (not implemented or only partially covered, see [Section 6](#6-known-gaps-and-deviations)).
- For how the requirements are realized in code, see [ARCHITECTURE.md](ARCHITECTURE.md).

---

## 1. Purpose and Scope

Ordena is a laboratory for comparing sorting algorithms over linear data structures. A C11 backend runs the algorithms with explicit dynamic memory management, measures time and memory, and exposes the results as JSON over an embedded HTTP server. A browser frontend (HTML, CSS, JavaScript) animates every comparison and swap and lets the user step through the execution.

**In scope:** sorting benchmarks, step-by-step visualization with undo/redo, batch execution, and an in-memory history of past tests.
**Out of scope:** persistent storage (history lives in RAM and is lost when the server stops), user accounts, and sorting of non-integer data.

## 2. Actors

| Actor | Description |
| :--- | :--- |
| **User** | Person using the web interface to run, watch and compare sorting tests. |
| **Frontend** | Browser application (`index.html`, `web/app.js`, `web/style.css`). |
| **Backend** | C server (`ordena-server`) that executes algorithms and stores results. |

## 3. Functional Requirements

### 3.1 Single sort execution

| ID | Requirement | Status |
| :--- | :--- | :---: |
| FR-01 | The system shall let the user choose one of five algorithms: Bubble, Selection, Insertion, Quick and Merge Sort. | Done |
| FR-02 | The system shall let the user choose one of four linear structures to sort: Sequential List (dynamic array), Singly Linked List, Doubly Linked List and Circular Linked List. | Done |
| FR-03 | The system shall accept an input sequence of 2 to 300 positive integers (`1..1000000`) separated by comma, space or semicolon, and shall reject invalid input with a clear error. | Done |
| FR-04 | The system shall provide helpers to generate input: random integer sequence, ascending sequence and descending sequence. | Done |
| FR-05 | The system shall sort linked lists by relinking node pointers (`next`/`prev`), not by copying values into an array. | Done |
| FR-06 | The system shall record every compare, swap and write operation of an execution so it can be replayed. | Done |

### 3.2 Metrics

| ID | Requirement | Status |
| :--- | :--- | :---: |
| FR-07 | The system shall report, for each execution: number of comparisons, number of swaps, elapsed time in milliseconds and memory consumption in bytes. | Done |
| FR-08 | The system shall report memory per structure type, including the auxiliary buffer used by Merge Sort on the Sequential List. | Done |
| FR-09 | The system shall allow the user to compare the performance of linked lists against the contiguous array for the same algorithm and input. | Done |

### 3.3 Visualization and step-by-step mode

| ID | Requirement | Status |
| :--- | :--- | :---: |
| FR-10 | The frontend shall draw the sequence as bars and animate each compare/swap/write operation returned by the backend. | Done |
| FR-11 | The user shall be able to play, pause and reset the animation and adjust its speed. | Done |
| FR-12 | The user shall be able to move one step forward and one step back, and jump to any step with a timeline slider. | Done |
| FR-13 | **Undo/Redo:** step-back shall be implemented with a Stack ADT. Undone operations shall be moved to a second (redo) stack. Both stack sizes shall be visible in the interface. | Done |

### 3.4 Batch processing

| ID | Requirement | Status |
| :--- | :--- | :---: |
| FR-14 | The system shall run several tests in one request by enqueuing every `(algorithm, structure)` combination in a FIFO Queue ADT and executing them in order. | Done |
| FR-15 | The user shall be able to launch a batch with: all 4 structures for the current algorithm, all 5 algorithms for the current structure, or the full 5 × 4 matrix. | Done |
| FR-16 | The batch response shall include the number of tasks processed, the queue peak memory and one result row (with metrics) per task. | Done |

### 3.5 Test history (Hash Table, extra)

| ID | Requirement | Status |
| :--- | :--- | :---: |
| FR-17 | The system shall store every executed test (single or batch) in a Hash Table indexed by algorithm name, with collisions handled by separate chaining. | Done |
| FR-18 | The user shall be able to list stored tests filtered by algorithm name, or all tests. | Done |
| FR-19 | The response shall include table statistics: total tests, bucket count and memory in bytes. | Done |
| FR-20 | The user shall be able to open a stored test by ID and replay it in the visualizer. | Done |
| FR-21 | The user shall be able to clear the history, after confirming in a dialog. | Done |

### 3.6 Data exchange (backend → frontend)

| ID | Requirement | Status |
| :--- | :--- | :---: |
| FR-22 | The backend shall return the execution log (operations, input, sorted values, metrics) as JSON for the frontend to consume. | Done |
| FR-23 | The backend shall serve the frontend files (`index.html`, `web/app.js`, `web/style.css`) so the whole system runs from a single binary. | Done |
| FR-24 | The backend shall export logs as a downloadable **CSV or JSON file**. | Gap |

### 3.7 HTTP API summary

| ID | Endpoint | Method | Purpose | Related FR |
| :--- | :--- | :---: | :--- | :--- |
| API-01 | `/api/sort` | GET | Run one sort. Params: `algorithm` (0..4), `structure` (0..3, default 0), `values`. | FR-01..FR-07 |
| API-02 | `/api/batch` | GET | Run a batch. Params: `algorithms`, `structures`, `values`. | FR-14..FR-16 |
| API-03 | `/api/history` | GET | List stored tests. Param: `algorithm` (name or `all`). | FR-17..FR-19 |
| API-04 | `/api/history/detail` | GET | Get one stored test with its operations. Param: `id`. | FR-20 |
| API-05 | `/api/history/clear` | DELETE | Empty the history. | FR-21 |

Error conventions: `400` invalid or missing parameters, `404` unknown route or test ID, `405` wrong HTTP method, `500` allocation or serialization failure.

## 4. Non-Functional Requirements

| ID | Category | Requirement | Status |
| :--- | :--- | :--- | :---: |
| NFR-01 | Memory | All data structures, buffers and histories shall use dynamic allocation (`malloc`, `realloc`, `free`). No fixed-size global arrays for user data. | Done |
| NFR-02 | Encapsulation | Every ADT shall be an opaque type: the `struct` is defined in the `.c` file and only a `typedef` is exposed in the `.h` file. | Done |
| NFR-03 | Robustness | Every allocation shall be checked for `NULL` and failures shall be reported to the caller (HTTP `500`) instead of crashing. | Done |
| NFR-04 | Limits | Input is limited to 300 values; a single run records at most 2,000,000 operations; a JSON response is limited to 32 MB. | Done |
| NFR-05 | Portability | The project shall build with `gcc`/`clang` (C11) and GNU Make on Linux, macOS and Windows. | Done |
| NFR-06 | Code quality | The code shall compile with `-Wall -Wextra` without warnings. | Done |
| NFR-07 | Language | All code identifiers, code comments and `README.md` shall be written in English. | Done |
| NFR-08 | Performance | Time shall be measured with a monotonic clock (`CLOCK_MONOTONIC`) around the sorting call only, excluding serialization. | Done |
| NFR-09 | Maintainability | Code shall be organized in layers: ADTs (`src/tad`), helpers (`src/utils`), engine (`src/sort_engine.c`) and HTTP layer (`src/server.c`). | Done |
| NFR-10 | Version control | The team shall use Git/GitHub with incremental commits. | Done |
| NFR-11 | Academic integrity | The team shall keep `AI_LOG.md` with the main AI prompts and how each answer was adapted or fixed. AI-generated code shall be refactored to the course standards. | Done |

## 5. Constraints and Assumptions

- The server is single-threaded and uses a global in-memory history, so it is meant for one user at a time.
- Memory metrics are computed from the structure sizes (`sizeof` of the list and its nodes or capacity, plus auxiliary buffers). They are an accurate model of the structure's footprint, not a reading from the operating system.
- The history is volatile: restarting the server clears it.
- The browser needs JavaScript enabled.

## 6. Known Gaps and Deviations

1. **File export (FR-24).** The course description asks the C program to export logs as JSON or CSV for the web interface to read. Ordena delivers the same JSON through a REST API instead of a file. This is functionally equivalent for the animation, but there is no CSV output and no "download log" button. Adding one would close the gap.
2. **Interface language.** The web interface text is in Portuguese (`lang="pt-BR"`). The English requirement applies to code documentation and `README.md`, so this is acceptable, but the team should confirm with the instructor.
3. **README layout.** `README.md` still describes JSON serialization inside `server.c` and does not list the `src/utils/` modules added later. It should be updated to match [ARCHITECTURE.md](ARCHITECTURE.md).

## 7. Traceability to the Course Description

| Course item | Requirements |
| :--- | :--- |
| Dynamic allocation and pointers, time and memory metrics | NFR-01, NFR-03, FR-07, FR-08, NFR-08 |
| Linear lists (singly, doubly, circular) with pointer-based sorting vs. contiguous vectors | FR-02, FR-05, FR-09 |
| Stack for undo in step-by-step mode | FR-12, FR-13 |
| Queue for batch processing | FR-14, FR-15, FR-16 |
| (Extra) Hash table with separate chaining to retrieve past reports by algorithm name | FR-17..FR-21 |
| Web interface that reads the log and animates the bars | FR-10, FR-11, FR-22, FR-23 (FR-24 pending) |
| Software engineering: Git, requirements, architecture documentation | NFR-10, this document, ARCHITECTURE.md |
| English documentation and README | NFR-07 |
| AI usage log and no "black box" code | NFR-11 |
