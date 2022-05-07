// GRR20186716 Vinícius Teixeira Vieira dos Santos
// PingPongOS - PingPong Operating System
// Versão original cedida por:
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
//? colocando mais includes 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

//? Definição de constantes próprias
#define STACKSIZE 64*1024	/* tamanho de pilha das threads (contexts.c)*/
#define QUANTUM_TICKS 20	/* tamanho de pilha das threads (contexts.c)*/

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;	// ponteiros para usar em filas
  int id ;				              // identificador da tarefa
  ucontext_t context ;			    // contexto armazenado da tarefa
  short status ;			          // pronta, rodando, suspensa, ...
  short preemptable ;			      // pode ser preemptada?
  int static_prio;              // prioridade estática
  int din_prio;                 // prioridade dinânmica
  int quantum;                  // quantum da tarefa
  unsigned int exec_time;       // tempo de execução
  unsigned int cpu_time;        // tempo de uso de CPU
  unsigned int wakeup_time;     // tempo que a tarefa volta de suspensa
  unsigned int activations;     // vezes que foi ativada
  int sys_task;                 // se a tarefa é do sistema ou não (usuário)
  int exit_code;                // código de saída da tarefa
  struct task_t *suspendQueue; // fila de tarefas a qual deve esperar  
   // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  task_t *s_queue;
  int s_counter;
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
  int max_size;   // máximo de itens dentro do buffer
  int msg_size;   // tamanho de cada mensagem
  int status;     // status da queue (0 é "destruída" e 1 é "funcional")
  int msg_count;  // número de mensagens que estão no buffer
  void *buffer;   // estrutura de dados para armazenar as mensagens
  int head;       // indica a "primeira" mensagem
  int tail;       // indica a "última" mensagem
  // semáforos semelhantes aos do P11
  semaphore_t s_buffer; 
  semaphore_t s_item;   
  semaphore_t s_vaga;   
  // preencher quando necessário
} mqueue_t ;

#endif

