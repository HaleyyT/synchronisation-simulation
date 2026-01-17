#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "sim.h"

void *teacher_thread(void *arg) {
    (void)arg;
    printf("Teacher: I’m waiting for all students to arrive.\n");

    // Wait until all students have arrived
    pthread_mutex_lock(&sim.mutex);
    while (sim.arrived_count < sim.N) {
        pthread_cond_wait(&sim.cond_all_arrived, &sim.mutex);
    }
    pthread_mutex_unlock(&sim.mutex);

    printf("Teacher: All students have arrived. I start to assign group ids to students.\n");

    // Calculate group sizes:
    // first (N % M) groups get floor(N / M) + 1, the rest get floor(N / M)
    int base   = sim.N / sim.M;
    int excess = sim.N % sim.M;
    for (int g = 0; g < sim.M; g++) {
        sim.group_size[g] = base + (g < excess);
    }

    // Assign every student a group id and wake them so they can read it
    pthread_mutex_lock(&sim.mutex);
    int sid = 0;
    for (int g = 0; g < sim.M; g++) {
        for (int i = 0; i < sim.group_size[g]; i++, sid++) {
            sim.group_id[sid] = g;
            printf("Teacher: student [%d] is in group [%d].\n", sid, g);
        }
    }
    pthread_cond_broadcast(&sim.cond_assigned);
    pthread_mutex_unlock(&sim.mutex);

    // Wait for all students to be “ready” (they’ve printed their waiting line)
    pthread_mutex_lock(&sim.mutex);
    while (sim.student_ready_count < sim.N) {
        pthread_cond_wait(&sim.cond_student_ready, &sim.mutex);
    }
    pthread_mutex_unlock(&sim.mutex);

    printf("Teacher: I’m waiting for lab rooms to become available.\n");

    // Assign groups to tutors one-by-one, in order 0..M-1.
    // For each non-empty group g: wait for a free tutor → assign → wait tutor to publish room →
    // announce lab availability → wake that group’s students.
    pthread_mutex_lock(&sim.mutex);
    for (int g = 0; g < sim.M; g++) {
        // Skip empty groups: ignore per the spec (no tutor, no announcement)
        if (sim.group_size[g] == 0) continue;

        // Wait until a tutor is available (FIFO queue not empty)
        while (sim.qcount == 0) {
            pthread_cond_wait(&sim.cond_tutor_queue, &sim.mutex);
        }

        // Dequeue a tutor
        int tid = sim.tutor_queue[sim.qhead];
        sim.qhead = (sim.qhead + 1) % sim.K;
        sim.qcount--;

        // Assign this group to that tutor and wake tutors
        sim.tutor_current_group[tid] = g;
        pthread_cond_broadcast(&sim.cond_tutor_assigned);

        // Wait for that tutor to publish the room for this group (room_of_group[g] set)
        while (sim.room_of_group[g] == -1) {
            pthread_cond_wait(&sim.cond_lab_vacant, &sim.mutex);
        }
        int room_id = sim.room_of_group[g];

        // Print the announcement without holding the lock (keeps logs tidy)
        pthread_mutex_unlock(&sim.mutex);
        printf("Teacher: The lab [%d] is now available. Students in group [%d] can enter the room and start your lab exercise.\n",
               room_id, g);
        pthread_mutex_lock(&sim.mutex);

        // Wake exactly the students of this group to enter
        pthread_cond_broadcast(&sim.cond_group_called[g]);
    }
    pthread_mutex_unlock(&sim.mutex);

    // Wait until all non-empty groups have completed the exercise
    pthread_mutex_lock(&sim.mutex);
    int needed = 0;
    for (int g = 0; g < sim.M; g++) {
        if (sim.group_size[g] > 0) needed++;
    }
    while (sim.groups_done < needed) {
        pthread_cond_wait(&sim.cond_group_done, &sim.mutex);
    }

    // before dismissing tutors. Students signal cond_students_leave when the last one leaves.
    while (sim.students_leaving < sim.N) {
        pthread_cond_wait(&sim.cond_students_leave, &sim.mutex);
    }

    // Dismiss tutors (print once per tutor id, regardless of whether they handled a group)
    for (int t = 0; t < sim.K; t++) {
        printf("Teacher: There are no students waiting. Tutor [%d], you can go home now.\n", t);
    }

    // Tell tutors to exit their loops
    sim.all_done = 1;
    pthread_cond_broadcast(&sim.cond_tutor_assigned);
    // Wait until every tutor has printed "Bye" and incremented tutors_exited
    while (sim.tutor_leaving < sim.K) {
        pthread_cond_wait(&sim.cond_tutor_leave, &sim.mutex);
    }
    pthread_mutex_unlock(&sim.mutex);

    printf("Teacher: All students and tutors are left. I can now go home.\n");
    return NULL;
}
