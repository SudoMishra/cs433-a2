# CS433 aassignment 2
## Synchronization of Parallel Programs using Locks and Barriers
This repository contains code for implementing Locks and Barriers in parallel programs using various algorithms.

`sync_lib.c` contains all algorithms in one place.

Additionally lock algorithms are present under `Locks` Folder and barrier algorithms under `Barriers` Folder.

## Locks
`Locks` contains 8 different lock algorithms. We have implemented the following locks:
- lock_omp_critical.c - Lock using `#pragma omp critical` of OpenMP
- lock_posix.c - Lock using `pthread mutex` locks of POSIX Threads
- lock_semaphores.c - Lock using `binary semaphores` implemented using POSIX Threads
- lock_spin.c - Spin Lock using `cmpxchg`
- lock_lamport.c - Lamport Bakery lock algorithm using POSIX Threads
- TTS.c - Test Test and Set lock algorithm using `cmpxchg` with POSIX Threads
- lock_ticket.c - Ticket Lock using `fetch and increment` implemented using `cmpxchg` with POSIX Threads
- lock_array.c - Array Lock using `fetch and increment` implemented using `cmpxchg` with POSIX Threads

## Barriers
`Barriers` contains 6 different barrier algorithms. We have implemented the following barriers:
- Centralised_Barrier.c : Centralised Barrier with sense reversal implemented using `pthread mutex` and POSIX Threads
- Barrier_Posix_interface.c : Barrier using the `pthread_barrier` of POSIX Threads
- Posix_conditional.c : Barrier using POSIX conditional variables
- Tree_Busy_Wait.c : Tree Barrier using busy wait flags and POSIX Threads
- Tree_CV.c : Tree Barrier using POSIX condtional variables
- barrier_omp.c : Barrier using `#pragma omp barrier` of OpenMP
## Compiling and Running Code
Compile the code using:
To run any lock/barrier algorithm just uncomment the corresponding `main_{}` function in the `main()` of `sync_lib.c`
### To compile `sync_lib`
`gcc -O0 -pthread -fopenmp sync_lib.c -o sync_lib`
### Running the code
`./sync_lib N` where `N` is the number of threads to run in the program.
