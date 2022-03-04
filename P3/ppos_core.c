
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
        printf("mudou para o scheduler!\n");
    #endif
    task_t *aux;
    aux = readyQueue;
    task_t *priorityTask; 
    priorityTask = readyQueue; // assumo que a tarefa de prioridade é a primeira 
    // não temos tarefas prontas
    if (aux == NULL)
        return NULL;
    // iterage para achar a prioridade
    while(aux -> next != readyQueue){
        #ifdef DEBUG
            printf("Passando pela tarefa %p!\n", &aux);
        #endif
        
        if ( aux->din_prio < priorityTask->din_prio)
            priorityTask = aux;
        else if ( // se for igual, define prioridade pelo estático
            aux->din_prio == priorityTask->din_prio 
            && aux->static_prio < priorityTask->static_prio
        )
            priorityTask = aux;
        /*
        // atualizo a prioridade dinâmica
        if (aux->din_prio > -20)
            aux->din_prio--;
        */
        aux = aux -> next;
    }
    // atualizo a prioridade dinâmica
    for(aux = readyQueue; aux -> next != readyQueue; aux = aux -> next){
        if (aux->din_prio > -20)
            aux->din_prio--;
    } 

    return priorityTask; 
}

void dispatcher_func(){
    
    #ifdef DEBUG
        printf("mudou para o dispatcher!\n");
    #endif

    task_t *nextTask;
    while(userTasks > 0){
        nextTask = scheduler();
        if (nextTask != NULL){
            task_switch(nextTask);
        }
    }
}

void task_yield (){
    return;
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
        queue_append((queue_t **) &readyQueue, (queue_t *) task);
        #ifdef DEBUG
            printf("Adicionaei na fila de prontos!\n");
        #endif
    }

    return (task->id);
}


int task_id(){
    return(currentTask->id);
}

void task_exit(int exitCode){
    // exitCode: ignorar este parâmetro por enquanto, pois ele somente será usado mais tarde
    task_switch(&mainTask);
    currentTask = &mainTask;
}

int task_switch(task_t *task){
    if (task == NULL){
        perror ("Erro na troca da tarefa\n");
        return -1;
    }
    /* Isso não funciona :(
    // Faço a troca de contexto
	swapcontext (&currentTask->context, &task->context);
    // e atualizo a tarefa atual
	currentTask = task;
    */
	task_t *aux;
	aux = currentTask;
	currentTask = task;
	swapcontext (&aux->context, &task->context);

	return(0);
    
}