#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>


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


// clock incrementation
int shm_id;
int* system_clock;
void incrementClock(int* clock , int nano) {
    clock[1] += nano;
    if (clock[1] >= 1000000000) {
        clock[0] ++;
        clock[1] -= 1000000000;
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
    printf("-i: How often a worker should be launched (in milliseconds)\n");
}


int main(int argc, char *argv[]) {
    srand(time(NULL));
    // Opt variable and Opt String
    char opt;
    const char optstr[] = "hn:s:t:i:";
    int status;

    // Variables to hold values from Optstr
    int arg_n = 0;
    int arg_s = 0;
    char *arg_t;
    int arg_i = 0;


    if (setupinterrupt() == -1) {
        perror("Failed to set up handler for SIGPROF");
        return 1;
    }
    if (setupitimer() == -1) {
        perror("Failed to set up ITIMER_PROF interval timer");
        return 1;
    }


    // TEST: INFINITE LOOP TO TEST CLOCK DELETE THIS LATER
    // TEST: SYSTEM CLOCK USAGE DELETE THIS LATER

    // OPTARGS
    while ((opt = getopt(argc, argv, optstr)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return(EXIT_SUCCESS);
            case 'n':
                arg_n = atoi(optarg);
                break;
            case 's':
                arg_s = atoi(optarg);
                break;
            case 't':
                arg_t = optarg;
                break;
            case 'i':
                arg_i = atoi(optarg);
                arg_i *= 1000000;
                break;
            case '?':
                print_usage(argv[0]);
                break;
            default:
                printf("Invalid option %c\n" , optopt);
                print_usage(argv[0]);
                return (EXIT_FAILURE);
        }
    }

    // Check if all argument were provided for use
    if (arg_n <= 0 || arg_s <= 0 || arg_t <= 0 || arg_i <= 0) {
        printf("All arguments are required\n");
        print_usage(argv[0]);

        return(EXIT_FAILURE);
    }

    // Keep the iterator low to prevent confusion and time lag on OpSyS server
    if (atoi(arg_t) > 10) {
        printf("Please keep your time limit for workers between 0 and 10 seconds to reduce time strain");

        return (EXIT_FAILURE);
    }
    // Keep the number of simultaneous processes low to reduce lag on OpSys server
    if (arg_s > 20) {
        printf("Please keep the simultaneous number of processes below 20");

        return(EXIT_FAILURE);
    }

    // initialize the system clock



    // Create or get the shared memory segment
    shm_id = shmget(sh_key, sizeof(int) * 2, IPC_CREAT|0666);
    printf("ShmID: %d\n" , shm_id);
    if (shm_id == -1) {
        perror("Error creating/getting shared memory");
        return 1;
    }

    // Attach the shared memory segment to the address space
    system_clock = (int*)shmat(shm_id, NULL, 0);
    if ((int)system_clock == -1) {
        perror("Error attaching shared memory");
        return 1;
    }

    system_clock[0] = 0;
    system_clock[1] = 1;

    // Pass to forked Worker and setup clock
    int workerLaunch = 0;
    int activeUsers = 0;

    while (workerLaunch < arg_n) {
        incrementClock(system_clock , 10);
        if (system_clock[1] % 500000000 == 0) {
            printf("Process Table Goes Here");
        }
        if (activeUsers < arg_s) {
            pid_t workPid = fork();
            if (workPid == 0) {
                // Randomize the outgoing seconds and nanoseconds
                int rand_tS = rand() % atoi(arg_t) + 1;
                int rand_tNs = rand() % 1000000000 + 1;

                char rand_tS_str[20];
                char rand_tNs_str[20];
                snprintf(rand_tS_str, sizeof(rand_tS_str), "%d" , rand_tS);
                snprintf(rand_tNs_str, sizeof(rand_tNs_str), "%d" , rand_tNs);

                printf("Forking now current clock: %d %d\n" , system_clock[0] , system_clock[1]);
                char* args[] = {"./worker" , rand_tS_str , rand_tNs_str , 0};
                execvp(args[0] , args);
                perror("Error in execvp launching");
                exit(EXIT_FAILURE);
            } else if(workPid == -1) {
                perror("Error in fork of worker process");
                exit(EXIT_FAILURE);
            } else {
                printf("Update PCB here\n");
                activeUsers++;
                workerLaunch++;
            }
        }


        //check for child termination
        int pid = waitpid(-1 , &status, WNOHANG);
        if (pid > 0) {
            activeUsers--;
        }
        usleep(100);
    }



    // Detach the shared memory segment
    if (shmdt(system_clock) == -1) {
        perror("Error detaching shared memory");
        return(EXIT_FAILURE);
    }

    // Remove the shared memory segment
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Error removing shared memory");
        return(EXIT_FAILURE);
    }

    return 0;
}