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


int main(int argc, char *argv[]) {
    int shm_id;


    shm_id = shmget(sh_key, sizeof(int) * 2, 0644);
    if (shm_id == -1) {
        perror("Error getting shared memory");
        return 1;
    }

    int* system_clock = (int*)shmat(shm_id , NULL, 0);
    if (system_clock == (void *) -1) {
        perror("Error attaching shared memory");
        return 1;
    }

    // Arguments
    int runtimeS = (int) (system_clock[0] + atoi(argv[1]));
    int runtimeNs = (int) (system_clock[1] + atoi(argv[2]));

    // Print Starting Line
    printf("WORKER PID:%d PPID%d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n --Just Starting\n" , getpid() , getppid() , system_clock[0], system_clock[1], runtimeS, runtimeNs);


    int shutdown = 0;
    int currentTime = system_clock[0];
    int secondTracker = 0;
    while(shutdown) {

        if (runtimeS - system_clock[0] <= 0 && runtimeNs - system_clock[1] <= 0) {
            printf("WORKER PID:%d PPID%d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n --Terminating\n" , getpid() , getppid() , system_clock[0], system_clock[1], runtimeS, runtimeNs);
        } else if (system_clock[0] - currentTime == 1) {
            secondTracker++;
            currentTime = system_clock[0];
            printf("WORKER PID:%d PPID%d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d\n --%d second(s) have passed since starting\n" , getpid() , getppid() , system_clock[0], system_clock[1], runtimeS, runtimeNs, secondTracker);
        }
    }

    // Detach the shared memory segment
    if (shmdt(system_clock) == -1) {
        perror("Error detaching shared memory");
        return 1;
    }

    return 0;

}