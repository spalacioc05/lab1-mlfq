#ifndef CSV_H
#define CSV_H

#include "process.h"

/* Escribe results.csv con el encabezado y una fila por proceso. Retorna 1 si tuvo exito. */
int export_csv(const char *filename, const Process procs[], int n);

#endif
