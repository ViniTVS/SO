// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Teste de semáforos (pesado)

#include <stdio.h>
#include <stdlib.h>
#include "../ppos.h"

task_t taskProdutor[3];
task_t taskConsumidor[2];
semaphore_t  s_vaga, s_buffer, s_item;

int buffer[5] = {0, 0, 0, 0, 0};
int b_consumidor = 0; // onde consumidor vai retirar
int b_produtor = 0;   // onde produtor vai inserir

// ------------------------- funções auxiliares --------------------------
int randoms(int lower, int upper){
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

int produz(){
  // valor que será "produzido"
  int temp = randoms(1, 99);
  buffer[b_produtor] = temp; // "produz"
  b_produtor = (b_produtor + 1) % 5;
  return temp;
}

int consome(){
  // valor que será "consumido"
  int temp = buffer[b_consumidor];
  buffer[b_consumidor] = 0; // "consome"
  b_consumidor = (b_consumidor + 1) % 5;
  return temp;
}

// ------------------------- corpo das tarefas --------------------------
void consumidor(void *id){
  while (1){
    sem_down(&s_item);
    sem_down(&s_buffer);
    int temp = consome();
    sem_up(&s_buffer);
    sem_up(&s_vaga);
    // print item
    printf("\t\t\t%s consumiu %d\n", (char *) id, temp);
    task_sleep(1000);
  }
}

void produtor(void *id){
  while (1){
    task_sleep(1000);

    sem_down(&s_vaga);

    sem_down(&s_buffer);
    // insere item no buffer
    int temp = produz(id);
    sem_up(&s_buffer);

    printf("%s produziu %d\n", (char *) id, temp);

    sem_up(&s_item);
  }
}

// --------------------------- main -------------------------------------
int main (int argc, char *argv[]){
  int i;
  ppos_init();

  // inicializa semáforos
  sem_create(&s_vaga, 3);
  sem_create(&s_buffer, 5);
  sem_create(&s_item, 0);

  // cria as tarefas
  task_create(&taskProdutor[0], produtor, "p1");
  task_create(&taskProdutor[1], produtor, "p2");
  task_create(&taskProdutor[2], produtor, "p3");

  task_create(&taskConsumidor[0], consumidor, "c1");
  task_create(&taskConsumidor[1], consumidor, "c2");

  // aguarda as tarefas encerrarem
  for (i=0; i < 3; i++)
    task_join(&taskProdutor[i]);
  
  for (i=0; i < 2; i++)
    task_join(&taskConsumidor[i]);
  
  // destroi o semáforo
  sem_destroy(&s_vaga);
  sem_destroy(&s_buffer);
  sem_destroy(&s_item);

  task_exit(0);
  exit(0);
}
