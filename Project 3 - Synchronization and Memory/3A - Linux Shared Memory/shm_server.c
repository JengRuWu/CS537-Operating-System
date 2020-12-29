#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#define SHM_NAME "/jeng-ru_feena"
#define PAGESIZE 4096
// ADD NECESSARY HEADERS
typedef struct {
    int pid;
    char birth[25];
    char clientString[10];
    int elapsed_sec;
    double elapsed_msec;
} stats_t;

union seg_t{
    stats_t clientStats;
    char padding[64];
};

char* shm_ptr;
// Mutex variables
pthread_mutex_t* mutex;
pthread_mutexattr_t mutexAttribute;

void exit_handler(int sig) 
{
    // ADD
    if(munmap(shm_ptr, PAGESIZE)==-1){
        exit(1);
    }
    if(shm_unlink(SHM_NAME)==-1){
        exit(1);
    }
	exit(0);
}

int main(int argc, char *argv[]) 
{
    // ADD
	struct sigaction act;
    memset (&act, 0, sizeof(act));
    act.sa_handler = exit_handler;
    if(sigaction(SIGINT, &act, NULL)==-1){
        exit(1);
    }
    if(sigaction(SIGTERM, &act, NULL)==-1){
        exit(1);
    }

	// Creating a new shared memory segment
	int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);
    if(fd_shm==-1){
        exit(1);
    }
	int trun = ftruncate(fd_shm, PAGESIZE);
    if(trun==-1){
        shm_unlink(SHM_NAME);
        exit(1);
    }
	shm_ptr = (char*)mmap(NULL, PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (*(int*)shm_ptr == -1) {
      shm_unlink(SHM_NAME);
      exit(1);
    }


    mutex = (pthread_mutex_t*)shm_ptr;	
    
    // Initializing mutex
	pthread_mutexattr_init(&mutexAttribute);
	pthread_mutexattr_setpshared(&mutexAttribute, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(mutex, &mutexAttribute);


    union seg_t *procs = (union seg_t*)(shm_ptr+64);

    for (int i=0; i < 63; i++) {
      procs[i].clientStats.pid = -1;
    }

    int iteration = 0;
    while (1) 
	{
		// ADD
        iteration++;
        for (int i = 0; i < 63; i++) {
            if (procs[i].clientStats.pid != -1) {
            fprintf(stdout,"%d, pid : %d, birth : %s, elapsed : %d s %5.4f ms, %s\n",
                iteration, procs[i].clientStats.pid , procs[i].clientStats.birth, procs[i].clientStats.elapsed_sec, procs[i].clientStats.elapsed_msec, procs[i].clientStats.clientString);
            }
        }
        sleep(1);
    }

    return 0;
}
