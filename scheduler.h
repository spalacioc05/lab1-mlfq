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

/*
 * Observador opcional de la simulacion: se invoca una vez por ciclo, justo
 * antes de que el proceso consuma su ciclo de CPU. Existe para que el
 * scheduler pueda reportar lo que ocurre sin saber a donde va ese reporte
 * (consola, archivo, o ningun lado). Gracias a esto scheduler.c no
 * necesita incluir stdio.h.
 */
typedef void (*TickObserver)(int time, const Process *p);

/* Configuracion del MLFQ: quantum de cada cola y periodo del priority boost (S). */
typedef struct {
    int quantum[NUM_QUEUES];
    int boost_interval;
    DemotionPolicy demote;  /* NULL usa la regla por defecto (Q0->Q1->Q2, se queda en Q2) */
    TickObserver on_tick;   /* NULL no reporta nada */
} SchedulerConfig;

/* Codigo del primer problema encontrado al validar. */
typedef enum {
    VALIDATION_OK = 0,
    VALIDATION_PROCESS_COUNT,
    VALIDATION_BOOST_INTERVAL,
    VALIDATION_QUANTUM,
    VALIDATION_ARRIVAL_TIME,
    VALIDATION_BURST_TIME
} ValidationResult;

/*
 * Valida procesos y configuracion SIN imprimir nada: devuelve el codigo del
 * primer problema y deja en *detail el dato que lo identifica (el nivel de
 * cola con quantum invalido, o el pid del proceso invalido); -1 si el codigo
 * no necesita detalle. Quien llama decide como comunicar el error al usuario.
 */
ValidationResult validate_input(const Process procs[], int n, const SchedulerConfig *cfg, int *detail);

/* Corre la simulacion ciclo a ciclo y deja los tiempos calculados dentro de cada Process. */
void run_simulation(Process procs[], int n, SchedulerConfig cfg);

#endif
