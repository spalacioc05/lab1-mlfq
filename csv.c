#include <stdio.h>
#include "csv.h"

int export_csv(const char *filename, const Process procs[], int n) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: no se pudo crear el archivo %s\n", filename);
        return 0;
    }

    fprintf(fp, "PID,Arrival,Burst,Start,Finish,Response,Turnaround,Waiting\n");
    for (int i = 0; i < n; i++) {
        const Process *p = &procs[i];
        fprintf(fp, "P%d,%d,%d,%d,%d,%d,%d,%d\n",
                p->pid, p->arrival_time, p->burst_time, p->start_time, p->finish_time,
                response_time(p), turnaround_time(p), waiting_time(p));
    }

    fclose(fp);
    return 1;
}
