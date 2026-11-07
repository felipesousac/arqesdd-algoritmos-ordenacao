#include <stdlib.h>
#include <string.h>
#include "../headerFiles/listaSequencial.h"

Lista* criaLista(size_t tamanhoElemento){
    if(tamanhoElemento == 0) return NULL;
    Lista *list = (Lista*) malloc(sizeof(Lista)); // aloca estrutura da lista
    if(list != NULL) {
        list->prox = 0;
        list->capacidade = CAPACIDADE_INICIAL;
        list->tamanhoElemento = tamanhoElemento;
        list->elementos = malloc((size_t)list->capacidade * tamanhoElemento);
        
        if(list->elementos == NULL){ // falha na alocação do array
            free(list);
            return NULL;
        }
    }
    return list;
}

// Função auxiliar (privada) para dobrar o tamanho da lista quando encher
static int expandeLista(Lista* list){
    int nova_capacidade = list->capacidade * 2;
    if(nova_capacidade <= list->capacidade ||
       (size_t)nova_capacidade > (size_t)-1 / list->tamanhoElemento) return 0;
    void *novo_vetor = realloc(list->elementos,
                               (size_t)nova_capacidade * list->tamanhoElemento);
    if(novo_vetor == NULL) // memória insuficiente
        return 0;
    
    list->elementos = novo_vetor;
    list->capacidade = nova_capacidade;
    return 1;
}

int insereFinal(Lista* list, const void *elemento){
    if(list == NULL || elemento == NULL) return 0;
    if(list->prox == list->capacidade){ // lista "cheia", precisa expandir
        if(!expandeLista(list)) return 0; // falha ao expandir
    }
    memcpy((char*)list->elementos + (size_t)list->prox * list->tamanhoElemento,
           elemento, list->tamanhoElemento);
    list->prox++;
    return 1;
}

int insereInicio(Lista* list, const void *elemento){
    if(list == NULL || elemento == NULL) return 0;
    if(list->prox == list->capacidade){
        if(!expandeLista(list)) return 0;
    }
    memmove((char*)list->elementos + list->tamanhoElemento,
            list->elementos, (size_t)list->prox * list->tamanhoElemento);
    memcpy(list->elementos, elemento, list->tamanhoElemento);
    list->prox++;
    return 1;
}

int removePosicao(Lista* list, int pos){
    if(list == NULL || pos < 0 || pos >= list->prox) return 0;
    size_t restantes = (size_t)(list->prox - pos - 1);
    memmove((char*)list->elementos + (size_t)pos * list->tamanhoElemento,
            (char*)list->elementos + (size_t)(pos + 1) * list->tamanhoElemento,
            restantes * list->tamanhoElemento);
    list->prox--;
    return 1;
}

int removeInicio(Lista* list){
    if(list == NULL || list->prox == 0) return 0;
    return removePosicao(list, 0);
}

int removeFinal(Lista* list){
    if(list == NULL || list->prox == 0) return 0;
    list->prox--; 
    return 1;
}

int tamLista(Lista* list){
    if(list == NULL) return -1;
    return list->prox;
}

int listaCheia(Lista* list){
    if(list == NULL) return -1;
    // Uma lista dinâmica nunca está conceitualmente cheia, a não ser que acabe a memória.
    // Retornamos 0 (falso) para manter compatibilidade com assinaturas estáticas.
    return 0; 
}

int listaVazia(Lista* list){
    if(list == NULL) return -1;
    return (list->prox == 0);
}

int consultaPosicao(Lista* list, int pos, void *elemento){
    if(list == NULL || elemento == NULL || pos < 0 || pos >= list->prox)
        return 0;
    memcpy(elemento, (char*)list->elementos + (size_t)pos * list->tamanhoElemento,
           list->tamanhoElemento);
    return 1;
}

void apagaLista(Lista* list){
    if(list != NULL){
        free(list->elementos);
        free(list);         // Depois liberar a estrutura
    }
}
