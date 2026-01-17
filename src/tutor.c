#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "sim.h"

/**
 * FIFO tutor queue using circular buffer logic
*/
static void fifo_enqueue_tutor(int id) {
    sim.tutor_queue[sim.qtail] = id;             //assign the tutor in the tutor queue an id 
    sim.qtail = (sim.qtail + 1) % sim.K;         //FIFO logic for circular buffer to ensure wrap around
    sim.qcount++;
    pthread_cond_signal(&sim.cond_tutor_queue);  //signal for tutor to assign lab room to group 
}

/**
 * Tutor logic (available tutors, room management, FIFO queue)
*/
void *tutor_thread(void *arg) {
    int tid = *(int *)arg;

    // make this tutor available initially
    pthread_mutex_lock(&sim.mutex);
    fifo_enqueue_tutor(tid);
    pthread_mutex_unlock(&sim.mutex);

    while (1) {
        pthread_mutex_lock(&sim.mutex);

        // early exit if teacher already dismissed tutors
        if (sim.all_done) {
            pthread_mutex_unlock(&sim.mutex);
            printf("Tutor [%d]: Thanks Teacher. Bye!\n", tid);

            // Ordering guarantee for teacher final line:
            pthread_mutex_lock(&sim.mutex);
            sim.tutor_leaving++;
            pthread_cond_signal(&sim.cond_tutor_leave);
            pthread_mutex_unlock(&sim.mutex);
            return NULL;
        }

        // wait to be assigned a group
        while (sim.tutor_current_group[tid] == -1 && !sim.all_done) {
            pthread_cond_wait(&sim.cond_tutor_assigned, &sim.mutex);
        }

        if (sim.all_done) {
            pthread_mutex_unlock(&sim.mutex);
            printf("Tutor [%d]: Thanks Teacher. Bye!\n", tid);

            pthread_mutex_lock(&sim.mutex);
            sim.tutor_leaving++;
            pthread_cond_signal(&sim.cond_tutor_leave);
            pthread_mutex_unlock(&sim.mutex);
            return NULL;
        }

        // got a group
        int gid = sim.tutor_current_group[tid];

        // publish the room for this group and let teacher announce
        sim.room_of_group[gid] = tid; // mark assigned room
        printf("Tutor [%d]: The lab room [%d] is vacated and ready for one group.\n", tid, tid);

        // wake teacher that a lab is vacant for this group
        pthread_cond_broadcast(&sim.cond_lab_vacant);

        pthread_mutex_unlock(&sim.mutex);

        //Wait for all students in this group to enter
        pthread_mutex_lock(&sim.mutex);
        while (sim.entered_room[gid] < sim.group_size[gid]) {
            pthread_cond_wait(&sim.cond_all_enter[gid], &sim.mutex);
        }
        pthread_mutex_unlock(&sim.mutex);

        // Tutor announce students can start exercise
        printf("Tutor [%d]: All students in group [%d] have entered the room [%d]. You can start your exercise now.\n",
               tid, gid, tid);
        sleep(sim.T);

        // Excercise is completed and wake the group
        printf("Tutor [%d]: Students in group [%d] have completed the lab exercise in %d units of time. You may leave this room now.\n",
               tid, gid, sim.T);

        pthread_mutex_lock(&sim.mutex);
        sim.group_left_in_room[gid] = 1;
        pthread_cond_broadcast(&sim.cond_completed_excercise[gid]);

        // wait until every student in this group has left
        while (sim.left_count[gid] < sim.group_size[gid]) {
            pthread_cond_wait(&sim.cond_all_left[gid], &sim.mutex);
        }

        // group finished; report to teacher
        sim.groups_done++;
        pthread_cond_signal(&sim.cond_group_done);

        // free this tutor from the current group
        sim.tutor_current_group[tid] = -1;

        // reset per-group state if you want to reuse indexes (optional, harmless)
        sim.entered_room[gid] = 0;
        sim.group_left_in_room[gid] = 0;
        sim.left_count[gid] = 0;
        // room_of_group[gid] will be set on next assignment

        // re-enqueue this tutor to serve another group (supports K < M)
        fifo_enqueue_tutor(tid);

        pthread_mutex_unlock(&sim.mutex);
    }

    return NULL;
}
