/*
* Multi-lookup implementation for DNS resolution.
*
* Author: Carl Cortright
* Date: 10/15/2016
*/

// Include standard library files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/sysinfo.h>

// Include provided files
#include "multi-lookup.h"
#include "queue.h"
#include "util.h"

#define SBUFSIZE 1025
#define INPUTFS "%1024s"

/*
* Main function to request the IP addresses in parallel
*/
int main(int argc, char* argv[]){
  // Defensive programming, check to see if we have a good number of input files
  if (argc > 10){
    fprintf(stderr, "Error! Too many arguments. Please provide less than 10 input files");
    exit(1);
  }

  // Local vars
  queue q;
  pthread_cond_t q_cv;
  pthread_mutex_t q_lock;

  /* Create a thread for each of the input files */
  pthread_t requester_threads[argc];
  for(int i=1; i <= (argc-1); i++){
    // Open the input file
    FILE* inputfp = fopen(argv[i], "r");
  	if(!inputfp){
  	  fprintf(stderr, "Error Opening Input File: %s", argv[i]);
  	  break;
  	}

    // Create the struct to pass to add_request
    struct arg_struct args;
    args.file = inputfp;
    args.q = &q;
    args.cv = &q_cv;
    args.mutex = &q_lock;

    // Create the thread
    if(pthread_create(&(requester_threads[i]), NULL, add_request, (void *)&args)){
      fprintf(stderr, "Error creating threads!");
      exit(0);
    }
  }

  for(int t=1;t<=(argc-1);t++){
     pthread_join(requester_threads[t],NULL);
  }

  /*Create a thread for each processor core to process requests */

  return 0;
}

/*
* Adds each request from the file to the queue q
*/
void* add_request(void* in){
  // Deconstruct the arguments
  struct arg_struct *args = in;
  FILE* fp = args->file;
  queue* q = args->q;
  pthread_cond_t* cv = args->cv;
  pthread_mutex_t* mutex = args->mutex;

  // Read each line from the input file
  char* hostname[SBUFSIZE];
  char firstipstr[INET6_ADDRSTRLEN];
  while(fscanf(fp, INPUTFS, *hostname) > 0){
    // CRITICAL SECTION BEGIN: Push each domain to the queue
    pthread_mutex_lock(mutex);
    while(queue_is_full(&q)){
      pthread_cond_wait(cv, mutex);
    }
    queue_push(&q, hostname);
    pthread_mutex_unlock(mutex);
    // CRITICAL SECTION END: Let another process have the mutex
  }
  
  return NULL;
}

/*
* Processes each request coming out of the queue
*/
void* process_request(void* in){

  return NULL;
}
