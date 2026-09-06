#ifndef PROCESS_H
#define PROCESS_H

/* Tamano maximo de procesos soportado por el simulador. */
#define MAX_PROCESSES 50

/*
 * Representa un proceso dentro del simulador MLFQ.
 * remaining_time indica cuanto CPU le falta.
 * quantum_used indica cuanto del quantum actual (de su cola) ha consumido.
 * current_queue es el nivel de cola donde esta actualmente (0, 1 o 2).
 */
typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int start_time;
    int finish_time;
    int first_response_time;
    int current_queue;
    int quantum_used;
    int finished;
} Process;

/* start_time y first_response_time inician en -1 para saber que el proceso aun no ha corrido. */
static inline void process_init(Process *p, int pid, int arrival_time, int burst_time) {
    p->pid = pid;
    p->arrival_time = arrival_time;
    p->burst_time = burst_time;
    p->remaining_time = burst_time;
    p->start_time = -1;
    p->finish_time = -1;
    p->first_response_time = -1;
    p->current_queue = 0;
    p->quantum_used = 0;
    p->finished = 0;
}

/* Las tres metricas se calculan unicamente con estas formulas, tal como pide el enunciado. */
static inline int response_time(const Process *p) {
    return p->first_response_time - p->arrival_time;
}

static inline int turnaround_time(const Process *p) {
    return p->finish_time - p->arrival_time;
}

static inline int waiting_time(const Process *p) {
    return turnaround_time(p) - p->burst_time;
}

#endif
