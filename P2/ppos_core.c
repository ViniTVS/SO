
#include "ppos.h"
#include "ppos_data.h"

task_t mainTask;
task_t *currentTask;
int taskNum;

void ppos_init(){
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    // Ajusta a tarefa da main:
    mainTask.next = NULL;
    mainTask.prev = NULL;
    mainTask.id = 0;    // id 0 por ser main    
    taskNum = 0;    // número de tarefas existentes
    // Define a tarefa atual sendo a main 
    currentTask = &mainTask;
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
    
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
    // Para criar um ID "único", uso o número de tarefas atuais + 1 
    // (isso pode ocasionar ou não em problemas com overflow)
    taskNum++;
    task->id = taskNum;
    // Crio o contexto
    makecontext(&(task->context), (void *)(*start_routine), 1, arg);
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