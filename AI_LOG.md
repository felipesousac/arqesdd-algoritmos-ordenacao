# AI Usage & Engineering Adaptation Log (`AI_LOG.md`)

This document records the chronological timeline of prompts, architectural pivots, AI-assisted implementations, and manual refactorings carried out during the development of **Ordena — Data Structures & Sorting Benchmark Platform** for the Data Structures course (`ARQESDD` — IFSP Araraquara).

> **Note on Prompt Language:** While this repository and documentation are standardized in English, all quoted prompts below are preserved verbatim in **Brazilian Portuguese (pt-BR)** exactly as they were originally written.

---

## 1. Chronological Development Timeline

### Stage 1 — Initial Project Conception & `PLAN.md` (C + WebAssembly + Vanilla JS)

The project started with the goal of building a browser-based sorting algorithm visualizer powered by a C engine compiled to WebAssembly (`.wasm`), combined with a Vanilla JavaScript frontend.

#### Prompt 1 — Initial Architecture & `PLAN.md` Generation
> *"me ajude a gerar um PLAN.md para estruturar a criação de um projeto que terá a interface visual escrita em javascript e o motor da aplicação em C, vai ser um projeto que permite o usuário a informar um array de numeros e comparar o funcionamento e performance de algoritmos de ordenação, o motor da aplicação vai ser escrito em C, mas acredito que fazer chamadas HTTP para essa aplicação C não é sustentável, uma vez que trocas e comparações precisam ser contabilizadas e mostradas ao usuário a medida em que estão acontecendo, vi uma abordagem utilizando .wasm que acredito que atenda a essa questão, a seguir segue um texto com alguns requisitos do projeto, não precisa levar tudo ao pé da letra, me pergunte para confirmar suas decisões, a idéia é criar uma plataforma no navegador que permite ao usuário visualizar os algoritmos em funcionamento e mostrar as métricas pro usuário, além dessa integração dos projetos, também serão implementadas as funcionalidades -> Projeto Testes de algoritmos de ordenação (...) Principais Recursos: Seleção de algoritmos como Bolha, Inserção, Seleção, Merge e Quick; Geração de dados aleatórios, invertidos ou ordenados; Painel de estatísticas com tempo exato, número de comparações e movimentações; Modo de passo a passo para avançar a execução do código. (...) Estrutura de Dados Sugerida: `Metricas` (`nome`, `comparacoes`, `trocas`, `tempo_ms`)."*

- **AI Output (`PLAN.md`)**:
  - Designed a 4-phase plan using **C compiled to WebAssembly via Emscripten** as the in-browser sorting engine and **Vanilla JS / HTML5 / CSS3** for rendering and playback.
  - Defined the core C structures (`TipoOperacao`: `COMPARA`, `TROCA`, `ESCRITA`; `Operacao`; and `Metricas` with `historico_operacoes` and `emscripten_get_now()`).
  - Structured implementation into: (1) Emscripten/Wasm setup & memory interop, (2) C engine & the 5 sorting algorithms, (3) Vanilla JS UI, and (4) `requestAnimationFrame` animation player with Play/Pause/Step controls.
- **Execution & Initial Fixes**:
  - A fresh chat session (without prior context) was opened to implement the generated `PLAN.md`.
  - After the initial implementation, a series of short conversations were used to fix minor early bugs and adjust algorithm behavior.

---

### Stage 2 — Architectural Pivot: From WebAssembly to a C REST API

To simplify the architecture and integrate course-style Abstract Data Types (ADTs) directly into the C backend, WebAssembly was replaced by a C HTTP REST API built with **Mongoose**, returning the full operation history as JSON for the web client to animate.

#### Prompt 2 — Switching from WebAssembly to Mongoose REST API + Dynamic Sequential List
> *"Vamos alterar a forma como esse projeto vai funcionar, iremos esquecer o WebAssembly, agora a aplicaçao em C vai funcionar como uma API que vai receber as requisicoes GET e devolver a resposta com todo o historico de operacoes para o cliente, cliente esse que ira ser responsavel por ler a resposta e formatar ela na interface para o usuario, acredito que de forma semelhante o codigo em web assembly faz isso. Utilizaremos um tipo abstrato de dados para lidar com a montagem do json do historico de ordenacao, uma lista sequencial dinamica (deixarei no codigo os arquivos base dessa lista que deverao ser adaptados para o nosso codigo). do lado do C, vai ser utilizada a biblioteca Moongose para servir como o servidor de rede, alem disso acredito que o codigo C vai ter que ser adaptado para essa mudança, mantenha a base dos algoritmos como estao e adicione o que precisa. pode remover a logica relacionada a webassembly e seu consumo no projeto"*

- **Outcome & Adaptations**:
  - Removed all WebAssembly / Emscripten build artifacts and JavaScript interop code.
  - Added the **Mongoose** HTTP server in `src/server.c` to serve `GET /api/sort` requests and return the complete sorting operation trace in JSON format.
  - Adapted the course-provided sequential list base files (`listaSequencial`) into a dynamic sequential list ADT to collect and serialize the sorting operation history, establishing the first working REST API version of the project.

---

### Stage 3 — New Course Requirements & Full Data Structures Architecture (`plan.md`)

When the final assessment specification PDF introduced additional mandatory criteria (dynamic memory allocation with memory tracking, Singly/Doubly/Circular Linked Lists with pointer manipulation, Stack for step-by-step Undo, Queue for batch processing, Separate-Chaining Hash Table for test history, and full English standardization), the PDF and the discipline's reference ADT files (`lista.h`/`lista.c` and `tabela_hash.c`) were provided as context.

#### Prompt 3 — Gap Analysis & Full Architecture Planning (`plan.md`)
> *"note que o projeto atualmente está em progresso, já temos progresso referente ao que está sendo pedido, atualmente utilizamos uma lista sequencial para montar o array de comparações de cada algoritmo de ordenação, porém precisamos adaptar para atender todos os critérios. minha idéia é criar um plan.md depois dessa análise para iniciarmos a implementação.*
>
> *sobre pilhas, entendo que teremos que criar a estrutura de dado do lado do javascript, uma vez que esse controle de operações é feito por lá, a fila para o batch processing deverá ser feita do lado do C*
>
> *tabela hash será feita do lado do C e essa funcionalidade para recuperar testes anteriores ainda não existe, deveremos salvar todos os testes e seu histórico*
>
> *leve em consideração o pdf de contexto e minhas falas para análise do projeto e me retorne um plan.md para as alterações.*
>
> *as estruturas de dados enviadas servirao como base para o padrão de TADs que busco, mas lembre-se que deverão ser dinamicas, entao utilize malloc, realloc e free para manipular esses dados*
>
> *me pergunte se surgirem duvidas no meio do caminho"*

- **AI Output (`plan.md`)**:
  - Produced a full Gap Analysis comparing the existing REST prototype against every course criterion.
  - Specified the 5-phase engineering plan covering dynamic allocation (`malloc`/`realloc`/`free`), memory usage instrumentation (`memory_bytes`), English standardization, new REST endpoints (`/api/batch`, `/api/history`, `/api/history/detail`), and the exact ADT designs.
- **Execution & Adaptations to Course ADT Standards**:
  - A new chat session was started to implement `plan.md`, producing the complete data-structures platform with the following key adaptations:
  1. **Opaque Linear ADTs (`src/tad/`)**: Refactored `listaSequencial` to `sequential_list` and implemented `singly_linked_list`, `doubly_linked_list`, and `circular_linked_list` following the discipline's opaque-type pattern (`typedef struct ...` in headers under `src/tad/headerFiles/`, internal structs encapsulated in `.c` files under `src/tad/sourceFiles/`).
  2. **Pointer-Based Sorting & Observer Callbacks (`list_sort_observer.h`)**: Implemented all 5 algorithms (Bubble, Selection, Insertion, Quick, Merge) operating on linked list nodes by relinking `next`/`prev` pointers (including backwards traversal via `prev` in Doubly Linked List Insertion Sort and in-place pointer merging in Merge Sort without an auxiliary array), using lightweight observer callbacks (`on_compare`, `on_swap`, `on_write`) to emit visualization events without breaking ADT encapsulation.
  3. **Dynamic Queue ADT (`queue.h/.c`)**: Built a linked-node dynamic queue (`front`/`rear` pointers, `malloc`/`free`) in C to enqueue and execute batch benchmark tasks via `/api/batch`.
  4. **Dynamic Separate-Chaining Hash Table ADT (`hash_table.h/.c`)**: Adapted the discipline's modular hash table pattern (`tabela_hash.c`) into a 100% dynamic Separate-Chaining Hash Table indexed by `algorithmName`, using a dynamically allocated bucket array (`HashNode **buckets`) resized via `realloc` when load factor $\alpha = \text{count} / M > 0.75$ and linked lists (`HashNode *next`) to chain collisions and multiple test runs.
  5. **Frontend Stack ADT (`OperationStack` in `web/app.js`)**: Replaced flat frame-array indexing with two linked-node stacks (`undoStack` and `redoStack`) storing reversible step records (`prevFirstValue`, `prevSecondValue`, previous metrics) for Step Back (Undo) and Step Forward (Redo) playback.

---

### Stage 4 — Post-Implementation Bug Fixes & Manual Refactoring

- **Bug Fixes**: Following the implementation of `plan.md`, a series of short conversations were used to fix minor runtime and edge-case bugs.
- **Manual Refactoring**: Several parts of the codebase were manually refactored to improve modularity, readability, and separation of concerns, including:
  - Extracting responsibilities out of `src/server.c` into dedicated utility modules under `src/utils/` (`batch_runner`, `history_store`, `http_response`, `input_parser`, `json_builder`, and `json_serializer`).
  - Refining internal implementations across `src/server.c`, `src/tad/sourceFiles/hash_table.c`, `src/tad/sourceFiles/queue.c`, `src/tad/sourceFiles/sequential_list.c`, and `web/app.js`.

---

## 2. Verification & Quality Assurance
- Compiled with `gcc -O2 -std=c11 -D_GNU_SOURCE -Wall -Wextra` via `make rebuild` with **zero warnings**.
- Verified all 20 combinations of `(5 algorithms × 4 linear data structures)` for sorting correctness, operation trace accuracy, batch queue execution, hash table history lookup/replay, and dynamic memory cleanup (`free`).
