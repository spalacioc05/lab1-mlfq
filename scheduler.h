#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "queue.h"

#define NUM_QUEUES 3

/*
 * Punto de extension: que hacer con un proceso que agoto su quantum sin
 * terminar. Permite reemplazar la regla de democion (o agregar una cola
 * mas) sin tocar el bucle principal de run_simulation.
 */
typedef void (*DemotionPolicy)(Process *p, Queue queues[NUM_QUEUES]);

/* Configuracion del MLFQ: quantum de cada cola y periodo del priority boost (S). */
typedef struct {
    int quantum[NUM_QUEUES];
    int boost_interval;
    DemotionPolicy demote; /* NULL usa la regla por defecto (Q0->Q1->Q2, se queda en Q2) */
} SchedulerConfig;

/* Valida procesos y configuracion. Retorna 1 si todo es valido, 0 si no (imprime el motivo). */
int validate_input(const Process procs[], int n, const SchedulerConfig *cfg);

/* Corre la simulacion ciclo a ciclo y deja los tiempos calculados dentro de cada Process. */
void run_simulation(Process procs[], int n, SchedulerConfig cfg, int verbose);

#endif
