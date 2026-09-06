#include <stdio.h>
#include "queue.h"

void init_queue(Queue *q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

int is_empty(const Queue *q) {
    return q->count == 0;
}

void enqueue(Queue *q, Process *p) {
    if (q->count == MAX_PROCESSES) {
        fprintf(stderr, "Error: la cola esta llena, no se puede encolar el proceso P%d\n", p->pid);
        return;
    }
    q->items[q->rear] = p;
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->count++;
}

Process *dequeue(Queue *q) {
    if (is_empty(q)) {
        return NULL;
    }
    Process *p = q->items[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->count--;
    return p;
}
