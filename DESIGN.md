# Decisiones de diseño

Documento de diseño del simulador MLFQ. Explica las decisiones no
triviales, qué principios se aplicaron y dónde, qué patrón se usó y por
qué, y qué se decidió **no** hacer.

**Estudiantes:** David Arango Pineda - Juan Pablo Herrera - Santiago Palacio

---

## 1. Capas y regla de dependencia

El proyecto está separado en tres capas. La regla es que las flechas
apuntan siempre hacia adentro: el dominio no sabe que existen la consola,
los archivos ni la línea de comandos.

```
        ┌──────────────────────────────────────────────┐
        │  COMPOSICIÓN                                 │
        │  main.c          CLI, consola, arranque      │
        │  experiments.c   corridas comparativas       │
        │  tests.c         pruebas                     │
        └───────────────┬──────────────────────────────┘
                        │ depende de
                        v
        ┌──────────────────────────────────────────────┐
        │  DOMINIO                                     │
        │  scheduler.c/h   política MLFQ               │
        │  queue.c/h       cola FIFO                   │
        │  process.h       proceso y métricas          │
        └───────────────┬──────────────────────────────┘
                        │ no depende de nadie
                        v
                    (nada)

        ┌──────────────────────────────────────────────┐
        │  INFRAESTRUCTURA                             │
        │  csv.c/h         escritura de results.csv    │
        └──────────────────────────────────────────────┘
             usada por main.c, nunca por el dominio
```

Esto es verificable, no declarativo: `scheduler.c` y `queue.c` **no
incluyen `stdio.h`**. Solo usan `<stddef.h>` (para `NULL`) y `<assert.h>`,
que son headers de tipos y macros, sin entrada/salida.

  Las dos consecuencias prácticas:

- **`experiments.c` reutiliza el scheduler sin tocarlo.** Compila contra
  `scheduler.c` y `queue.c` únicamente, sin arrastrar `csv.c` ni la CLI, y
  corre el mismo algoritmo sobre otros escenarios. Si el dominio imprimiera
  o escribiera archivos, no se podría.
- **Los errores viajan como datos, no como mensajes.** `validate_input()`
  devuelve un `ValidationResult` y el dato que identifica el problema;
  `main.c` decide cómo redactarlo. El dominio no elige idioma ni destino.

---

## 2. Principios aplicados

### SRP — Responsabilidad única

Un archivo, una razón para cambiar:

| Archivo | Cambia si cambia... |
|---|---|
| `process.h` | qué datos define un proceso o cómo se calcula una métrica |
| `queue.c/h` | la estructura de datos de las colas |
| `scheduler.c/h` | la política de planificación |
| `csv.c/h` | el formato del archivo de salida |
| `main.c` | la forma de invocar el programa o de presentar resultados |

Dentro de `scheduler.c` el mismo criterio se aplica por función: los seis
pasos del ciclo (`add_new_arrivals`, `apply_priority_boost`,
`preempt_if_needed`, `pick_next_process`, `default_demote`) son funciones
separadas y `run_simulation` solo las orquesta. Ninguna pasa de 20 líneas.

### OCP — Abierto a extensión, cerrado a modificación

El enunciado pide poder agregar "una regla de democión distinta" sin
modificar el núcleo. La regla de democión es un puntero a función en la
configuración:

```c
typedef void (*DemotionPolicy)(Process *p, Queue queues[NUM_QUEUES]);
```

`run_simulation` no conoce ninguna regla concreta, solo invoca la que
recibe:

```c
DemotionPolicy demote = cfg.demote ? cfg.demote : default_demote;
demote(current, queues);
```

Si `cfg.demote` es `NULL` se usa `default_demote` (Q0→Q1→Q2, se queda en
Q2). Para probar otra regla —que no reinicie el quantum, que salte dos
niveles, que use una cola adicional— se escribe una función nueva y se
asigna al campo. **No se toca una sola línea del bucle.**

### DIP — Depender de abstracciones ## Inversion de dependencias

El scheduler depende de dos *firmas*, no de implementaciones:

- `DemotionPolicy`: qué hacer al agotar el quantum.
- `TickObserver`: a quién reportar cada ciclo ejecutado.

En C la abstracción idiomática es el puntero a función: define el
contrato sin atar al llamador a una implementación. El caso de
`TickObserver` es el más claro: el scheduler quiere *reportar* lo que
ocurre, pero no debe saber si eso va a la consola (`main.c` con `-v`), a
un acumulador de estadísticas (`experiments.c` midiendo starvation) o a
ningún lado (`tests.c`, que pasa `NULL`). Los tres casos existen hoy en el
proyecto con el mismo scheduler sin modificar.

### Encapsulamiento

El encapsulamiento real de este proyecto está a nivel de módulo: las seis
funciones internas de `scheduler.c` son `static`, es decir, invisibles
fuera del archivo. La superficie pública del scheduler son tres cosas:
`SchedulerConfig`, `validate_input()` y `run_simulation()`.

Lo que **no** se hizo: esconder los campos de `Process` detrás de
getters/setters. Es una decisión consciente, no un descuido. `Process` es
un registro de datos plano —el equivalente a un struct de POD— y en C
envolverlo en accesores agrega una capa de ruido en los cuatro archivos
que lo usan sin proteger ninguna invariante real: los campos se llenan
durante la simulación y se leen al final. La invariante que sí importa
(que un proceso no desborde una cola) se protege donde ocurre, con un
`assert` en `enqueue()`.

---

## 3. Patrón aplicado: Strategy

**Qué es:** encapsular un algoritmo intercambiable detrás de una
interfaz, para poder sustituirlo sin modificar a quien lo usa.

**Dónde:** `SchedulerConfig.demote` (`DemotionPolicy`), como se explicó
en OCP.

**Por qué este y no otro:** porque el enunciado pide exactamente esa
capacidad ("agregar una nueva política de cola... o una regla de democión
distinta sin modificar el núcleo del scheduler"). El patrón entró para
resolver un requisito concreto, con el costo mínimo posible: un `typedef`,
un campo, y una línea en el bucle. Como el campo es opcional (`NULL` usa
la regla por defecto), el código que ya existía —`main.c`, `tests.c`— no
necesitó cambiar.

`TickObserver` es un **Observer** reducido a su mínima expresión: un solo
observador opcional en lugar de una lista de suscriptores. Se implementó
así, y no con una lista, porque no hay ningún caso en el proyecto que
necesite dos observadores a la vez; una lista sería estructura sin uso.

### Patrones que se descartaron

- **State** para los estados del proceso (NEW/READY/RUNNING/TERMINATED):
  el estado ya está representado sin ambigüedad por los datos existentes
  (`remaining_time`, `finished`, `current_queue`, y el hecho de ser o no
  `current`). Una jerarquía de estados con transiciones sería más código
  describiendo lo mismo.
- **Factory** para crear procesos: hay una sola fuente de procesos
  (funciones que arman escenarios fijos). Una fábrica que solo sabe
  construir de una forma es una función con un nombre más largo.
- **Observer completo** con lista de suscriptores: ver arriba.

El enunciado advierte que usar un patrón sin necesidad resta tanto como
no usarlo cuando aporta. Estos tres no tienen, hoy, un segundo caso de uso
que justifique la indirección.

---

## 4. Otras decisiones no obvias

**El proceso en CPU vive fuera de las colas.** Mientras un proceso
ejecuta, está en la variable `current`, no dentro de ninguna `Queue`. Si
se reencolara al final de su cola en cada ciclo, otro proceso del mismo
nivel tomaría el turno antes de que el primero agotara su quantum, y el
Round Robin dejaría de ser Round Robin. Las colas contienen únicamente a
quienes esperan.

**La preemption no regala quantum.** Cuando un proceso pierde la CPU
porque apareció algo de mayor prioridad, vuelve a su cola conservando su
`quantum_used`. De lo contrario, un proceso interrumpido con frecuencia
obtendría quantums efectivamente infinitos.

**El boost alcanza también al proceso en ejecución.** `apply_priority_boost`
vacía Q1 y Q2 hacia Q0 y, si hay alguien en CPU, lo reclasifica a Q0 sin
quitarle el ciclo. Se ve en la traza: un proceso puede cambiar de cola en
t=20 sin interrumpir su ejecución.

**`Queue` es un arreglo circular de tamaño fijo.** No una lista enlazada:
el número de procesos está acotado por `MAX_PROCESSES` y se conoce de
antemano, así que no hay razón para pagar asignación dinámica ni el riesgo
de fugas que trae.

**El desbordamiento de cola es un `assert`, no un mensaje.** No es un
error del usuario sino una violación de invariante: `validate_input`
garantiza `n <= MAX_PROCESSES` y un proceso solo puede estar en una cola a
la vez. Un `assert` falla ruidosamente en desarrollo y mantiene a
`queue.c` libre de dependencias de consola.

**El intervalo de boost es configurable por CLI.** No por capricho: es lo
que permite responder las preguntas de análisis con datos comparables sin
recompilar, y es la base de las tablas del README.

---

## 5. Límites conocidos

Cosas que este diseño **no** resuelve, dichas explícitamente:

- **El número de colas es fijo** (`NUM_QUEUES 3`). La regla de democión es
  intercambiable, pero agregar un cuarto nivel exige cambiar la constante
  y el tamaño del arreglo `quantum`. Hacerlo dinámico implicaría
  asignación de memoria para un caso de uso que el enunciado no pide.
- **Los procesos se definen en código**, no se leen de un archivo. El
  escenario del laboratorio es fijo, así que la validación cubre valores
  inválidos, no formatos de archivo malformados.
- **No hay I/O de procesos ni bloqueo.** Todos los procesos son CPU-bound
  puros; no existe estado WAITING porque nada los hace esperar.
