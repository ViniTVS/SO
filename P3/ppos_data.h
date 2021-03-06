// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
//? colocando mais includes 
#include <stdio.h>
#include <stdlib.h>

//? Definição de constantes próprias
#define STACKSIZE 64*1024	/* tamanho de pilha das threads (contexts.c)*/

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  short preemptable ;			// pode ser preemptada?
  int static_prio;     // prioridade estática
  int din_prio;    // prioridade dinânmica

   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;


//? gerenciamento das tarefas:
extern task_t mainTask;        // tareda da main 
extern task_t *currentTask;    // tarefa atual
extern int userTasks;          // número de tarefas (sem contabilizar a main)
/* status da tarefa:
   -1: suspensa
    0: terminada
    1: pronta
*/

//? gerenciamento dispatcher:
extern task_t dispatcher;      
extern task_t *dispatcherTask; // tarefa atual do dispatcher
extern task_t *readyQueue;     // fila de prontas

#endif

