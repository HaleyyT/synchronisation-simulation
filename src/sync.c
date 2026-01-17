#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "sim.h"

// Global instance
Sim sim;

static void init_group_cond_arrays(int M) {
    sim.cond_group_called = malloc(sizeof(*sim.cond_group_called) * M);
    sim.cond_all_enter    = malloc(sizeof(*sim.cond_all_enter)    * M);
    //sim.cond_group_done   = malloc(sizeof(*sim.cond_group_done)   * M);
    sim.cond_completed_excercise = malloc(sizeof(*sim.cond_completed_excercise) * M);
    if (!sim.cond_group_called || !sim.cond_all_enter || !sim.cond_completed_excercise) {
        fprintf(stderr, "allocation for group condvars failed\n");
        exit(1);
    }
    for (int i = 0; i < sim.M; i++) {
        pthread_cond_init(&sim.cond_group_called[i], NULL);
        pthread_cond_init(&sim.cond_all_enter[i],    NULL);
        //pthread_cond_init(&sim.cond_group_done[i],   NULL);
        pthread_cond_init(&sim.cond_completed_excercise[i],   NULL);
    }

    // add to init_group_cond_arrays(int M):
    sim.cond_all_left = malloc(sizeof(*sim.cond_all_left) * M);
    if (!sim.cond_all_left) { fprintf(stderr, "allocation for group condvars failed\n"); exit(1); }
    for (int i = 0; i < sim.M; i++) {
        pthread_cond_init(&sim.cond_all_left[i], NULL);
    }
}

static void destroy_group_cond_arrays(int M) {
    if (sim.cond_group_called) {
        for (int i = 0; i < M; i++) pthread_cond_destroy(&sim.cond_group_called[i]);
        free(sim.cond_group_called);
        sim.cond_group_called = NULL;
    }

    if (sim.cond_all_enter) {
        for (int i = 0; i < M; i++) pthread_cond_destroy(&sim.cond_all_enter[i]);
        free(sim.cond_all_enter);
        sim.cond_all_enter = NULL;
    }

    if (sim.cond_completed_excercise) {
    for (int g = 0; g < M; g++)
        pthread_cond_destroy(&sim.cond_completed_excercise[g]);
    free(sim.cond_completed_excercise);
    sim.cond_completed_excercise = NULL;

    // add to destroy_group_cond_arrays(int M):
    if (sim.cond_all_left) {
        for (int i = 0; i < M; i++) pthread_cond_destroy(&sim.cond_all_left[i]);
        free(sim.cond_all_left);
        sim.cond_all_left = NULL;
    }
}


}

/**
 * Initialise the lab elements with memory allocation and null check
*/
void init_sync(void) {
    // copy params
    sim.N = N; sim.M = M; sim.K = K; sim.T = T;

    // allocate arrays
    sim.group_id            = malloc(sizeof(int) * sim.N);
    sim.group_size          = malloc(sizeof(int) * sim.M);
    sim.room_of_group       = malloc(sizeof(int) * sim.M);
    sim.entered_room        = malloc(sizeof(int) * sim.M);
    sim.group_left_in_room  = malloc(sizeof(int) * sim.M);
    sim.tutor_queue         = malloc(sizeof(int) * sim.K);
    sim.tutor_current_group = malloc(sizeof(int) * sim.K);
    // in init_sync(), after allocating other per-group arrays:
    sim.left_count = malloc(sizeof(int) * sim.M);
    if (!sim.left_count) { fprintf(stderr, "Allocation failed\n"); exit(1); }
    for (int g = 0; g < sim.M; g++) sim.left_count[g] = 0;

    if (!sim.group_id || !sim.group_size || !sim.room_of_group ||
        !sim.entered_room || !sim.group_left_in_room || !sim.tutor_queue || !sim.tutor_current_group) {
        fprintf(stderr, "Allocation failed\n");
        exit(1);
    }

    // scalars
    sim.arrived_count = 0;
    sim.assigned_count = 0;
    sim.finished_groups = 0;
    sim.next_group_id = 0;
    sim.qhead = sim.qtail = sim.qcount = 0;
    sim.student_ready_count= 0;
    sim.all_done = 0;
    sim.students_leaving = 0;
    sim.tutor_leaving = 0;
    sim.groups_done = 0;

    // init arrays
    for (int i = 0; i < sim.N; i++) sim.group_id[i] = -1;
    
    for (int g = 0; g < sim.M; g++) {
        sim.group_size[g] = 0;
        sim.room_of_group[g] = -1;
        sim.entered_room[g] = 0;
        sim.group_left_in_room[g] = 0;
    }

    for (int t = 0; t < sim.K; t++){
        sim.tutor_current_group[t] = -1;
    }

    // sync primitives
    pthread_mutex_init(&sim.mutex, NULL);
    pthread_cond_init(&sim.cond_all_arrived, NULL);
    pthread_cond_init(&sim.cond_assigned,    NULL);
    pthread_cond_init(&sim.cond_tutor_queue, NULL);
    pthread_cond_init(&sim.cond_tutor_assigned, NULL);
    pthread_cond_init(&sim.cond_student_ready, NULL);
    pthread_cond_init(&sim.cond_lab_vacant ,NULL);
    pthread_cond_init(&sim.cond_can_start_excercise, NULL);
    pthread_cond_init(&sim.cond_students_leave, NULL);
    pthread_cond_init(&sim.cond_tutor_leave, NULL);
    pthread_cond_init(&sim.cond_group_done, NULL);

    init_group_cond_arrays(sim.M);

    // rng
    srand((unsigned)time(NULL));

    //printf("Synchronization + model initialized.\n");
}

void destroy_sync(void) {
    destroy_group_cond_arrays(sim.M);

    pthread_cond_destroy(&sim.cond_tutor_queue);
    pthread_cond_destroy(&sim.cond_assigned);
    pthread_cond_destroy(&sim.cond_all_arrived);
    pthread_cond_destroy(&sim.cond_tutor_assigned);
    pthread_mutex_destroy(&sim.mutex);
    pthread_cond_destroy(&sim.cond_student_ready);
    pthread_cond_destroy(&sim.cond_lab_vacant);
    pthread_cond_destroy(&sim.cond_can_start_excercise);
    pthread_cond_destroy(&sim.cond_students_leave);
    pthread_cond_destroy(&sim.cond_tutor_leave);
    pthread_cond_destroy(&sim.cond_group_done);

    free(sim.tutor_queue);         sim.tutor_queue = NULL;
    free(sim.group_left_in_room);  sim.group_left_in_room = NULL;
    free(sim.entered_room);        sim.entered_room = NULL;
    free(sim.room_of_group);       sim.room_of_group = NULL;
    free(sim.group_size);          sim.group_size = NULL;
    free(sim.group_id);            sim.group_id = NULL;
    free(sim.tutor_current_group); sim.tutor_current_group = NULL;
    free(sim.left_count); sim.left_count = NULL;

    //printf("Synchronization + model destroyed.\n");
}

int rand_duration_inclusive(int Tmax) {
    int lo = Tmax / 2;
    int hi = Tmax;
    if (hi < lo) hi = lo;
    return lo + (rand() % (hi - lo + 1));
}
