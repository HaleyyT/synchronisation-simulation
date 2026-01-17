#ifndef SIM_H
#define SIM_H

#include <pthread.h>

// Shared simulation parameters
extern int N; // total number of students
extern int M; // number of groups
extern int K; // number of tutors/rooms
extern int T; // max time per group


// The whole simulation's shared state.
// One global instance (defined in sync.c) is exposed as `sim`.
typedef struct {
    int N, M, K, T;

    // ---- Per-student / per-group state ----
    int *group_id;              // [N] set by teacher; -1 until assigned
    int *group_size;            // [M] computed from N, M
    int *room_of_group;         // [M] which room a group was assigned to (-1 if not yet)
    int *entered_room;          // [M] how many students of group g have entered
    int *group_left_in_room;    // [M] how many of group g have left

    // ---- Global counters / progress ----
    int arrived_count;      // how many students have arrived
    int assigned_count;     // how many students assigned a group
    int finished_groups;    // how many groups have fully completed
    int next_group_id;       // teacher calls groups in natural order 0..M-1
    int student_ready_count; // counter for how many students have been finalised in group
    int all_done;      //student have been finalised into groups
    int students_leaving;  // count of students that finished excercise and left
    int tutor_leaving;      // count of students that finished excercise and left
    int groups_done;        // count of groups that finish excercise

    int *left_count;                 // per-group: how many students have left the room
    pthread_cond_t *cond_all_left;   // per-group: signaled when a group's students have all left


    // ---- Tutor FIFO queue (by tutor id) ----
    int *tutor_queue;          // circular buffer storage
    int *tutor_current_group;             // [K] -1 until teacher assigns a group to tutor tid
    pthread_cond_t cond_tutor_assigned;   // tutors wake when their gid is set
    int qhead, qtail, qcount;  // head, tail, count 

    // ---- Synchronization primitives ----
    pthread_mutex_t mutex;

    // Teacher waits for all students to arrive
    pthread_cond_t cond_all_arrived;

    // Students wait until they've been assigned (teacher can broadcast)
    pthread_cond_t cond_assigned;

    // One set of condvars per group (arrays of size M)
    pthread_cond_t *cond_group_called; // A certain group is allowed to enter the room 
    pthread_cond_t *cond_all_enter;    //all students in a certain group have entered
    pthread_cond_t cond_group_done;   //tutor signal that the group is finish 

    // Students were finalised in groups and now ready to enter the labb rooms
    pthread_cond_t cond_student_ready;
    
    // Tutors wait here when (re)joining the FIFO
    pthread_cond_t cond_tutor_queue;

    // Lab is vacant
    pthread_cond_t cond_lab_vacant;

    // Lab is available and Teacher signal students they can start excercise
    pthread_cond_t cond_can_start_excercise;

     // Tutor signal a group has completed excecise
    pthread_cond_t *cond_completed_excercise;

    // Signal students to leave after finish lab excercise
    pthread_cond_t cond_students_leave;

    // Signal tutor to leave after lab excercise finish
    pthread_cond_t cond_tutor_leave;


} Sim;

// global instance defined in sync.c
extern Sim sim;

// Thread functions
void *teacher_thread(void *arg);
void *tutor_thread(void *arg);
void *student_thread(void *arg);

// Initialization helpers
void init_sync(void);
void destroy_sync(void);

// Utility
int rand_duration_inclusive(int Tmax);

#endif // SIM_H
