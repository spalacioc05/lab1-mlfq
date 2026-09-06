#include <stdio.h>
#include "process.h"
#include "scheduler.h"
#include "csv.h"

/* Escenario principal del laboratorio: P1..P4 con los arrival/burst dados por el profesor. */
static int build_scenario(Process procs[]) {
    process_init(&procs[0], 1, 0, 8);
    process_init(&procs[1], 2, 1, 4);
    process_init(&procs[2], 3, 2, 9);
    process_init(&procs[3], 4, 3, 5);
    return 4;
}

static void print_results(const Process procs[], int n) {
    printf("\nPID  Arrival  Burst  Start  Finish  Response  Turnaround  Waiting\n");
    for (int i = 0; i < n; i++) {
        const Process *p = &procs[i];
        printf("P%-3d %-8d %-6d %-6d %-7d %-9d %-11d %-7d\n",
               p->pid, p->arrival_time, p->burst_time, p->start_time, p->finish_time,
               response_time(p), turnaround_time(p), waiting_time(p));
    }
}

int main(void) {
    Process procs[MAX_PROCESSES];
    int n = build_scenario(procs);

    SchedulerConfig cfg = {
        .quantum = {2, 4, 8},
        .boost_interval = 20
    };

    printf("MLFQ Scheduler Simulator\n");
    printf("------------------------\n\n");
    printf("Q0 quantum: %d\n", cfg.quantum[0]);
    printf("Q1 quantum: %d\n", cfg.quantum[1]);
    printf("Q2 quantum: %d\n", cfg.quantum[2]);
    printf("Priority boost: %d\n\n", cfg.boost_interval);

    if (!validate_input(procs, n, &cfg)) {
        return 1;
    }

    printf("Simulating...\n");
    run_simulation(procs, n, cfg, 0);

    print_results(procs, n);

    const char *filename = "results.csv";
    if (export_csv(filename, procs, n)) {
        printf("\nResults exported to %s\n", filename);
    } else {
        return 1;
    }

    return 0;
}
