# Biblioteca de Filas
O sistema operacional gerencia muitas filas: de processos prontos, suspensos, dormindo, esperando em semáforos, etc. A estrutura de dados mais adequada para implementar essas filas é uma lista circular duplamente encadeada, como indicada na figura abaixo:

Este projeto consiste em construir uma pequena biblioteca que ofereça operações básicas de inserção e remoção em uma lista circular duplamente encadeada totalmente escrita em C (padrão C99), usando estruturas e ponteiros. A fila é genérica e pode ser usada para organizar vários tipos de dados.

Esta biblioteca será utilizada em vários outros projetos, portanto capriche na implementação!

## Interface

A biblioteca a ser construída deverá respeitar rigorosamente a interface definida no arquivo queue.h (que não deve ser modificado). Ela deverá ser totalmente escrita em C (C99 ou similar), em um arquivo único chamado queue.c, e deverá funcionar corretamente com o programa de teste testafila.c.

Eventuais mensagens de erro devem ser escritas na saída de erro (stderr).
## A entregar

Somente o arquivo queue.c deverá ser entregue ao professor.
