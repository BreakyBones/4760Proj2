#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Set up the PCB according to what was given in the instructions
struct PCB {
    int occupied;
    pid_t pid;
    int startSeconds;
    int startNano;
};

struct PCB processTable[20];

// Shared memory Key just an arbitrary number I picked
const int sh_key = 205569;
const int sh_size = sizeof(struct timespec);

// clock incrementation
// discovered timespec online through a question asking about simulating a clock
void incrementClock(struct timespec *current_time, long nanoseconds) {
    current_time->tv_nsec += nanoseconds;

    if (current_time->tv_nsec >= 1000000000) {
        current_time->tv_sec += (current_time->tv_nsec / 1000000000);
        current_time->tv_nsec %= 1000000000;
    }
}

// Set up the failsafe shutdown
static void myHandler(int s) {
    printf("Got signal %d, terminate!\n" , s);
    exit(1);
}

// Set up myHandler
static int setupinterrupt (void) {
    struct sigaction act;
    act.sa_handler = myHandler;
    act.sa_flags = 0;

    return(sigemptyset(&act.sa_mask) || sigaction(SIGINT , &act, NULL) || sigaction(SIGPROF , &act , NULL));
}

static int setupitimer(void) { /* set ITIMER_PROF for 60-second intervals */
    struct itimerval value;
    value.it_interval.tv_sec = 60;
    value.it_interval.tv_usec = 0;
    value.it_value = value.it_interval;
    return (setitimer(ITIMER_PROF, &value, NULL));
}


void print_usage(const char *progName) {
    printf("Usage for %s: -n <n_value> -s <s_value> -t <t_value> -i <i_value>\n" , progName);
    printf("Options:\n");
    printf("-n: stands for the total number of workers to launch\n");
    printf("-s: Defines how many workers are allowed to run simultaneously\n");
    printf("-t: The time limit to pass to the workers\n");
    printf("-i: How often a worker should be launched");
}

int main(int argc, char *argv[]) {


    if (setupinterrupt() == -1) {
        perror("Failed to set up handler for SIGPROF");
        return 1;
    }
    if (setupitimer() == -1) {
        perror("Failed to set up ITIMER_PROF interval timer");
        return 1;
    }


    // TEST: INFINITE LOOP TO TEST CLOCK DELETE THIS LATER

    // initialize the system clock
    int shm_id;
    struct timespec *system_clock;

    // Create or get the shared memory segment
    shm_id = shmget(sh_key, sh_size, IPC_CREAT|0666);
    if (shm_id == -1) {
        perror("Error creating/getting shared memory");
        return 1;
    }

    // Attach the shared memory segment to the address space
    system_clock = (struct timespec *)shmat(shm_id, NULL, 0);
    if (system_clock == (struct timespec *)(-1)) {
        perror("Error attaching shared memory");
        return 1;
    }

    // TESTING SYSTEM CLOCK USAGE
    int run = 1;



    while (run == 1) {
        incrementClock(system_clock , 5);
        if (system_clock->tv_sec == 5) {
            run = 0;
        }
    }

    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("Error Forking");
        return 1;
    }

    if (child_pid == 0) {
        execlp("./worker", "worker", NULL);
        perror("Error executing worker.c");
        return 1;
    }

    // Detach the shared memory segment
    if (shmdt(system_clock) == -1) {
        perror("Error detaching shared memory");
        return 1;
    }

    // Remove the shared memory segment
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Error removing shared memory");
        return 1;
    }

    return 0;
}