#ifndef QUEUE_H
#define QUEUE_H

#include "process.h"

/*
 * Cola FIFO simple implementada con un arreglo circular de punteros a
 * Process. Representa una de las tres colas de prioridad del MLFQ.
 */
typedef struct {
    Process *items[MAX_PROCESSES];
    int front;
    int rear;
    int count;
} Queue;

void init_queue(Queue *q);
int is_empty(const Queue *q);
void enqueue(Queue *q, Process *p);
Process *dequeue(Queue *q);

#endif
