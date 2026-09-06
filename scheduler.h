#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

#define NUM_QUEUES 3

/* Configuracion del MLFQ: quantum de cada cola y periodo del priority boost (S). */
typedef struct {
    int quantum[NUM_QUEUES];
    int boost_interval;
} SchedulerConfig;

/* Valida procesos y configuracion. Retorna 1 si todo es valido, 0 si no (imprime el motivo). */
int validate_input(const Process procs[], int n, const SchedulerConfig *cfg);

/* Corre la simulacion ciclo a ciclo y deja los tiempos calculados dentro de cada Process. */
void run_simulation(Process procs[], int n, SchedulerConfig cfg, int verbose);

#endif
