#include "cs537.h"
#include "request.h"
#include <pthread.h>

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too
pthread_mutex_t mutex;
pthread_cond_t full;
pthread_cond_t empty;
int* buffer;
int buff_max;
int buff_num;
int buff_insert = 0;
int buff_take = 0;

void getargs(int *port, int *threads, int *buffers, int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
	exit(1);
    }
    
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffers = atoi(argv[3]);

    if (*threads < 1 || *buffers < 1) {
      fprintf(stderr, "threads and bufferes must be positive integers\n");
      exit(1);
    } 
}

void writeRequest(int connfd){
    pthread_mutex_lock(&mutex);
    while (buff_num == buff_max){ 
        pthread_cond_wait(&empty, &mutex);
    }
    buffer[buff_insert] = connfd;
    buff_num++;
    buff_insert = (buff_insert+1) % buff_max;
    pthread_cond_signal(&full);
    pthread_mutex_unlock(&mutex); 
}

void *handler(void *arg){
    while (1) { 
        pthread_mutex_lock(&mutex);
        while (buff_num == 0) 
            pthread_cond_wait(&full, &mutex);
        int fd = buffer[buff_take];
        buff_take = (buff_take+1) % buff_max;
        buff_num --;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex); 
        requestHandle(fd);
        Close(fd);
  } 


}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threads, buffers;
    struct sockaddr_in clientaddr;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&full, NULL);
    pthread_cond_init(&empty, NULL);


    getargs(&port, &threads, &buffers, argc, argv);
    buff_max = buffers;
    buffer = (int*)malloc(buffers*sizeof(int));

    // 
    // CS537: Create some threads...
    //
    pthread_t t;
    for(int i=0; i<threads;i++){
        pthread_create(&t, NULL, handler, NULL);
    }

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 

    writeRequest(connfd);
    }

}


    


 
