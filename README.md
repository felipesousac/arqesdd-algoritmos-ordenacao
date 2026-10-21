# Ordena — visualizador de algoritmos

Visualizador em português para Bubble, Selection, Insertion, Quick e Merge Sort. O usuário informa a sequência de inteiros; o motor principal está em C e registra comparações, trocas e escritas; o navegador lê o histórico do WebAssembly e reproduz cada operação no Canvas.

## Requisitos e execução

- Para compilar o motor WebAssembly: GNU Make e [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) com `emcc` no `PATH`.
- Para servir a interface: qualquer servidor HTTP local. A página usa módulos ES para carregar o módulo gerado.

```sh
make          # compila Wasm se emcc estiver instalado; caso contrário avisa e mantém o modo demonstração
make serve    # inicia o servidor em http://localhost:8000
```

Abra `http://localhost:8000`. Com Emscripten instalado e disponível no `PATH`, `make` cria `web/sort_engine.js` e `web/sort_engine.wasm`. Sem Emscripten, `make` informa que a compilação foi ignorada e a interface entra automaticamente em modo de demonstração com implementações JavaScript equivalentes; nesse modo os números de tempo medem JavaScript, não o motor C.

Para instalar o compilador WebAssembly, siga o [guia oficial de instalação do Emscripten](https://emscripten.org/docs/getting_started/downloads.html) e ative o ambiente do SDK no terminal antes de executar `make` novamente. Por exemplo, a instalação via `emsdk` requer ativar o ambiente a cada nova sessão de terminal.

## Escolhas de implementação

- O motor aceita de 1 a 4096 inteiros e registra até 2 milhões de operações por execução. A interface limita o vetor a 128 itens para manter a animação responsiva.
- Informe de 2 a 128 inteiros positivos entre 1 e 1.000.000, separados por vírgula, espaço ou ponto e vírgula. “Aplicar sequência” valida os dados; ordenar também aplica diretamente o texto atual. A sequência inicial é apenas um exemplo editável.
- Operações usam o formato compacto `{ tipo, indice1, indice2, valor1, valor2 }`, com enum `COMPARE=0`, `SWAP=1`, `WRITE=2`. O C também exporta getters para o vetor final, métricas e histórico. O JS lê as estruturas da memória linear do Wasm conforme o layout C de 32 bits.
- `comparisons` conta avaliações entre valores; `swaps` conta trocas efetivas de posições. Insertion e Merge registram deslocamentos/cópias como escritas, não como trocas.
- O tempo mede a execução do algoritmo no C usando `emscripten_get_now()`, sem incluir renderização ou leitura do histórico.
- O log tem limite fixo configurável por chamada. Se atingido, a ordenação continua e o vetor final/métricas permanecem corretos, mas passos posteriores não são registrados.

## Arquivos

- `src/sort_engine.c` — motor C e histórico de operações.
- `web/app.js` — ponte Wasm, fallback, Canvas e player.
- `web/style.css` — interface responsiva.
- `index.html` — estrutura da aplicação.
- `Makefile` — compilação Emscripten e alvo nativo opcional.
