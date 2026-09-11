#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "scheduler.h"
#include "csv.h"

/*
 * Esta es la capa mas externa: es el unico archivo que sabe que existe una
 * consola. Traduce los codigos que devuelve el dominio a mensajes, y le pasa
 * al scheduler las funciones con las que debe reportar lo que ocurre.
 */

/* Escenario principal del laboratorio: P1..P4 con los arrival/burst dados por el profesor. */
static int build_scenario(Process procs[]) {
    process_init(&procs[0], 1, 0, 8);
    process_init(&procs[1], 2, 1, 4);
    process_init(&procs[2], 3, 2, 9);
    process_init(&procs[3], 4, 3, 5);
    return 4;
}

/* Opciones de linea de comandos. */
typedef struct {
    int boost_interval;
    int trace;       /* 1 = imprimir la traza ciclo a ciclo */
    int show_help;
    int bad_option;  /* 1 = hubo un argumento invalido */
} Options;

/* Recibe el stream para que la ayuda pedida con -h salga por stdout y la que
   acompana a un error salga por stderr, junto al mensaje que la motivo. */
static void print_usage(FILE *out, const char *program) {
    fprintf(out, "Uso: %s [-v] [-b N]\n", program);
    fprintf(out, "  -v, --verbose   imprime la traza ciclo a ciclo\n");
    fprintf(out, "  -b, --boost N   intervalo del priority boost (por defecto 20)\n");
    fprintf(out, "  -h, --help      muestra esta ayuda\n");
}

/*
 * El intervalo de boost es configurable por CLI para poder responder las
 * preguntas de analisis con datos: correr el mismo escenario con distintos
 * valores y comparar los promedios, sin recompilar.
 */
static Options parse_options(int argc, char *argv[]) {
    Options opts = { .boost_interval = 20, .trace = 0, .show_help = 0, .bad_option = 0 };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            opts.trace = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            opts.show_help = 1;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--boost") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s necesita un numero (ejemplo: -b 5).\n", argv[i]);
                opts.bad_option = 1;
                return opts;
            }
            char *end;
            long value = strtol(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || *end != '\0' || value <= 0 || value > INT_MAX) {
                fprintf(stderr, "Error: '%s' no es un intervalo de boost valido (entero mayor que 0).\n",
                        argv[i + 1]);
                opts.bad_option = 1;
                return opts;
            }
            opts.boost_interval = (int)value;
            i++; /* el valor ya fue consumido */
        } else {
            fprintf(stderr, "Error: opcion desconocida '%s'.\n", argv[i]);
            opts.bad_option = 1;
            return opts;
        }
    }
    return opts;
}

/* Implementacion de TickObserver: la unica parte que sabe que la traza va a consola. */
static void print_tick(int time, const Process *p) {
    printf("[t=%2d] P%d ejecuta en Q%d (restante=%d)\n",
           time, p->pid, p->current_queue, p->remaining_time);
}

/* Traduce el codigo de validacion del dominio a un mensaje para el usuario. */
static void print_validation_error(ValidationResult result, int detail) {
    switch (result) {
        case VALIDATION_OK:
            break;
        case VALIDATION_PROCESS_COUNT:
            fprintf(stderr, "Error: la cantidad de procesos (%d) debe estar entre 1 y %d.\n",
                    detail, MAX_PROCESSES);
            break;
        case VALIDATION_BOOST_INTERVAL:
            fprintf(stderr, "Error: el intervalo de priority boost (%d) debe ser mayor que 0.\n", detail);
            break;
        case VALIDATION_QUANTUM:
            fprintf(stderr, "Error: el quantum de Q%d debe ser mayor que 0.\n", detail);
            break;
        case VALIDATION_ARRIVAL_TIME:
            fprintf(stderr, "Error: P%d tiene arrival_time negativo.\n", detail);
            break;
        case VALIDATION_BURST_TIME:
            fprintf(stderr, "Error: P%d tiene burst_time invalido (debe ser > 0).\n", detail);
            break;
    }
}

static void print_results(const Process procs[], int n) {
    printf("\nPID  Arrival  Burst  Start  Finish  Response  Turnaround  Waiting\n");

    double total_response = 0.0;
    double total_turnaround = 0.0;
    double total_waiting = 0.0;

    for (int i = 0; i < n; i++) {
        const Process *p = &procs[i];
        printf("P%-3d %-8d %-6d %-6d %-7d %-9d %-11d %-7d\n",
               p->pid, p->arrival_time, p->burst_time, p->start_time, p->finish_time,
               response_time(p), turnaround_time(p), waiting_time(p));

        total_response += response_time(p);
        total_turnaround += turnaround_time(p);
        total_waiting += waiting_time(p);
    }

    /* Los anchos alinean los promedios bajo Response, Turnaround y Waiting. */
    printf("\n%-36s%-10.2f%-12.2f%-7.2f\n", "Promedio",
           total_response / n, total_turnaround / n, total_waiting / n);
}

int main(int argc, char *argv[]) {
    Options opts = parse_options(argc, argv);
    if (opts.bad_option) {
        print_usage(stderr, argv[0]);
        return 1;
    }
    if (opts.show_help) {
        print_usage(stdout, argv[0]);
        return 0;
    }

    Process procs[MAX_PROCESSES];
    int n = build_scenario(procs);

    SchedulerConfig cfg = {
        .quantum = {2, 4, 8},
        .boost_interval = opts.boost_interval,
        .demote = NULL,                            /* NULL = regla de democion por defecto */
        .on_tick = opts.trace ? print_tick : NULL  /* la traza solo se arma con -v */
    };

    printf("MLFQ Scheduler Simulator\n");
    printf("------------------------\n\n");
    printf("Q0 quantum: %d\n", cfg.quantum[0]);
    printf("Q1 quantum: %d\n", cfg.quantum[1]);
    printf("Q2 quantum: %d\n", cfg.quantum[2]);
    printf("Priority boost: %d\n\n", cfg.boost_interval);

    int detail;
    ValidationResult validation = validate_input(procs, n, &cfg, &detail);
    if (validation != VALIDATION_OK) {
        print_validation_error(validation, detail);
        return 1;
    }

    printf("Simulating...\n");
    run_simulation(procs, n, cfg);

    print_results(procs, n);

    const char *filename = "results.csv";
    if (!export_csv(filename, procs, n)) {
        return 1;
    }
    printf("\nResults exported to %s\n", filename);

    return 0;
}
