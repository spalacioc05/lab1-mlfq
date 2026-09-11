#include <assert.h>
#include <stdio.h>
#include "process.h"
#include "scheduler.h"

/* Configuracion MLFQ estandar reutilizada por varias pruebas. */
static SchedulerConfig default_config(void) {
    SchedulerConfig cfg = { .quantum = {2, 4, 8}, .boost_interval = 20 };
    return cfg;
}

/* 1. Response time = first_response_time - arrival_time */
static void test_response_time(void) {
    Process p;
    process_init(&p, 1, 3, 5);
    p.first_response_time = 3;
    assert(response_time(&p) == 0);

    p.first_response_time = 7;
    assert(response_time(&p) == 4);
}

/* 2. Turnaround time = finish_time - arrival_time */
static void test_turnaround_time(void) {
    Process p;
    process_init(&p, 1, 2, 8);
    p.finish_time = 25;
    assert(turnaround_time(&p) == 23);
}

/* 3. Waiting time = turnaround_time - burst_time */
static void test_waiting_time(void) {
    Process p;
    process_init(&p, 1, 0, 8);
    p.finish_time = 25;
    assert(waiting_time(&p) == turnaround_time(&p) - p.burst_time);
    assert(waiting_time(&p) == 17);
}

/*
 * 4. Democion Q0 -> Q1: burst=3 agota el quantum de Q0 (2) sin terminar,
 * asi que se demueve a Q1 y termina alli. Al final debe quedar en Q1.
 */
static void test_demotion_q0_to_q1(void) {
    Process procs[1];
    process_init(&procs[0], 1, 0, 3);
    run_simulation(procs, 1, default_config());

    assert(procs[0].finished == 1);
    assert(procs[0].current_queue == 1);
}

/*
 * 5. Democion Q1 -> Q2: burst=7 agota Q0 (2) y luego Q1 (4) sin terminar,
 * asi que termina el ultimo ciclo en Q2.
 */
static void test_demotion_q1_to_q2(void) {
    Process procs[1];
    process_init(&procs[0], 1, 0, 7);
    run_simulation(procs, 1, default_config());

    assert(procs[0].finished == 1);
    assert(procs[0].current_queue == 2);
}

/*
 * 6. Un proceso en Q2 se queda en Q2 al agotar su quantum: burst=17 agota
 * Q0(2)+Q1(4)+Q2(8) una vez sin terminar y sigue en Q2 (no existe Q3).
 */
static void test_q2_stays_in_q2(void) {
    Process procs[1];
    process_init(&procs[0], 1, 0, 17);
    run_simulation(procs, 1, default_config());

    assert(procs[0].finished == 1);
    assert(procs[0].current_queue == 2);
}

/*
 * 7. Priority boost devuelve procesos a Q0: con boost_interval=3 y burst=5,
 * el proceso se demueve una vez a Q1 pero el boost lo regresa a Q0 antes
 * de terminar, asi que termina en Q0.
 */
static void test_priority_boost_returns_to_q0(void) {
    Process procs[1];
    process_init(&procs[0], 1, 0, 5);

    SchedulerConfig cfg = { .quantum = {2, 4, 8}, .boost_interval = 3 };
    run_simulation(procs, 1, cfg);

    assert(procs[0].finished == 1);
    assert(procs[0].current_queue == 0);
    assert(procs[0].finish_time == 5);
}

/*
 * 8. Un proceso que termina antes de agotar su quantum no es demovido:
 * burst=1 es menor que el quantum de Q0 (2), por lo que termina en Q0.
 */
static void test_no_demotion_if_finishes_early(void) {
    Process procs[1];
    process_init(&procs[0], 1, 0, 1);
    run_simulation(procs, 1, default_config());

    assert(procs[0].finished == 1);
    assert(procs[0].current_queue == 0);
    assert(procs[0].finish_time == 1);
}

/*
 * 9. La validacion ya no imprime: devuelve un codigo y deja en detail el dato
 * que identifica el problema (aqui, el pid del proceso con burst invalido).
 */
static void test_validation_reports_invalid_burst(void) {
    Process procs[2];
    process_init(&procs[0], 1, 0, 5);
    process_init(&procs[1], 2, 0, 0); /* burst_time invalido */

    SchedulerConfig cfg = default_config();
    int detail = 0;

    assert(validate_input(procs, 2, &cfg, &detail) == VALIDATION_BURST_TIME);
    assert(detail == 2);
}

/* 10. Un boost_interval invalido (puede llegar por CLI con -b) se rechaza. */
static void test_validation_reports_invalid_boost(void) {
    Process procs[1];
    process_init(&procs[0], 1, 0, 4);

    SchedulerConfig cfg = default_config();
    cfg.boost_interval = 0;
    int detail = 0;

    assert(validate_input(procs, 1, &cfg, &detail) == VALIDATION_BOOST_INTERVAL);
}

/* 11. Una entrada correcta pasa sin codigo de error ni detalle asociado. */
static void test_validation_accepts_valid_input(void) {
    Process procs[1];
    process_init(&procs[0], 1, 0, 4);

    SchedulerConfig cfg = default_config();
    int detail = 0;

    assert(validate_input(procs, 1, &cfg, &detail) == VALIDATION_OK);
    assert(detail == -1);
}

int main(void) {
    test_response_time();
    test_turnaround_time();
    test_waiting_time();
    test_demotion_q0_to_q1();
    test_demotion_q1_to_q2();
    test_q2_stays_in_q2();
    test_priority_boost_returns_to_q0();
    test_no_demotion_if_finishes_early();
    test_validation_reports_invalid_burst();
    test_validation_reports_invalid_boost();
    test_validation_accepts_valid_input();

    printf("Todas las pruebas pasaron.\n");
    return 0;
}
