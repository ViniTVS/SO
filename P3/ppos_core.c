
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

//? gerenciamento das tarefas:
task_t mainTask;        // tareda da main 
task_t *currentTask;    // tarefa atual
int userTasks;          // número de tarefas (sem contabilizar a main)
/* status da tarefa:
   -1: suspensa
    0: terminada
    1: pronta
*/

//? gerenciamento dispatcher:
task_t dispatcher;      
task_t *dispatcherTask; // tarefa atual do dispatcher
task_t *readyQueue;     // fila de prontas


// Política de escalonamento para decidir qual a próxima tarefa a ativar
task_t *scheduler(){
    #ifdef DEBUG
        printf("mudou para o scheduler!  \n");
    #endif
    task_t *aux = readyQueue;
    task_t *priorityTask = readyQueue; // assumo que a tarefa de prioridade é a primeira 
    
    // não temos tarefas prontas
    if (aux == NULL)
        return NULL;
    // iterage para achar a prioridade
    do{
        #ifdef DEBUG
            printf("Passando pela tarefa %d do %p\n", aux->id, readyQueue);
        #endif
        
        if ( aux->din_prio < priorityTask->din_prio)
            priorityTask = aux;
        else {// se for igual, define prioridade pelo estático 
            if ( 
                aux->din_prio == priorityTask->din_prio 
                && aux->static_prio < priorityTask->static_prio
            ){
                priorityTask = aux;
            }
        }
        /*
        // atualizo a prioridade dinâmica
        if (aux->din_prio > -20)
            aux->din_prio--;
        */
        aux = aux -> next;
    }while(aux != readyQueue);
    
    // atualizo a prioridade dinâmica
    aux = priorityTask -> next;
    while(aux != priorityTask){
        #ifdef DEBUG
            printf("Passando pela tarefa %d do %d\n", aux->id, priorityTask->id);
        #endif
        if (aux->din_prio > -20)
            aux->din_prio--;
        aux = aux -> next;
    } 

    #ifdef DEBUG
        printf("priorityTask ficou com a: %d\n", priorityTask->id);
    #endif
    return priorityTask; 
}

void dispatcher_func(){
    
    #ifdef DEBUG
        printf("mudou para o dispatcher!\n");
    #endif

    task_t *nextTask;
    while(queue_size((queue_t *) readyQueue) > 0){
        nextTask = scheduler();

        if (nextTask != NULL){
            if (queue_remove((queue_t **) &readyQueue, (queue_t *) nextTask) < 0){
                perror ("Erro ao remover tarefa na fila de prontos\n");
                exit(-1);        
            }
            task_switch(nextTask);
            // se a tarefa foi terminada
            if (nextTask->status == 0)
                free(nextTask->context.uc_stack.ss_sp);
        }
    }

        #ifdef DEBUG
            printf("Buscando nextTask\n");
        #endif
    task_exit(0);
}

void task_yield (){
    
    if( currentTask != &mainTask && currentTask != &dispatcher )
        if (queue_append((queue_t **) &readyQueue, (queue_t *)currentTask) < 0){
            perror ("Erro ao adicionar tarefa na fila de prontos\n");
            exit(-1);        
        }
    
    #ifdef DEBUG
        printf("A tarefa %d pediu yield\n", currentTask->id);
    #endif
    
    task_switch(&dispatcher);
}


void task_setprio (task_t *task, int prio){
    // if (prio > 20 || prio < -20)
    //     return;


    if (task != NULL){
        #ifdef DEBUG
            printf("task_setprio da %d\n\tde: %d para: %d\n\n", task->id, task->static_prio, prio);
        #endif
        task -> static_prio = prio;
        task -> din_prio = prio;
    }
    else{
        currentTask -> static_prio = prio;
        currentTask -> din_prio = prio;
    }
}

int task_getprio (task_t *task){
    
    if (task == NULL)
        return currentTask -> id;
    
    return task -> static_prio;
}

void ppos_init(){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    // Ajusta a tarefa da main:
    mainTask.next = NULL;
    mainTask.prev = NULL;
    mainTask.id = 0;    // id 0 por ser main    
    userTasks = 0;    // número de tarefas existentes além da main

    // Define a tarefa atual sendo a main 
    currentTask = &mainTask;
    // cria o dispatcher:
    task_create(&dispatcher,(void*) dispatcher_func, NULL);
    
    #ifdef DEBUG
        printf("Dispatcher criado!\n");
    #endif
}

int task_create (task_t *task, void (*start_routine)(void *),  void *arg){
    // Verifico pegadinhas do prof.
    if (task == NULL){
        perror ("Erro na criação da tarefa\n");
        return -1;
    }

    // Basicamente um copia e cola de contexts.c...
    getcontext (&(task->context));

    char *stack;
    stack = malloc (STACKSIZE);
    // Erro ao alocar
    if (!stack){
        perror ("Erro na criação da pilha\n");
        return -1;
    }
    // ajustes do context
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
    // recebe prioridade default (0) 
    task->static_prio = 0;
    task->din_prio = 0;
    task->status = 1;

    // Para criar um ID "único", uso o número de tarefas atuais + 1 
    // (isso pode ocasionar ou não em problemas com overflow)
    userTasks++;
    task->id = userTasks;
    // Crio o contexto
    makecontext(&(task->context), (void *)(*start_routine), 1, arg);
    // adiciono à fila de prontos se não for o dispatcher ou a main
    if (task != &dispatcher && task != &mainTask){
        if (queue_append((queue_t **) &readyQueue, (queue_t *) task) < 0){
            perror ("Erro ao adicionar tarefa na fila de prontos\n");
            exit(-1);        
        }
        #ifdef DEBUG
            printf("Adicionaei %p na fila de prontos %p\n", task, &readyQueue);
        #endif
    }

    return (task->id);
}


int task_id(){
    return (currentTask->id);
}

void task_exit(int exitCode){

    #ifdef DEBUG
        printf("Exit tarefa %d\n", currentTask->id);
    #endif

    currentTask -> status = 0;
    
    if(currentTask == &dispatcher){
        free (dispatcher.context.uc_stack.ss_sp);
        task_switch(&mainTask);
        return;
    }
    task_switch(&dispatcher);
}

int task_switch(task_t *task){
    if (task == NULL){
        perror ("Erro na troca da tarefa\n");
        return -1;
    }

    #ifdef DEBUG
        printf("Switch da tarefa %d para %d \n", currentTask->id, task -> id);
    #endif

    /* Isso não funciona :(
    // Faço a troca de contexto
	swapcontext (&currentTask->context, &task->context);
    // e atualizo a tarefa atual
	currentTask = task;
    */
	task_t *aux = currentTask;
	currentTask = task;
	swapcontext (&aux->context, &task->context);

	return(0);
    
}