#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
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

union seg_t *procs;
int available = -1;
char* shm_ptr;
// Mutex variables
pthread_mutex_t* mutex;

void exit_handler(int sig) {
    // ADD

    // critical section begins
	pthread_mutex_lock(mutex);

    // Client leaving; needs to reset its segment   
    procs[available].clientStats.pid = -1;

	pthread_mutex_unlock(mutex);
	// critical section ends
    if(munmap(shm_ptr, PAGESIZE)==-1){
        exit(1);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    if(argc!=2||strlen(argv[1])>10){
        exit(1);
    }
    struct timeval start, end;
    struct sigaction act;
	// ADD
    memset (&act, 0, sizeof(act));
    act.sa_handler = exit_handler;
    if(sigaction(SIGINT, &act, NULL)==-1){
        exit(1);
    }
    if(sigaction(SIGTERM, &act, NULL)==-1){
        exit(1);
    }

	int fd_shm = shm_open(SHM_NAME, O_RDWR, 0660);
    if(fd_shm==-1){
        exit(1);
    }

    shm_ptr = (char*)mmap(NULL, PAGESIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if (*(int*)shm_ptr == -1) {
      shm_unlink(SHM_NAME);
      exit(1);
    }


    mutex = (pthread_mutex_t*)shm_ptr;
    procs = (union seg_t*)(shm_ptr+64);
    
    // critical section begins
    pthread_mutex_lock(mutex);

    for (int i = 0; i < 63; i++) {
      if (procs[i].clientStats.pid == -1) {
        available = i;
        break;
      }
    }
    
    if(available==-1){
        pthread_mutex_unlock(mutex);
        munmap(shm_ptr, PAGESIZE);
        exit(1);
    }
    gettimeofday(&start, NULL);
    time_t birth;
    time(&birth);
    strncpy(procs[available].clientStats.birth, ctime(&birth), strlen(ctime(&birth))-1);
    strncpy(procs[available].clientStats.clientString, argv[1], strlen(argv[1]));
    procs[available].clientStats.pid = getpid();
	// client updates available segment
	pthread_mutex_unlock(mutex);
    // critical section ends
        
	while (1) {
        
		// ADD
        gettimeofday(&end, NULL);
        procs[available].clientStats.elapsed_sec = end.tv_sec - start.tv_sec;
        procs[available].clientStats.elapsed_msec = (end.tv_usec - start.tv_usec)/1000.f;

        sleep(1);

		// Print active clients
        fprintf(stdout,"Active clients :");
        for (int i = 0; i < 63; i++) {
          if (procs[i].clientStats.pid != -1) {
              fprintf(stdout," %d", procs[i].clientStats.pid);
          }
        }
        fprintf(stdout,"\n");
    }
    

    return 0;
}
