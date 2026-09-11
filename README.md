# Laboratorio 1 - MLFQ

**Estudiante:** David Arango Pineda - Juan Pablo Herrera - Santiago Palacio

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
├── main.c        Escenario principal, CLI y salida por consola
├── process.h     Struct Process y fórmulas de métricas
├── queue.c/h     Cola FIFO simple (arreglo circular)
├── scheduler.c/h Simulación MLFQ (el corazón del laboratorio)
├── csv.c/h       Generación de results.csv
├── experiments.c Corridas comparativas para la sección de análisis
├── tests.c       Pruebas mínimas con assert()
├── Makefile      Compilación para Windows y Linux
├── README.md
├── DESIGN.md     Decisiones de diseño, principios y patrones
├── .gitignore
└── results.csv   Salida de la última ejecución
```

Cada archivo tiene una única responsabilidad: `process` describe los
datos, `queue` implementa la estructura de datos, `scheduler` decide qué
proceso corre, y `csv` exporta resultados. Las decisiones de diseño y los
principios aplicados están explicados en [DESIGN.md](DESIGN.md).

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

### Opciones de línea de comandos

```
mlfq [-v] [-b N]
  -v, --verbose   imprime la traza ciclo a ciclo
  -b, --boost N   intervalo del priority boost (por defecto 20)
  -h, --help      muestra esta ayuda
```

`-v` muestra qué proceso ejecuta en cada ciclo y en qué cola:

```
.\mlfq.exe -v
[t= 0] P1 ejecuta en Q0 (restante=8)
[t= 1] P1 ejecuta en Q0 (restante=7)
[t= 2] P2 ejecuta en Q0 (restante=4)
...
```

`-b N` permite comparar intervalos de boost sin recompilar; es lo que
usa la sección de análisis.

### Experimentos

```
mingw32-make run-experiments     # Windows
make run-experiments             # Linux
```

Corre el mismo scheduler sobre varios escenarios e intervalos de boost y
produce las tablas de la sección de análisis.

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

Las decisiones no triviales del simulador —por qué el proceso en CPU vive
fuera de las colas, por qué la regla de democión es intercambiable, por
qué el dominio no incluye `stdio.h`— están explicadas y justificadas en
**[DESIGN.md](DESIGN.md)**, junto con los principios (SRP, OCP, DIP) y los
patrones aplicados.

## Pruebas

`tests.c` usa `assert()` de la biblioteca estándar (sin frameworks) y
cubre 11 casos:

- cálculo de response, turnaround y waiting time;
- democión Q0 → Q1 y Q1 → Q2;
- permanencia en Q2 al agotar el quantum allí;
- priority boost devolviendo un proceso a Q0;
- un proceso que termina antes de agotar su quantum (no se demueve);
- validación: burst inválido, boost inválido y entrada correcta,
  verificando el código devuelto y el dato que identifica el problema.

## Análisis

Las respuestas salen de correr el propio simulador, no de la intuición.
`experiments.c` ejecuta el mismo scheduler con distintos intervalos de
boost y imprime las tablas de abajo:

```
mingw32-make run-experiments     # Windows
make run-experiments             # Linux
```

Se usa `boost = 1000` para representar **no tener boost**: es mayor que
la duración de ambos escenarios, así que nunca llega a dispararse.

### Escenario del enunciado (P1–P4)

| boost | response | turnaround | waiting | espera continua máx. de P1 | último ciclo |
|-------|----------|------------|---------|----------------------------|--------------|
| 2     | 2.00     | 19.25      | 12.75   | 6                          | 26           |
| 3     | 2.25     | 19.50      | 13.00   | 9                          | 26           |
| 5     | 1.75     | 20.25      | 13.75   | 7                          | 26           |
| 10    | 1.50     | 19.25      | 12.75   | 6                          | 26           |
| 20    | 1.50     | 19.50      | 13.00   | 9                          | 26           |
| 1000  | 1.50     | 19.50      | 13.00   | 9                          | 26           |

El primer hallazgo es incómodo pero real: **con boost 20 los resultados
son idénticos a no tener boost**. Este escenario dura 26 ciclos, solo
dispara un boost (en t=20) y para entonces ya no llegan procesos nuevos,
así que las colas se vacían en el mismo orden con o sin él. El boost sí
cambia *en qué cola* ejecutan P1 y P3 al final (Q0 en vez de Q2), pero
no cambia *cuándo* terminan.

Para ver el efecto real del boost hace falta un escenario con contención
sostenida.

### Escenario de contención (1 proceso de burst 20 + 24 cortos de burst 2)

Un proceso corto llegando cada 2 ciclos satura exactamente la CPU: Q0
nunca se vacía.

| boost | response | turnaround | waiting | espera continua máx. del largo | último ciclo |
|-------|----------|------------|---------|--------------------------------|--------------|
| 2     | 7.04     | 11.68      | 8.96    | 12                             | 68           |
| 3     | 7.04     | 11.68      | 8.96    | 12                             | 68           |
| 5     | 6.40     | 11.04      | 8.32    | 14                             | 68           |
| 10    | 3.68     | 8.32       | 5.60    | 10                             | 68           |
| 20    | 1.44     | 6.08       | 3.36    | 20                             | 68           |
| 1000  | 0.00     | 4.64       | 1.92    | **48**                         | 68           |

### ¿Puede haber starvation?

Sí, y aquí está medida. La columna clave es la **espera continua máxima**:
cuántos ciclos seguidos pasa el proceso largo sin tocar la CPU.

Sin boost son **48 ciclos seguidos** sin ejecutar. Con boost cada 20, esa
espera queda **acotada a 20**: el boost funciona exactamente como un techo
de starvation. Es la justificación de por qué el mecanismo existe.

Lo que el boost **no** cambia es el último ciclo: 68 en todos los casos.
La CPU nunca está ociosa, así que el trabajo total es el mismo; MLFQ
redistribuye quién espera, no reduce cuánto hay que ejecutar.

### ¿Qué ocurre si el boost es muy frecuente?

Empeora a los procesos cortos: su waiting promedio sube de 1.92 (sin
boost) a 8.96 (boost cada 2 ciclos), porque el proceso largo vuelve
constantemente a Q0 a competir con ellos y las colas bajas dejan de
cumplir su función de separar lo corto de lo largo.

Y hay un efecto contraintuitivo: con boost 2 la espera continua máxima
del largo es **12**, peor que los 10 del boost cada 10 ciclos. La razón es
que el boost reencola en Q0 **al final**, así que boostear muy seguido
mete al proceso largo detrás de toda la fila de cortos una y otra vez.
Más boost no es automáticamente más justo.

### ¿Qué ocurre si no existe boost?

Los procesos cortos obtienen el mejor resultado posible (waiting promedio
1.92, response 0.00) a costa del largo, que queda relegado 48 ciclos
seguidos. Sin boost, MLFQ optimiza el caso promedio y abandona el peor
caso.

### ¿Cómo afecta un quantum pequeño en la cola de mayor prioridad?

Q0 usa quantum 2, el más pequeño de los tres. Eso hace que un proceso
nuevo obtenga CPU muy rápido (response promedio de 1.50 en el escenario
del enunciado, y 0.00 para los cortos del escenario de contención,
porque cada corto entra a Q0 y ejecuta de inmediato).

El costo es más cambios de proceso y más demociones: en el escenario del
enunciado los cuatro procesos son demovidos de Q0 tras solo 2 ciclos,
aunque a tres de ellos les faltaba poco para terminar. Un quantum de Q0
más grande reduciría ese trasiego a cambio de peor response time.
