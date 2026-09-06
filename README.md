# Laboratorio 1 - MLFQ

Estudiante:
Santiago Palacio Cárdenas

## Objetivo

Implementar en C un simulador de scheduler con la política **Multi-Level
Feedback Queue (MLFQ)**, con tres colas de prioridad, quantum distinto por
cola, priority boost configurable y cálculo de métricas estándar
(response time, turnaround time, waiting time).

## Cómo funciona el MLFQ en este programa

El simulador avanza el reloj **ciclo por ciclo** (tiempo discreto). En cada
ciclo `t`:

1. Se agregan a Q0 los procesos cuyo `arrival_time == t`.
2. Si `t` es múltiplo del intervalo de boost, todos los procesos que no han
   terminado vuelven a Q0 y su quantum consumido se reinicia.
3. Si hay un proceso listo en una cola de mayor prioridad que la del
   proceso en ejecución, este último es interrumpido y regresa a su propia
   cola (conservando el quantum ya consumido, sin regalarle uno nuevo).
4. Se elige la cola no vacía de mayor prioridad y se ejecuta un ciclo de
   CPU del proceso que está al frente de ella (Round Robin dentro de la
   cola).
5. Se descuenta `remaining_time` y se incrementa `quantum_used`.
6. Si el proceso termina, se registra `finish_time`. Si en cambio agotó su
   quantum sin terminar, se demueve a la siguiente cola (Q2 se queda en
   Q2, porque no existe una cola inferior).
7. El reloj avanza una unidad y se repite hasta que todos los procesos
   terminan.

### Decisión de diseño importante

Mientras un proceso tiene la CPU, se guarda en una variable `current`
**aparte** de las colas (no se saca y se vuelve a meter en cada ciclo).
Esto es necesario para que el Round Robin sea correcto: si un proceso se
reencolara en cada ciclo, otros procesos de la misma cola le quitarían el
turno antes de agotar su quantum. Las colas (`Queue`) solo contienen
procesos en espera; `current` representa al que está al frente de su cola
ejecutándose.

Cuando un proceso es interrumpido por la llegada de uno de mayor
prioridad, se reencola al final de su propia cola (con la API simple de
`enqueue`/`dequeue`, sin operaciones especiales de "insertar al frente").
En el escenario principal esto nunca afecta el resultado, porque ninguna
cola tiene más de un proceso esperando en el momento de una interrupción.

## Estructura del proyecto

```
lab1-mlfq/
├── main.c        Escenario principal y ejecución del programa
├── process.h     Struct Process y fórmulas de métricas
├── queue.h/.c    Cola FIFO simple (arreglo circular)
├── scheduler.h/.c Simulación MLFQ (el corazón del laboratorio)
├── csv.h/.c      Generación de results.csv
├── tests.c       Pruebas mínimas con assert()
├── Makefile
└── README.md
```

Cada archivo tiene una única responsabilidad: `process` describe los
datos, `queue` implementa la estructura de datos, `scheduler` decide qué
proceso corre, y `csv` solo exporta resultados. Esto es simplemente
separación de responsabilidades a nivel de archivos; no se implementan
patrones de diseño formales (no hay Factory, Strategy, interfaces
artificiales, etc.).

## Cómo compilar

```
make
```

Esto genera el ejecutable `mlfq` (o `mlfq.exe` en Windows).

## Cómo ejecutar

```
./mlfq
```

(en Windows: `mlfq.exe` o `.\mlfq.exe`)

El programa ejecuta automáticamente el escenario principal, imprime la
tabla de resultados en consola y genera `results.csv` en el directorio
actual.

## Cómo ejecutar las pruebas

```
make test
```

Esto compila y ejecuta `tests.c`, que usa `assert()` de la biblioteca
estándar. Si todo pasa, se imprime `Todas las pruebas pasaron.`.

## Explicación de Q0, Q1 y Q2

- **Q0** (prioridad alta, quantum = 2): todo proceso nuevo entra aquí.
  Turnos cortos para dar buen tiempo de respuesta a procesos interactivos
  o cortos.
- **Q1** (prioridad media, quantum = 4): procesos que ya demostraron
  necesitar más CPU de la que Q0 permite.
- **Q2** (prioridad baja, quantum = 8): procesos largos. Si agotan su
  quantum aquí, simplemente se quedan en Q2 (no hay una cola inferior).

Siempre se ejecuta la cola no vacía de mayor prioridad; dentro de una
cola, el orden es FIFO (Round Robin).

## Explicación del priority boost

Cada `boost_interval` ciclos (20 en el escenario principal), todos los
procesos que aún no han terminado —estén esperando en Q1/Q2 o
ejecutándose— vuelven a Q0 y su quantum consumido se reinicia. Esto evita
que un proceso quede indefinidamente relegado a una cola de baja
prioridad.

## Explicación de las métricas

- `response_time = first_response_time - arrival_time`
- `turnaround_time = finish_time - arrival_time`
- `waiting_time = turnaround_time - burst_time`

Estas fórmulas están implementadas como funciones pequeñas en
`process.h` y se usan tanto en la salida por consola como en `results.csv`.

## Escenario principal

| PID | Arrival | Burst |
|-----|---------|-------|
| P1  | 0       | 8     |
| P2  | 1       | 4     |
| P3  | 2       | 9     |
| P4  | 3       | 5     |

Con Q0=2, Q1=4, Q2=8 y boost=20, el resultado de la simulación es:

| PID | Arrival | Burst | Start | Finish | Response | Turnaround | Waiting |
|-----|---------|-------|-------|--------|----------|------------|---------|
| P1  | 0       | 8     | 0     | 23     | 0        | 23         | 15      |
| P2  | 1       | 4     | 2     | 14     | 1        | 13         | 9       |
| P3  | 2       | 9     | 4     | 26     | 2        | 24         | 15      |
| P4  | 3       | 5     | 6     | 21     | 3        | 18         | 13      |

## Decisiones de diseño sencillas

- `pid` se guarda como entero y se imprime con el prefijo `P` solo al
  mostrar/exportar resultados.
- `start_time` y `first_response_time` inician en `-1` para saber si un
  proceso ya corrió alguna vez; ambos se fijan la primera vez que el
  proceso obtiene la CPU.
- La cola es un arreglo circular de tamaño fijo (`MAX_PROCESSES`) porque
  el número de procesos del laboratorio es pequeño y conocido; no hace
  falta una lista enlazada.
- Se valida antes de simular: cantidad de procesos, `arrival_time >= 0`,
  `burst_time > 0`, quantum de cada cola `> 0` y `boost_interval > 0`.

## Preguntas de análisis

**1. ¿Qué ocurre si el boost es muy frecuente?**

Los procesos vuelven constantemente a la cola de mayor prioridad, por lo
que las colas inferiores pierden parte de su utilidad y los procesos
largos reciben repetidamente prioridad alta.

**2. ¿Qué ocurre si no existe boost?**

Los procesos largos pueden permanecer en las colas inferiores durante
mucho tiempo, especialmente si siguen llegando procesos nuevos.

**3. ¿Cómo afecta un quantum pequeño en la cola de mayor prioridad?**

Mejora el tiempo de respuesta porque permite alternar rápidamente entre
procesos, pero produce más cambios de contexto.

**4. ¿Puede haber starvation?**

Sí. Sin mecanismos como el priority boost, un proceso en una cola de baja
prioridad podría esperar demasiado tiempo si siguen llegando procesos de
mayor prioridad.
