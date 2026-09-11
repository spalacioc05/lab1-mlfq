#include <stddef.h> /* NULL: solo tipos y macros, sin I/O */
#include "scheduler.h"
#include "queue.h"

/*
 * Decision de diseno: mientras un proceso tiene la CPU se guarda aparte en
 * "current" y NO permanece dentro del arreglo de la cola. Esto evita tener
 * que sacarlo y reencolarlo en cada ciclo (lo cual romperia el Round Robin,
 * porque otros procesos de la misma cola le quitarian el turno antes de
 * tiempo). Las colas solo contienen procesos en espera.
 *
 * Este archivo no incluye stdio.h a proposito: las reglas del scheduler no
 * deben depender de la consola ni de archivos. Los errores se devuelven como
 * codigo (ValidationResult) y los eventos se reportan por cfg.on_tick.
 */

ValidationResult validate_input(const Process procs[], int n, const SchedulerConfig *cfg, int *detail) {
    *detail = -1;

    if (n <= 0 || n > MAX_PROCESSES) {
        *detail = n;
        return VALIDATION_PROCESS_COUNT;
    }
    if (cfg->boost_interval <= 0) {
        *detail = cfg->boost_interval;
        return VALIDATION_BOOST_INTERVAL;
    }
    for (int i = 0; i < NUM_QUEUES; i++) {
        if (cfg->quantum[i] <= 0) {
            *detail = i; /* nivel de cola con el quantum invalido */
            return VALIDATION_QUANTUM;
        }
    }
    for (int i = 0; i < n; i++) {
        if (procs[i].arrival_time < 0) {
            *detail = procs[i].pid;
            return VALIDATION_ARRIVAL_TIME;
        }
        if (procs[i].burst_time <= 0) {
            *detail = procs[i].pid;
            return VALIDATION_BURST_TIME;
        }
    }
    return VALIDATION_OK;
}

/*
 * Paso 1: todo proceso nuevo entra a Q0 porque es la cola de mayor
 * prioridad del MLFQ; asi cualquier proceso tiene oportunidad inmediata
 * de correr antes de que el sistema sepa si es corto o largo.
 */
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

/*
 * Paso 10: agoto el quantum sin terminar -> baja de prioridad. Q2 es la
 * ultima cola (no existe una cola inferior), asi que un proceso que ya
 * esta en Q2 simplemente vuelve al final de Q2 con el quantum reiniciado.
 *
 * Esta es la regla por defecto. cfg.demote permite reemplazarla (ej. una
 * cola adicional, o una regla que no reinicie el quantum) sin modificar
 * run_simulation.
 */
static void default_demote(Process *p, Queue queues[NUM_QUEUES]) {
    if (p->current_queue < NUM_QUEUES - 1) {
        p->current_queue++;
    }
    p->quantum_used = 0;
    enqueue(&queues[p->current_queue], p);
}

/*
 * Bucle principal: el tiempo avanza de a un ciclo. En cada vuelta se
 * repiten los mismos pasos (llegadas, boost, posible preemption, elegir
 * quien corre, ejecutar un ciclo, y terminar o demover si corresponde).
 * Un solo ciclo de CPU por vuelta es lo que hace que la simulacion sea
 * de tiempo discreto y facil de seguir.
 */
void run_simulation(Process procs[], int n, SchedulerConfig cfg) {
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

        /* start_time sigue en -1 solo la primera vez que el proceso obtiene la CPU. */
        if (current->start_time == -1) {
            current->start_time = time;
            current->first_response_time = time;
        }

        /* Se reporta el ciclo antes de consumirlo, para que el observador vea
           el estado con el que el proceso entra a ejecutar. */
        if (cfg.on_tick != NULL) {
            cfg.on_tick(time, current);
        }

        current->remaining_time--;
        current->quantum_used++;

        if (current->remaining_time == 0) {
            /* El ciclo "time" acaba de ejecutarse completo, asi que el proceso termina en time + 1. */
            current->finish_time = time + 1;
            current->finished = 1;
            finished_count++;
            current = NULL;
        } else if (current->quantum_used == cfg.quantum[current->current_queue]) {
            DemotionPolicy demote = cfg.demote ? cfg.demote : default_demote;
            demote(current, queues);
            current = NULL;
        }

        time++;
    }
}
