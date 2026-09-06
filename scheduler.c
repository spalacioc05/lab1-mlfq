#include <stdio.h>
#include "scheduler.h"
#include "queue.h"

/*
 * Decision de diseno: mientras un proceso tiene la CPU se guarda aparte en
 * "current" y NO permanece dentro del arreglo de la cola. Esto evita tener
 * que sacarlo y reencolarlo en cada ciclo (lo cual romperia el Round Robin,
 * porque otros procesos de la misma cola le quitarian el turno antes de
 * tiempo). Las colas solo contienen procesos en espera.
 */

int validate_input(const Process procs[], int n, const SchedulerConfig *cfg) {
    if (n <= 0 || n > MAX_PROCESSES) {
        fprintf(stderr, "Error: la cantidad de procesos debe estar entre 1 y %d.\n", MAX_PROCESSES);
        return 0;
    }
    if (cfg->boost_interval <= 0) {
        fprintf(stderr, "Error: el intervalo de priority boost debe ser mayor que 0.\n");
        return 0;
    }
    for (int i = 0; i < NUM_QUEUES; i++) {
        if (cfg->quantum[i] <= 0) {
            fprintf(stderr, "Error: el quantum de Q%d debe ser mayor que 0.\n", i);
            return 0;
        }
    }
    for (int i = 0; i < n; i++) {
        if (procs[i].arrival_time < 0) {
            fprintf(stderr, "Error: P%d tiene arrival_time negativo.\n", procs[i].pid);
            return 0;
        }
        if (procs[i].burst_time <= 0) {
            fprintf(stderr, "Error: P%d tiene burst_time invalido (debe ser > 0).\n", procs[i].pid);
            return 0;
        }
    }
    return 1;
}

/* Paso 1: mete a Q0 los procesos que llegan exactamente en "time". */
static void add_new_arrivals(Process procs[], int n, int time, Queue *q0) {
    for (int i = 0; i < n; i++) {
        if (procs[i].arrival_time == time) {
            procs[i].current_queue = 0;
            enqueue(q0, &procs[i]);
        }
    }
}

/* Paso 2: cada boost_interval ciclos, todo lo que no ha terminado vuelve a Q0. */
static void apply_priority_boost(Queue queues[NUM_QUEUES], Process **current) {
    for (int level = 1; level < NUM_QUEUES; level++) {
        Process *p;
        while ((p = dequeue(&queues[level])) != NULL) {
            p->current_queue = 0;
            p->quantum_used = 0;
            enqueue(&queues[0], p);
        }
    }
    if (*current != NULL) {
        (*current)->current_queue = 0;
        (*current)->quantum_used = 0;
    }
}

/*
 * Paso 3: si llego (o el boost trajo) un proceso a una cola de mayor
 * prioridad que la del proceso en ejecucion, este se interrumpe. Conserva
 * su quantum_used y vuelve a su propia cola (no se le regala un quantum
 * nuevo por la interrupcion).
 */
static void preempt_if_needed(Queue queues[NUM_QUEUES], Process **current) {
    if (*current == NULL) {
        return;
    }
    for (int level = 0; level < (*current)->current_queue; level++) {
        if (!is_empty(&queues[level])) {
            enqueue(&queues[(*current)->current_queue], *current);
            *current = NULL;
            return;
        }
    }
}

/* Paso 4: la cola de mayor prioridad no vacia manda. Dentro de una cola, FIFO = Round Robin. */
static Process *pick_next_process(Queue queues[NUM_QUEUES]) {
    for (int level = 0; level < NUM_QUEUES; level++) {
        if (!is_empty(&queues[level])) {
            return dequeue(&queues[level]);
        }
    }
    return NULL;
}

/* Paso 10: agoto el quantum sin terminar -> baja de prioridad (Q2 se queda en Q2). */
static void demote(Process *p, Queue queues[NUM_QUEUES]) {
    if (p->current_queue < NUM_QUEUES - 1) {
        p->current_queue++;
    }
    p->quantum_used = 0;
    enqueue(&queues[p->current_queue], p);
}

void run_simulation(Process procs[], int n, SchedulerConfig cfg, int verbose) {
    Queue queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++) {
        init_queue(&queues[i]);
    }

    Process *current = NULL;
    int time = 0;
    int finished_count = 0;

    while (finished_count < n) {
        add_new_arrivals(procs, n, time, &queues[0]);

        if (time > 0 && time % cfg.boost_interval == 0) {
            apply_priority_boost(queues, &current);
        }

        preempt_if_needed(queues, &current);

        if (current == NULL) {
            current = pick_next_process(queues);
        }

        if (current == NULL) {
            /* Nadie ha llegado todavia; la CPU queda ociosa este ciclo. */
            time++;
            continue;
        }

        if (current->start_time == -1) {
            current->start_time = time;
            current->first_response_time = time;
        }

        if (verbose) {
            printf("[t=%d] P%d ejecuta en Q%d (restante=%d)\n",
                   time, current->pid, current->current_queue, current->remaining_time);
        }

        current->remaining_time--;
        current->quantum_used++;

        if (current->remaining_time == 0) {
            current->finish_time = time + 1;
            current->finished = 1;
            finished_count++;
            current = NULL;
        } else if (current->quantum_used == cfg.quantum[current->current_queue]) {
            demote(current, queues);
            current = NULL;
        }

        time++;
    }
}
