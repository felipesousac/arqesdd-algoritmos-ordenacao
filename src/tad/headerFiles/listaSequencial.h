#ifndef LISTASEQUENCIAL_H_INCLUDED
#define LISTASEQUENCIAL_H_INCLUDED

#include <stddef.h>

#define CAPACIDADE_INICIAL 5

typedef struct lista{ 
    int prox;          // Quantidade de elementos inseridos
    int capacidade;    // Tamanho atual alocado na memória
    size_t tamanhoElemento;
    void *elementos;   // Array dinâmico com elementos de tamanho fixo
} Lista;

Lista* criaLista(size_t tamanhoElemento);
int insereFinal(Lista* list, const void *elemento);
int insereInicio(Lista* list, const void *elemento);
int removePosicao(Lista* list, int pos);
int removeInicio(Lista* list);
int removeFinal(Lista* list);
int consultaPosicao(Lista* list, int pos, void *elemento);
int tamLista(Lista* list);
int listaCheia(Lista* list);
int listaVazia(Lista* list);
void apagaLista(Lista* list);

#endif
