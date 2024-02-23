#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

const int sh_key = 205569;
const int sh_size = sizeof(struct timespec);

int main(int argc, char *argv[]) {
    int shm_id;
    struct timespec *system_clock;

    shm_id = shmget(sh_key, sh_size, 0666);
    if (shm_id == -1) {
        perror("Error getting shared memory");
        return 1;
    }

    system_clock = (struct timespec *)shmat(shm_id , NULL, 0);
    if (system_clock == (struct timespec *)(-1)) {
        perror("Error attaching shared memory");
        return 1;
    }

    // Access and use the system clock in shared memory
    printf("Worker Process - Seconds: %ld\n", system_clock->tv_sec);
    printf("Worker Process - Nanoseconds: %ld\n", system_clock->tv_nsec);

    // Detach the shared memory segment
    if (shmdt(system_clock) == -1) {
        perror("Error detaching shared memory");
        return 1;
    }

    return 0;

}