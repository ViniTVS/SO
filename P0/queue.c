
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
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
    return;
} 

int queue_append (queue_t **queue, queue_t *elem){
    // verifica se fila e elemento existem 
    if (elem == NULL || queue == NULL)
        return -1;
    // caso a fila esteja vazia
    if(*queue == NULL){
        elem->next = elem;
        elem->prev = elem;
        *queue = elem;
        return 0;
    }
    // caso contrário
    else{
        queue_t *aux = *queue;
        aux -> prev -> next = elem;
        elem -> prev = aux -> prev;
        aux -> prev = elem;
        elem -> next = aux;
        return 0;
    }
}

int queue_remove (queue_t **queue, queue_t *elem){
    if (elem == NULL || queue == NULL)
        return -1;
    // caso a fila esteja vazia
    if(*queue == NULL)
        return -1;
    // while

    queue_t *aux = *queue;
    printf("primeiro: %p\tRemove: %p \n", aux, elem);
    // aux = aux -> next;
    // percorre 
    do{
        aux = aux -> next;
        if(aux == elem){
            printf("\tAchei elemento pra tirar: %p\n", aux);

        }
        printf("\t%p \t%p\n", aux, elem);
    }while(aux != *queue);
    exit(0);
    // caso contrário
    // else{
    //     printf("Valor do ponteiro: %p \n", *queue);
    //     queue_t *aux = *queue;
    //     aux -> prev -> next = elem;
    //     elem -> prev = aux -> prev;
    //     aux -> prev = elem;
    //     elem -> next = aux;
    //     return 0;
    // }
    return 0;
}

