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

    // Arguments
    int runtimeS = (int) (system_clock->tv_sec + (argv[1]));
    int runtimeNs = (int) (system_clock->tv_nsec + (argv[2]));

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

    // Print Starting Line
    printf("WORKER PID:%d PPID%d SysClockS: %lld SysClockNano: %ld TermTimeS: %d TermTimeNano: %d\n --Just Starting\n" , getpid() , getppid() , system_clock->tv_sec, system_clock->tv_nsec, runtimeS, runtimeNs);


    int shutdown = 0;
    int currentTime = system_clock->tv_sec;
    int secondTracker = 0;
    while(shutdown) {

        if (runtimeS - system_clock->tv_sec <= 0 && runtimeNs - system_clock->tv_nsec <= 0) {
            printf("WORKER PID:%d PPID%d SysClockS: %lld SysClockNano: %ld TermTimeS: %d TermTimeNano: %d\n --Terminating\n" , getpid() , getppid() , system_clock->tv_sec, system_clock->tv_nsec, runtimeS, runtimeNs);
        } else if (system_clock->tv_sec - currentTime == 1) {
            secondTracker++;
            currentTime = system_clock->tv_sec;
            printf("WORKER PID:%d PPID%d SysClockS: %lld SysClockNano: %ld TermTimeS: %d TermTimeNano: %d\n --%d second(s) have passed since starting\n" , getpid() , getppid() , system_clock->tv_sec, system_clock->tv_nsec, runtimeS, runtimeNs, secondTracker);
        }
    }

    // Detach the shared memory segment
    if (shmdt(system_clock) == -1) {
        perror("Error detaching shared memory");
        return 1;
    }

    return 0;

}