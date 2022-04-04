// GRR20186716 Vinícius Teixeira Vieira dos Santos

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
//? gerenciamento do tempo:
unsigned int time = 0, startTime = 0;  
struct sigaction action ;
struct itimerval timer;

// Política de escalonamento para decidir qual a próxima tarefa a ativar
task_t *scheduler(){
    task_t *aux = readyQueue;
    task_t *priorityTask = readyQueue; // assumo que a tarefa de prioridade é a primeira 
    
    // não temos tarefas prontas
    if (aux == NULL)
        return NULL;
    // iterage para achar a tareda de menor prioridade
    do{
        if ( aux->din_prio < priorityTask->din_prio)
            priorityTask = aux;
        // se for igual, define prioridade pelo valor estático
        else if ( aux->din_prio == priorityTask->din_prio && aux->static_prio < priorityTask->static_prio)
            priorityTask = aux;
        
        aux = aux->next;
    }while(aux != readyQueue);
    
    // atualizo a prioridade dinâmica
    aux = priorityTask->next;
    
    while(aux != priorityTask){
        if (aux->din_prio > -20)
            aux->din_prio--;
        aux = aux->next;
    } 

    // como a tarefa será executada, din_prio volta a ser igual à static
    priorityTask->din_prio = priorityTask->static_prio;

    return priorityTask; 
}

void dispatcher_func(){

    task_t *nextTask;
    while(queue_size((queue_t *) readyQueue) > 0){
        nextTask = scheduler();

        if (nextTask != NULL){
            if (queue_remove((queue_t **) &readyQueue, (queue_t *) nextTask) < 0){
                perror ("Erro ao remover tarefa na fila de prontos\n");
                exit(-1);        
            }
            task_switch(nextTask);
            // se a tarefa foi terminada, libero a mem. alocada pelo contexto 
            if (nextTask->status == 0)
                free(nextTask->context.uc_stack.ss_sp);
        }
    }

    task_exit(0);
}

void task_yield (){
    
    if( currentTask->sys_task == 0){
        #ifdef DEBUG
            printf("%d é tarefa do usuário \n", currentTask->id);
        #endif
        if (queue_append((queue_t **) &readyQueue, (queue_t *)currentTask) < 0){
            perror ("Erro ao adicionar tarefa na fila de prontos\n");
            exit(-1);        
        }
    }
    task_switch(&dispatcher);
}

void task_setprio (task_t *task, int prio){
    // ignoro se a prioridade não for dentro do limite
    if (prio > 20 || prio < -20)
        return;

    if (task != NULL){
        task->static_prio = prio;
        task->din_prio = prio;
    }
    else{
        currentTask->static_prio = prio;
        currentTask->din_prio = prio;
    }
}

int task_getprio (task_t *task){
    
    if (task == NULL)
        return currentTask->static_prio;
    
    return task->static_prio;
}

unsigned int systime () {
	return time;
}


void tratador (){
    // ignoro se é uma tarefa de sistema 
    if (currentTask->sys_task == 1)
        return;

    if(currentTask->quantum == 0){
        // contabilizo o tempo de execução da tarefa 
        currentTask->cpu_time += systime() - startTime;
        task_yield();
    } else {
        currentTask->quantum--;
    }

    time++;
}

void ppos_init(){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    // Ajusta a tarefa da main:
    mainTask.next = NULL;
    mainTask.prev = NULL;
    mainTask.id = 0;    // id 0 por ser main    
    userTasks = 0;    // número de tarefas existentes além da main
    time = 0;           // tempo de execução começa a contar aqui
    // temporizadores
    mainTask.exec_time = systime();
    mainTask.cpu_time = 0;
    mainTask.activations = 0;
    mainTask.sys_task = 0;  // é uma tarefa de sistema
    // agora main tem contexto e quantum (P7)
    mainTask.quantum = QUANTUM_TICKS; 
    getcontext (&(mainTask.context));

    // Define a tarefa atual sendo a main 
    currentTask = &mainTask;

    // cria o dispatcher:
    task_create(&dispatcher,(void*) dispatcher_func, NULL);

    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = tratador ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0){
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;         // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;            // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;      // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;         // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer (ITIMER_REAL, &timer, 0) < 0){
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }
    task_yield(); // adiciona na queue
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
    // temporizadores
    task->exec_time = systime();
    task->cpu_time = 0;
    task->activations = 0;
    task->sys_task = 0;  // é uma tarefa do usuário
    task->quantum = 0;  // presumo primeiro que a tarefa é do sistema

    // Para criar um ID "único", uso o número de tarefas atuais + 1 
    // (isso pode ocasionar ou não em problemas com overflow)
    userTasks++;
    task->id = userTasks;
    // Crio o contexto
    makecontext(&(task->context), (void *)(*start_routine), 1, arg);
    // adiciono à fila de prontos se não for o dispatcher ou a main
    if (task != &dispatcher){
        #ifdef DEBUG
            printf("%d é tarefa do usuário \n", task->id);
        #endif
        task->quantum = QUANTUM_TICKS; // ajusto quantum pra tarefa do usuário

        if (queue_append((queue_t **) &readyQueue, (queue_t *) task) < 0){
            perror ("Erro ao adicionar tarefa na fila de prontos\n");
            exit(-1);        
        }
    }

    return (task->id);
}


int task_id(){
    return (currentTask->id);
}

void task_exit(int exitCode){
    currentTask->status = 0;
    currentTask->exec_time = systime() - currentTask->exec_time;
    
    printf("Task %d exit: execution time %u ms, processor time %u ms, %u activations\n",
	    currentTask->id, currentTask->exec_time, currentTask->cpu_time, currentTask->activations);
    // se a tarefa sendo finalizada for o dispatcher, volto pra main
    if(currentTask == &dispatcher){
        free (dispatcher.context.uc_stack.ss_sp);
        return;
    }
    task_switch(&dispatcher);
}

int task_switch(task_t *task){
    if (task == NULL){
        perror ("Erro na troca da tarefa. Tarefa é NULL.\n");
        return -1;
    }
    // tarefa foi acionada, então ajusto o quantum e o tempo em que inicia
    currentTask->activations++; 
    startTime = systime();
    if (currentTask->sys_task != 1){
        currentTask->quantum = QUANTUM_TICKS;
    }
	else {
        #ifdef DEBUG
            printf("%d é tarefa do usuário \n", task->id);
        #endif
    }
    task_t *aux = currentTask;
	currentTask = task;
	swapcontext (&aux->context, &task->context);

	return(0);
    
}