// GRR20186716 Vinícius Teixeira Vieira dos Santos

#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

//? gerenciamento das tarefas:
task_t mainTask;        // tareda da main 
task_t *currentTask;    // tarefa atual
int taskNumber = 0;     // número de tarefas "vivas" 
int userTasks;          // número de tarefas criadas p/ gerar seus códigos
/* status da tarefa:
    0: terminada
    1: pronta
    2: suspensa
*/

//? gerenciamento dispatcher:
task_t dispatcher;      // tarefa atual do dispatcher
task_t *readyQueue;     // fila de tarefas prontas
task_t *sleepQueue;     // fila de tarefas dormentes

//? gerenciamento do tempo:
unsigned int time = 0, startTime = 0;  
struct sigaction action;
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
    #ifdef DEBUG
        printf("Estou no dispatcher!\n");
    #endif
    
    while(taskNumber){
        // busca a task pronta para executá-la
        nextTask = scheduler();

        if (nextTask){
            if (queue_remove((queue_t **) &readyQueue, (queue_t *) nextTask) < 0){
                perror ("Erro ao remover tarefa na fila de prontos\n");
                exit(-1);        
            }
            task_switch(nextTask);
            // se a tarefa foi terminada, libero a mem. alocada pelo contexto 
            if (nextTask->status == 0)
                free(nextTask->context.uc_stack.ss_sp);
        }
        // busca se alguma task dormindo deve acordar
        if (sleepQueue){
            task_t *aux = sleepQueue;
            task_t *remove_task;
            do {
                if ( systime() >= aux->wakeup_time ){
                    remove_task = aux;
                    aux = aux -> next; // faz a troca pois a variável vai perder a ref. de next
                    // remove a tarefa da fila de dormindo e coloca na de prontas
                    if (queue_remove((queue_t **) &sleepQueue, (queue_t *) remove_task) < 0){
                        perror ("Erro ao remover tarefa na fila a mimir em dispatcher_func\n");
                        exit(-1);        
                    }
                    if (queue_append((queue_t **) &readyQueue, (queue_t *) remove_task) < 0){
                        perror ("Erro ao adicionar tarefa na fila de prontos em dispatcher_func\n");
                        exit(-1);        
                    }
                    remove_task->status = 1;
                } else {
                    aux = aux -> next;
                }
            } while (aux != sleepQueue && sleepQueue);
        }
    }

    task_exit(0);
}

void task_yield(){
    if( currentTask->sys_task == 0){
        if (queue_append((queue_t **) &readyQueue, (queue_t *)currentTask) < 0){
            perror ("Erro ao adicionar tarefa na fila de prontos\n");
            exit(-1);        
        }
    }
    task_switch(&dispatcher);
}

void task_setprio(task_t *task, int prio){
    // ignoro se a prioridade não for dentro do limite
    if (prio > 20 || prio < -20)
        return;

    if (task != NULL){
        task->static_prio = prio;
        task->din_prio = prio;
    } else {
        currentTask->static_prio = prio;
        currentTask->din_prio = prio;
    }
}

int task_getprio(task_t *task){
    if (task == NULL)
        return currentTask->static_prio;
    
    return task->static_prio;
}

unsigned int systime() {
	return time;
}

void tratador(){
    time++;
    currentTask->cpu_time++;
    // ignoro se é uma tarefa de sistema 
    if (currentTask->sys_task == 1)
        return;

    if(currentTask->quantum <= 0){
        // contabilizo o tempo de execução da tarefa 
        currentTask->cpu_time += systime() - startTime;
        task_yield();
    } else {
        currentTask->quantum--;
    }

}

void ppos_init(){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    // Ajusta a tarefa da main:
    mainTask.next = NULL;
    mainTask.prev = NULL;
    mainTask.id = 0;    // id 0 por ser main    
    userTasks = 0;      // número de tarefas existentes além da main
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
    #ifdef DEBUG
        printf("Estou no ppos_init!\n");
    #endif
    dispatcher.sys_task = 1;

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

int task_create(task_t *task, void (*start_routine)(void *),  void *arg){
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
    task->suspendQueue = NULL;  // fila de suspensas vazia
    task->exit_code = 0;
    // Para criar um ID "único", uso o número de tarefas atuais + 1 
    // (isso pode ocasionar ou não em problemas com overflow)
    userTasks++;
    task->id = userTasks;
    // Crio o contexto
    makecontext(&(task->context), (void *)(*start_routine), 1, arg);
    // adiciono à fila de prontos se não for o dispatcher ou a main
    if (task != &dispatcher){
        task->quantum = QUANTUM_TICKS; // ajusto quantum pra tarefa do usuário

        if (queue_append((queue_t **) &readyQueue, (queue_t *) task) < 0){
            perror ("Erro ao adicionar tarefa na fila de prontos\n");
            exit(-1);        
        }
    }
    taskNumber++;
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

    // enquanto tiver tarefas esperando por esta
    while (queue_size((queue_t *) currentTask->suspendQueue) > 0){
        // aux recebe a próxima tarefa
        task_t * aux = currentTask->suspendQueue;
        task_resume(aux, &(currentTask->suspendQueue));
        aux->exit_code = exitCode; 
    }
    
    // se a tarefa sendo finalizada for o dispatcher, volto pra main
    if(currentTask == &dispatcher){
        free (dispatcher.context.uc_stack.ss_sp);
        return;
    }
    taskNumber--;
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
    if (currentTask->sys_task == 0){
        currentTask->quantum = QUANTUM_TICKS;
    }
    task_t *aux = currentTask;
	currentTask = task;
	swapcontext (&aux->context, &task->context);

	return(0);
    
}

// A chamada task_join(b) faz com que a tarefa atual seja suspensa até a conclusão da tarefa b
int task_join(task_t *task){
    if (task == NULL){
        return -1;
    }
	if (task->status == 0 )
		return task->exit_code;	

    task_suspend (&(task->suspendQueue));
    return(currentTask->exit_code);
}

void task_suspend(task_t **queue){
    // remove da tarefa corrente (caso esteja lá) e insere na fila de adormecida
    queue_remove((queue_t **) &readyQueue, (queue_t *) currentTask);
    currentTask->status = 2;

    if (queue_append((queue_t **) queue, (queue_t *)currentTask) < 0){
        perror ("Erro ao adicionar tarefa na fila de suspensas\n");
        exit(-1);        
    }
    task_switch(&dispatcher);
}

void task_resume(task_t * task, task_t **queue){
    // remove da queue de adormecida e devolve à de prontos
    if (queue_remove((queue_t **) queue, (queue_t *) task) < 0){
        perror ("Erro ao remover tarefa na fila de suspensas em task_exit\n");
        exit(-1);        
    }
    task->status = 1; // upd status
    
    if (queue_append((queue_t **)&readyQueue, (queue_t *) task) < 0){
        perror ("Erro ao adicionar tarefa na fila de prontas em task_exit\n");
        exit(-1);        
    }
}

void task_sleep(int t){
	currentTask->status = 2;
    currentTask->wakeup_time = systime() + t;
    
    queue_remove((queue_t **)&readyQueue, (queue_t *)currentTask);
    
    if (queue_append((queue_t **)&sleepQueue, (queue_t *)currentTask) < 0){
        perror ("Erro ao adicionar tarefa na fila a mimir\n");
        exit(-1);        
    }
    task_switch(&dispatcher);
}


//--------------------------------------------- Semáforos --------------------------------------------------
// Inicializa um semáforo apontado por s com o valor inicial value e uma fila vazia.
int sem_create(semaphore_t *s, int value){
    s->s_queue = NULL;
    s->s_counter = value;
    // double-check
    if(s->s_counter == value)
        return 0;
    return -1;
}
/*
    Realiza a operação Down no semáforo apontado por s. Esta chamada pode ser bloqueante: caso o contador do semáforo seja negativo, 
    a tarefa corrente é suspensa, inserida no final da fila do semáforo e a execução volta ao dispatcher; caso contrário, a tarefa 
    continua a executar sem ser suspensa.

    Se a tarefa for bloqueada, ela será reativada quando uma outra tarefa liberar o semáforo (através da operação sem_up) ou caso o 
    semáforo seja destruído (operação sem_destroy)
*/
int sem_down(semaphore_t *s){
	if(s == NULL)
        return -1;
    // operação atômica de subtração
    if( __sync_sub_and_fetch(&(s->s_counter), 1) < 0){
        currentTask->status = 2;    // 2: suspensa

        if (queue_append((queue_t **)&s->s_queue, (queue_t *)currentTask) < 0){
            perror ("Erro ao inserir tarefa na fila do semáforo\n");
            exit(-1);        
        }
        task_switch(&dispatcher);
    } 
    return 0;
}

/*
    Realiza a operação Up no semáforo apontado por s. Esta chamada não é bloqueante (a tarefa que a executa não perde o processador). 
    Se houverem tarefas aguardando na fila do semáforo, a primeira da fila deve ser acordada e retornar à fila de tarefas prontas. 
*/
int sem_up(semaphore_t *s){
	if(s == NULL)
        return -1;

    __sync_add_and_fetch(&(s->s_counter), 1);
    // operação atômica de adição
    if (s->s_queue && s->s_counter <= 0){
        task_t *aux = s->s_queue;
	    if (queue_remove((queue_t **) &s->s_queue, (queue_t *) aux) < 0){
            perror ("Erro ao remover tarefa na fila do semáforo\n");
            exit(-1);
        }

        if (queue_append((queue_t **)&readyQueue, (queue_t *) aux) < 0){
            perror ("Erro ao inserir tarefa na fila de prontas\n");
            exit(-1);
        }
        aux->status = 1; // 1: pronta
    }
    
    return 0;
}

int sem_destroy(semaphore_t *s){
    task_t *remove_task;
    // remove todas as tasks do semáforo e as devolve à fila de prontas
    while (s->s_queue){
        remove_task = s->s_queue;
        if (queue_remove((queue_t **) &s->s_queue, (queue_t *) remove_task) < 0){
            perror ("Erro ao remover tarefa na fila do semáforo\n");
            exit(-1);
        }

        if (queue_append((queue_t **)&readyQueue, (queue_t *) remove_task) < 0){
            perror ("Erro ao inserir tarefa na fila de prontas\n");
            exit(-1);
        }
    }
    // double-check
    if (s->s_queue == NULL)
        return 0;

    return -1;
}



// ------------------------------- Filas de mensagens ---------------------------

int mqueue_create (mqueue_t *queue, int msgs, int size) {
    queue->buffer = malloc(msgs*size);
    // erro ao alocar memória
    if(!(queue->buffer))
        return -1;
    // configurações da queue
    queue->max_size = msgs;
    queue->status = 1;
    queue->msg_size = size;
    queue->msg_count = 0;
    queue->head = 0;
    queue->tail = 0;
    #ifdef DEBUG
        printf("queue max size: %d\n", queue->max_size);
    #endif
    // semáforos
    sem_create(&queue->s_item, 0);  
    sem_create(&queue->s_vaga, msgs);    
    sem_create(&queue->s_buffer, 1);

    return 0;
}

int mqueue_send (mqueue_t *queue, void *msg) {
    // status é diferente de 1 -> queue was destroyed
    if(!(queue->status))
        return -1;

    // semáforos para evitar racing conditions
    sem_down(&queue->s_vaga);
    sem_down(&queue->s_buffer);

    // insere no index final do buffer (tail) * tam da mensagem
    memcpy(queue->buffer + (queue->tail)*(queue->msg_size), msg, queue->msg_size);
    // atualiza valores de final do buffer (p/ próxima inserção) e counter de mensagens
    queue->tail = (queue->tail + 1) % queue->max_size;
    queue->msg_count++;

    // semáforos voltam
    sem_up(&queue->s_buffer);
    sem_up(&queue->s_item);
    
    return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg) {
    // status é diferente de 1 -> queue was destroyed
    if(!(queue->status))
        return -1;

    // semáforos para evitar racing conditions
    sem_down (&queue->s_item);
    sem_down (&queue->s_buffer);
    // copia do início (head) do buffer * tam da mensagem p/ msg
    memcpy (msg, queue->buffer + (queue->head)*(queue->msg_size), queue->msg_size);
    queue->head = (queue->head + 1) % queue->max_size;
    queue->msg_count--;
    // semáforos voltam
    sem_up (&queue->s_buffer);
    sem_up (&queue->s_vaga);

    return 0;
}

int mqueue_destroy (mqueue_t *queue) {
    if (!queue) 
        return -1;
    // status passa a ser 0 - "queue destruída"
    queue->status = 0;
    // erro ao destruir semáforos
    if (sem_destroy(&queue->s_item) || sem_destroy(&queue->s_vaga) || sem_destroy(&queue->s_buffer))
        return -1;
    return 0;
}

int mqueue_msgs (mqueue_t *queue) {
    if (queue->status)
        return queue->msg_count;
    
    return -1;
}

