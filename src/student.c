#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "sim.h"

/**
 * A function that allows student threads to acquire and work
 * The behaviours are waiting to be assigned into group, entering and leaving the room
*/
void *student_thread(void *arg) {
    int id = *(int *)arg;
    // Inform arrival 
    printf("Student [%d]: I have arrived and wait for being assigned to a group.\n", id);

    pthread_mutex_lock(&sim.mutex);

    // Contribute to "all arrived" barrier; last arrival wakes teacher
    sim.arrived_count++;
    if (sim.arrived_count == sim.N) {
        pthread_cond_signal(&sim.cond_all_arrived);
    }

    // Wait for teacher to assign a group:
    // Predicate: sim.group_id[id] != -1
    while (sim.group_id[id] == -1) {
        pthread_cond_wait(&sim.cond_assigned, &sim.mutex);
    }

    int group_id = sim.group_id[id];
    pthread_mutex_unlock(&sim.mutex);

    // Announce assignment 
    printf("Student [%d]: OK, I’m in group [%d] and waiting for my turn to enter a lab room.\n",
           id, group_id);

    // Mark "ready" after assignment, then wait for room call
    pthread_mutex_lock(&sim.mutex);
    sim.student_ready_count++;
    if (sim.student_ready_count == sim.N) {
        pthread_cond_signal(&sim.cond_student_ready);
    }

    // Wait until this group is called into a room
    while (sim.room_of_group[group_id] == -1) {
        pthread_cond_wait(&sim.cond_group_called[group_id], &sim.mutex);
    }
    int room_id = sim.room_of_group[group_id];

    // Inform entering the room (after being called)
    printf("Student [%d] in group [%d]: My group is called. I will enter the lab room [%d] now.\n",
        id, group_id, room_id);

    // Count entrants; last entrant signals "all entered" for this group
    sim.entered_room[group_id]++;
    if (sim.entered_room[group_id] == sim.group_size[group_id]) {
        pthread_cond_signal(&sim.cond_all_enter[group_id]);
    }
    pthread_mutex_unlock(&sim.mutex);

    // Wait until tutor signals exercise completion for this group:
    pthread_mutex_lock(&sim.mutex);
    while (!sim.group_left_in_room[group_id]) {
        pthread_cond_wait(&sim.cond_completed_excercise[group_id], &sim.mutex);
    }

    printf("Student [%d] in group [%d]: Thanks Tutor [%d]. Bye!\n", id, group_id, sim.room_of_group[group_id]);

    // Contribute to global "students leaving" count
    sim.students_leaving++;

    sim.left_count[group_id]++;                              
    if (sim.left_count[group_id] == sim.group_size[group_id])
        pthread_cond_signal(&sim.cond_all_left[group_id]);  

    //signal tutor that studnents in the group have all left 
    if (sim.students_leaving == sim.N) {
        pthread_cond_signal(&sim.cond_students_leave);
    }
    pthread_mutex_unlock(&sim.mutex);

    return NULL;
}
