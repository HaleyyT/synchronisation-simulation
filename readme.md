## Synchronization Simulation (C / pthreads)

A multithreaded OS-style synchronization simulation in C using POSIX pthreads. The program models a classroom “group lab exercise” workflow with students, a teacher, and tutors/lab rooms, focusing on correct coordination, fairness, and deadlock-free execution. 


## Highlights

**Real concurrency coordination** (not toy threads): multiple roles (teacher / tutors / students) interacting with shared state and strict ordering constraints.

**Fairness + determinism under concurrency:**

teacher calls groups in increasing group-id order (0 → M-1) when rooms are available 


tutors/lab rooms become available in a FIFO queue so the “first vacated room is assigned first” 


Correct use of pthread primitives: built only with mutexes + condition variables (no semaphores or other sync mechanisms). 


**Traceable behavior:** prints a clear, step-by-step event log so correctness can be verified from the output (arrivals → group assignment → room entry → exercise → leaving → shutdown). 



## Simulation model

The simulation follows this scenario:

There are N students divided into M groups.

A teacher thread waits for all students to arrive, then assigns each student to a group (students do not pick their own group).

There are K lab rooms, each with a tutor thread.

A room can host only one group at a time; groups enter only when the teacher calls them.

Each group’s lab time is randomly chosen between T/2 and T “time units” (1 unit = 1 second). 



## Build & run
```bash
Build
make

Run
./sim
```

**You will be prompted for:**
N: number of students

M: number of groups

K: number of tutors / lab rooms

T: max time units per group 



Example:
```text
=== Synchronization Simulation ===
Enter N (students): 5
Enter M (groups): 3
Enter K (tutors/rooms): 6
Enter T (max time units): 1
Simulation starting with N=5, M=3, K=6, T=1.
```
**Output trace (what to look for)**

The program prints specific status messages from teacher, students, and tutors to make the synchronization observable, including:

teacher waiting for arrivals

students arriving + waiting for assignment

teacher assigning group IDs

teacher waiting for room availability

tutors reporting vacated rooms

students entering when their group is called

tutor confirming all students in the group have entered before starting

students leaving and tutors going home when no groups remain

main thread termination message 



## Concurrency design (high level)

Key synchronization points the program enforces:

**Arrival barrier:** teacher cannot assign groups until all students have arrived.

**Group assignment handoff:** students block until the teacher assigns their group id.

**Room admission control:** students in a group only enter when:

their group is the next eligible group in order, and a tutor/lab room is available.

**Group-as-a-unit execution:** a tutor starts the exercise only after all students in that group have entered. 


Vacate → FIFO availability: after a group leaves, the tutor reports availability and joins a FIFO queue for reassignment. 

Clean shutdown: once no students are waiting, teacher dismisses tutors, then exits; main thread prints the final message. 



## Tech stack

Language: C (C11)

Concurrency: POSIX pthreads, mutex, condition variables 

Build: Makefile

## What I learned 

Designing synchronization protocols from a written spec (ordering constraints, fairness, bounded capacity).

Structuring shared state + condition variables to avoid:

- Missed wakeups
- Race conditions
- Deadlocks
- Starvation (via ordered group calling + FIFO room assignment).