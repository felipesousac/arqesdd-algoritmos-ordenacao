# Ordena — visualizador de algoritmos

Aplicação web para explorar Bubble Sort, Selection Sort, Insertion Sort, Quick Sort e Merge Sort. Os algoritmos e o registro das operações são executados em C. Um servidor Mongoose recebe requisições HTTP GET, devolve a execução em JSON e serve a interface; o navegador interpreta o histórico e anima os passos.

## Requisitos

- Compilador C com suporte a C11
- GNU Make

O Mongoose está incluído em `vendor/` (`mongoose.c` e `mongoose.h`), sob a licença GPL-2.0-only ou licença comercial. Consulte os avisos no início dos arquivos da dependência. O TAD da lista sequencial dinâmica usado para armazenar o histórico está em `src/tad/`.

## Compilar e executar

Na pasta do projeto:

```sh
make
make serve
```

Abra [http://localhost:8000](http://localhost:8000). Para escolher outro endereço, execute `./build/ordena-server http://127.0.0.1:9000`. `make clean` remove o executável.

## API

`GET /api/sort?algorithm=<id>&values=<lista>` executa a ordenação e retorna a entrada, o vetor ordenado, as métricas e o histórico completo de operações.

IDs de algoritmo: `0` Bubble, `1` Selection, `2` Insertion, `3` Quick e `4` Merge. Os valores são inteiros positivos entre 1 e 1.000.000; a API aceita de 2 a 128 valores separados por vírgula, espaço ou ponto e vírgula.

Exemplo:

```sh
curl 'http://localhost:8000/api/sort?algorithm=0&values=8,3,6,1'
```

A resposta contém `algorithm`, `algorithmName`, `input`, `sorted`, `metrics` (`itemCount`, `comparisons`, `swaps`, `elapsedMs`, `operationCount`) e `operations`. Cada operação contém `type`, `firstIndex`, `secondIndex`, `firstValue` e `secondValue`. `type` é `0` para comparação, `1` para troca e `2` para escrita. Em escritas, `secondValue` indica se o movimento entra na métrica de trocas/movimentos.

## Visualizador

1. Informe uma sequência de 2 a 128 inteiros positivos, ou gere uma sequência crescente/decrescente.
2. Escolha um algoritmo e pressione **Ordenar**.
3. Use os controles para reproduzir, pausar, avançar, voltar ou mudar a velocidade.

Insertion e Merge registram deslocamentos e cópias como escritas. A API limita os parâmetros para corresponder ao limite atual da interface.
