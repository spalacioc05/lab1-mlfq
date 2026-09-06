# Laboratorio 1 - MLFQ

**Estudiante:** Santiago Palacio Cárdenas

Simulador en C de un scheduler con política Multi-Level Feedback Queue
(MLFQ), con tres colas de prioridad, priority boost configurable y
cálculo de métricas de planificación.

## Objetivo

Implementar en C un simulador que decida, ciclo a ciclo, qué proceso
ocupa la CPU siguiendo la política MLFQ, y que calcule al final el
response time, turnaround time y waiting time de cada proceso.

## Funcionamiento

Hay tres colas de prioridad, cada una con su propio quantum:

```
Proceso nuevo
     |
     v
  Q0 (quantum 2)   <- prioridad alta
     |  quantum agotado sin terminar
     v
  Q1 (quantum 4)   <- prioridad media
     |  quantum agotado sin terminar
     v
  Q2 (quantum 8)   <- prioridad baja
     |  quantum agotado sin terminar
     v
  se queda en Q2 (no hay cola inferior)
```

Siempre se ejecuta la cola no vacía de mayor prioridad (Q0 > Q1 > Q2).
Dentro de una misma cola, el orden es FIFO, es decir, **Round Robin**: el
proceso al frente de la cola recibe la CPU y, si agota su quantum, pasa
al final de la siguiente cola.

Si un proceso agota completamente el quantum de su cola sin terminar, se
**demueve** un nivel (Q0→Q1, Q1→Q2); si ya está en Q2 vuelve al final de
Q2 mismo. Si termina antes de agotar el quantum, simplemente termina sin
democión.

Cada cierto número de ciclos (**priority boost**, configurable, 20 en el
escenario principal) todos los procesos que aún no han terminado vuelven
a Q0 con su quantum reiniciado. Esto evita que un proceso quede
relegado indefinidamente a una cola de baja prioridad.

La simulación avanza en **tiempo discreto**: en cada ciclo se agregan
llegadas, se aplica el boost si corresponde, se decide qué proceso
corre, se ejecuta exactamente un ciclo de CPU, y se revisa si el proceso
termina o debe demoverse.

## Estructura del proyecto

```
lab1-mlfq/
├── main.c        Escenario principal y ejecución del programa
├── process.h     Struct Process y fórmulas de métricas
├── queue.c/h     Cola FIFO simple (arreglo circular)
├── scheduler.c/h Simulación MLFQ (el corazón del laboratorio)
├── csv.c/h       Generación de results.csv
├── tests.c       Pruebas mínimas con assert()
├── Makefile      Compilación para Windows y Linux
├── README.md
├── .gitignore
└── results.csv   Salida de la última ejecución
```

Cada archivo tiene una única responsabilidad: `process` describe los
datos, `queue` implementa la estructura de datos, `scheduler` decide qué
proceso corre, y `csv` exporta resultados. Es simplemente separación de
responsabilidades por archivo, no una arquitectura formal.

## Compilación y ejecución

### Windows (MSYS2/MinGW)

Con Make:

```
mingw32-make
.\mlfq.exe

mingw32-make test

mingw32-make clean
```

O compilando directamente:

```
gcc -Wall -Wextra -std=c11 -o mlfq.exe main.c queue.c scheduler.c csv.c
```

### Linux

Con Make:

```
make
./mlfq

make test

make clean
```

O compilando directamente:

```
gcc -Wall -Wextra -std=c11 -o mlfq main.c queue.c scheduler.c csv.c
```

El programa no usa nada específico de un sistema operativo (solo C
estándar y la biblioteca estándar), así que el mismo código fuente
compila igual en ambos.

## Escenario de prueba

| PID | Arrival | Burst |
|-----|---------|-------|
| P1  | 0       | 8     |
| P2  | 1       | 4     |
| P3  | 2       | 9     |
| P4  | 3       | 5     |

Q0 = 2, Q1 = 4, Q2 = 8, priority boost cada 20 ciclos.

## Resultados

| PID | Arrival | Burst | Start | Finish | Response | Turnaround | Waiting |
|-----|---------|-------|-------|--------|----------|------------|---------|
| P1  | 0       | 8     | 0     | 23     | 0        | 23         | 15      |
| P2  | 1       | 4     | 2     | 14     | 1        | 13         | 9       |
| P3  | 2       | 9     | 4     | 26     | 2        | 24         | 15      |
| P4  | 3       | 5     | 6     | 21     | 3        | 18         | 13      |

Estos valores salen de `results.csv`, generado al ejecutar el programa.

## Métricas

- **Response:** tiempo hasta que el proceso obtiene CPU por primera vez.
- **Turnaround:** tiempo total desde que llega hasta que termina.
- **Waiting:** tiempo total que estuvo esperando CPU (sin ejecutar).

```
response_time   = first_response_time - arrival_time
turnaround_time = finish_time - arrival_time
waiting_time    = turnaround_time - burst_time
```

## Decisiones de diseño

- `Queue` es un arreglo circular de tamaño fijo (no una lista enlazada),
  porque el número de procesos es pequeño y conocido de antemano.
- El proceso que tiene la CPU se guarda en una variable `current`
  **separada** de las colas mientras se ejecuta. Si se reencolara en
  cada ciclo, otros procesos de la misma cola le quitarían el turno
  antes de agotar su quantum, rompiendo el Round Robin.
- Si llega (o el boost trae) un proceso a una cola de mayor prioridad,
  `current` libera la CPU pero conserva su `quantum_used`: no se le
  regala un quantum nuevo por la interrupción.
- El priority boost mueve a Q0 todo lo que espera en Q1/Q2 y, si hay un
  proceso ejecutándose, lo reclasifica como Q0 sin sacarlo de la CPU.
- El CSV se genera en un archivo aparte (`csv.c`) para no mezclar la
  lógica de planificación con la de exportación de resultados.

No se implementan patrones de diseño formales (Factory, Strategy,
Observer, etc.); la separación por archivos solo busca que cada uno sea
fácil de leer por separado.

## Pruebas

`tests.c` usa `assert()` de la biblioteca estándar (sin frameworks) y
cubre:

- cálculo de response, turnaround y waiting time;
- democión Q0 → Q1 y Q1 → Q2;
- permanencia en Q2 al agotar el quantum allí;
- priority boost devolviendo un proceso a Q0;
- un proceso que termina antes de agotar su quantum (no se demueve).

## Análisis

**¿Qué ocurre si el boost es muy frecuente?**
Los procesos vuelven muy seguido a Q0 y las colas inferiores pierden
parte de su utilidad.

**¿Qué ocurre si no existe boost?**
Un proceso en una prioridad baja puede tener que esperar durante mucho
tiempo.

**¿Cómo afecta un quantum pequeño en Q0?**
Permite responder rápido a procesos nuevos, aunque puede generar más
cambios de proceso.

**¿Puede haber starvation?**
Sí. Sin mecanismos como el priority boost, procesos de prioridad baja
pueden quedar esperando demasiado tiempo.
