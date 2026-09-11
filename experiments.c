#include <stdio.h>
#include "process.h"
#include "scheduler.h"

/*
 * Programa aparte que responde con datos las preguntas de analisis del
 * enunciado. Reutiliza el mismo scheduler que mlfq.exe sin modificarlo: es
 * posible porque el dominio no depende de la consola ni de archivos, solo
 * recibe una SchedulerConfig y devuelve los tiempos dentro de los procesos.
 *
 * Se compila aparte (make experiments) para no mezclar la corrida oficial
 * del laboratorio con las corridas exploratorias.
 */

typedef struct {
    double response;
    double turnaround;
    double waiting;
    int last_finish;      /* ciclo en que termina el ultimo proceso */
    int first_turnaround; /* turnaround de procs[0]; en contencion es el proceso largo */
    int longest_gap;      /* ciclos seguidos sin CPU de procs[0]: esto es starvation */
} Averages;

/*
 * Observador que mide la racha mas larga que procs[0] pasa sin CPU. Es la
 * medida directa de starvation: no importa cuando termina, sino cuanto tiempo
 * seguido queda relegado.
 *
 * Usa variables de archivo porque TickObserver no transporta un contexto de
 * usuario; es aceptable aqui porque experiments.c corre un escenario a la vez
 * y las reinicia antes de cada corrida.
 */
static int gap_tracked_pid = 1;
static int gap_last_tick = -1;
static int gap_longest = 0;

static void track_longest_gap(int time, const Process *p) {
    if (p->pid != gap_tracked_pid) {
        return;
    }
    if (gap_last_tick >= 0) {
        int gap = time - gap_last_tick - 1;
        if (gap > gap_longest) {
            gap_longest = gap;
        }
    }
    gap_last_tick = time;
}

/* Escenario oficial del enunciado. */
static int build_lab_scenario(Process procs[]) {
    process_init(&procs[0], 1, 0, 8);
    process_init(&procs[1], 2, 1, 4);
    process_init(&procs[2], 3, 2, 9);
    process_init(&procs[3], 4, 3, 5);
    return 4;
}

/*
 * Escenario de contencion: un proceso largo compitiendo contra una racha de
 * procesos cortos que siguen llegando cada 2 ciclos. Es el caso donde el
 * priority boost deja de ser decorativo, porque sin el, el proceso largo
 * queda atrapado en las colas bajas mientras Q0 nunca se vacia.
 */
static int build_contention_scenario(Process procs[]) {
    process_init(&procs[0], 1, 0, 20); /* el largo */

    /*
     * Un corto de burst 2 llegando cada 2 ciclos satura exactamente la CPU:
     * Q0 nunca alcanza a vaciarse, asi que el proceso largo solo avanza
     * cuando algo lo devuelve a Q0. Con menos cortos la racha se agota y el
     * largo termina igual con o sin boost, que fue justo lo que paso en el
     * escenario del enunciado.
     */
    int n = 1;
    for (int i = 0; i < 24; i++) {
        process_init(&procs[n], n + 1, 2 + i * 2, 2);
        n++;
    }
    return n;
}

static Averages run_and_measure(int (*build)(Process[]), int boost_interval) {
    Process procs[MAX_PROCESSES];
    int n = build(procs);

    gap_tracked_pid = procs[0].pid;
    gap_last_tick = -1;
    gap_longest = 0;

    SchedulerConfig cfg = {
        .quantum = {2, 4, 8},
        .boost_interval = boost_interval,
        .on_tick = track_longest_gap
    };
    run_simulation(procs, n, cfg);

    Averages avg = {0.0, 0.0, 0.0, 0, 0, 0};
    avg.first_turnaround = turnaround_time(&procs[0]);
    avg.longest_gap = gap_longest;

    for (int i = 0; i < n; i++) {
        avg.response += response_time(&procs[i]);
        avg.turnaround += turnaround_time(&procs[i]);
        avg.waiting += waiting_time(&procs[i]);

        if (procs[i].finish_time > avg.last_finish) {
            avg.last_finish = procs[i].finish_time;
        }
    }
    avg.response /= n;
    avg.turnaround /= n;
    avg.waiting /= n;
    return avg;
}

static void print_table(const char *title, int (*build)(Process[]),
                        const int intervals[], int count) {
    printf("\n%s\n", title);
    printf("boost   response  turnaround  waiting  turnaround P1  espera max P1  ultimo ciclo\n");

    for (int i = 0; i < count; i++) {
        Averages avg = run_and_measure(build, intervals[i]);
        printf("%-8d%-10.2f%-12.2f%-9.2f%-15d%-15d%-12d\n",
               intervals[i], avg.response, avg.turnaround, avg.waiting,
               avg.first_turnaround, avg.longest_gap, avg.last_finish);
    }
}

int main(void) {
    /* 1000 es mayor que la duracion de ambos escenarios: equivale a no tener boost. */
    const int intervals[] = {2, 3, 5, 10, 20, 1000};
    const int count = (int)(sizeof(intervals) / sizeof(intervals[0]));

    printf("Comparacion de intervalos de priority boost\n");
    printf("(boost = 1000 equivale a NO tener boost: la simulacion termina antes)\n");

    print_table("Escenario del enunciado: P1-P4, sin contencion sostenida",
                build_lab_scenario, intervals, count);

    print_table("Escenario de contencion: 1 proceso de burst 20 + 24 cortos de burst 2",
                build_contention_scenario, intervals, count);

    printf("\n");
    return 0;
}
