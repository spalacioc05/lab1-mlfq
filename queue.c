#include <assert.h>
#include <stddef.h> /* NULL: solo tipos y macros, sin I/O */
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
    /*
     * Desbordar la cola no es un error del usuario sino una violacion de una
     * invariante: validate_input ya garantiza n <= MAX_PROCESSES y un proceso
     * solo puede estar en una cola a la vez. Por eso es un assert (falla
     * ruidosamente en desarrollo) y no un mensaje a stderr: asi este modulo
     * no depende de la consola.
     */
    assert(q->count < MAX_PROCESSES);

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
