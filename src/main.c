#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "sim.h"

// Global parameters
int N, M, K, T;

int main(void) {
    printf("=== Synchronization Simulation ===\n");

    // Get parameters from user
    printf("Enter N (students): ");
    scanf("%d", &N);
    printf("Enter M (groups): ");
    scanf("%d", &M);
    printf("Enter K (tutors/rooms): ");
    scanf("%d", &K);
    printf("Enter T (max time units): ");
    scanf("%d", &T);

    printf("Simulation starting with N=%d, M=%d, K=%d, T=%d\n",
           N, M, K, T);

    // Initialize synchronization primitives
    init_sync();

    pthread_t teacher_th;
    pthread_t *student_th = malloc(sizeof(*student_th) * N);
    int *student_id = malloc(sizeof(*student_id) * N);
    pthread_t *tutor_th = malloc(sizeof(*tutor_th) * K);
    int *tutor_id = malloc(sizeof(*tutor_id) * K);

    // TODO: create teacher, tutor, and student threads
    pthread_create(&teacher_th, NULL, teacher_thread, NULL);

    // create student threads
    for (int i = 0; i < N; i++) {
        student_id[i] = i;
        pthread_create(&student_th[i], NULL, student_thread, &student_id[i]);
    }

    
    // create tutor threads
    for (int i = 0; i < K; i++) {
        tutor_id[i] = i; 
        pthread_create(&tutor_th[i], NULL, tutor_thread, &tutor_id[i]);
    }
    

    // join students
    for (int i = 0; i < N; i++) {
        pthread_join(student_th[i], NULL);
    }

    // join teacher
    pthread_join(teacher_th, NULL);

    
    // join tutors
    for (int i = 0; i < K; i++) {
        pthread_join(tutor_th[i], NULL);
    }

    // free the threads
    free(student_th);
    free(student_id);
    free(tutor_th);
    free(tutor_id);

    // Clean up synchronization
    destroy_sync();

    printf("Main thread: All students have completed their lab exercises. This is the end of simulation.\n");
    return 0;
}
