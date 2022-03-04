// GRR20186716 Vinícius Teixeira Vieira dos Santos
#include "queue.h"
#include <stdio.h>

int queue_size (queue_t *queue){
    // fila vazia?
    if(queue == NULL)
        return 0;

    // então ela tem ao menos um item
    int i = 1;    
    queue_t *aux = queue->next;
    
    while(aux != queue){
        aux = aux -> next;
        i++;
    }
    
    return i;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){
    printf("%s: [", name);
    if (queue == NULL || print_elem == NULL){
        printf("]\n");
        return;
    }

    // percorre fila p/ print
    queue_t *aux = queue;
    do{ 
        print_elem(aux);
        aux = aux -> next;
        printf(" ");
    }while(aux != queue);

    printf("]\n");
    return;
} 

int queue_append (queue_t **queue, queue_t *elem){
    // verifica se fila e elemento existem 
    if (elem == NULL || queue == NULL)
        return -1;
    // caso o elem já esteja em uma fila
    if(elem -> prev != NULL || elem -> next != NULL)
        return -1;
    // caso a fila esteja vazia
    if(*queue == NULL){
        // elem só precisa apontar p/ si mesmo
        elem->next = elem;
        elem->prev = elem;
        *queue = elem;
        return 0;
    }
    // insere no final da fila
    queue_t *first = *queue;
    // arruma ponteiros do que era o último e novo elem
    first -> prev -> next = elem;
    elem -> prev = first -> prev;
    // arruma ponteiros do primeiro e novo elem
    first -> prev = elem;
    elem -> next = first;
    return 0;
    
}

int queue_remove (queue_t **queue, queue_t *elem){
    // verifica se fila e elemento existem 
    if (elem == NULL || queue == NULL)
        return -1;
    // caso a fila esteja vazia
    if(*queue == NULL)
        return -1;

    queue_t *aux = *queue;
    
    // percorre a fila
    do{
        // encontra elem pra remover
        if(aux == elem){
            // se a fila tiver um único elemento, a esvazio
            if(aux -> next == aux){
                *queue = NULL;
            } // se tiver mais que um
            else{
                // anterior e próximo apontam p/ si mesmos
                aux -> prev -> next = aux -> next;
                aux -> next -> prev = aux -> prev;
                // se eu estiver retirando o primeiro elemento, aponto p/ o seguinte
                if(aux == *queue)
                    *queue = aux -> next;
            }
            // o elem a ser removido aponta para nada
            elem -> prev = NULL;
            elem -> next = NULL;
            
            return 0;
        }

        aux = aux -> next;
    }while(aux != *queue);
    // não encontrou o elemento na fila
    return -1;
}

