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
#include <unistd.h>

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
  int running = 0;

  if(queue_init(&q, 128)==QUEUE_FAILURE) {
		fprintf(stderr,"Error creating the Queue\n");
		return EXIT_FAILURE;
	}

  /* Create a thread for each of the input files */
  pthread_t requester_threads[argc];
  for(int i=0; i < (argc-1); i++){
    // Open the input file
    if( access( argv[i], F_OK ) != -1 ) {
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
      args.alive = &running;

      // Create the thread
      if(pthread_create(&(requester_threads[i]), NULL, add_request, (void *)&args)){
        fprintf(stderr, "Error creating threads!");
        exit(0);
      }
    } else {
      fprintf(stderr, "ERROR: Could not find input file %s \n", argv[i]);
    }
  }

  /* Create a thread for each processor core to process requests */
  int procs = get_nprocs();
  pthread_t resolver_threads[procs];
  FILE* outfp;
  if( access( argv[argc-1], F_OK ) != -1 ) {
    // Create the output fp
    outfp = fopen(argv[argc-1], "w");
    if(!outfp){
      fprintf(stderr, "Error Opening output File: %s exiting... \n", argv[argc-1]);
      exit(1);
    }

    for(int i = 0; i < procs; i++){
      // Create the struct to pass to processor
      struct arg_struct args;
      args.file = outfp;
      args.q = &q;
      args.cv = &q_cv;
      args.mutex = &q_lock;
      args.alive = &running;

      if(pthread_create(&(resolver_threads[i]), NULL, process_request, (void *)&args)){
        fprintf(stderr, "Error creating threads!");
        exit(0);
      }
    }
  } else {
      fprintf(stderr, "ERROR: Could not find the file %s, exiting... \n", argv[argc-1]);
      exit(1);
  }


  // Wait for all of the processes to finish
  for(int t=1;t < (argc-1);t++){
     pthread_join(requester_threads[t],NULL);
  }
  for(int t=0; t < procs; t++){
    pthread_join(resolver_threads[t],NULL);
  }

  fclose(outfp);

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
  int* alive = args->alive;

  // Increment the number of request threads
  pthread_mutex_lock(mutex);
  *alive++;
  pthread_mutex_unlock(mutex);

  // Read each line from the input file
  char* hostname[SBUFSIZE];
  while(fscanf(fp, INPUTFS, hostname) > 0){
    // CRITICAL SECTION BEGIN: Push each domain to the queue
    pthread_mutex_lock(mutex);
    while(queue_is_full(&q)){
      pthread_cond_wait(cv, mutex);
    }
    char* payload;
    payload=malloc(SBUFSIZE);
    payload=strncpy(payload, hostname, SBUFSIZE);
    queue_push(q, payload);
    pthread_mutex_unlock(mutex);
    // CRITICAL SECTION END: Let another process have the mutex
  }

  // Decrement the number of request threads
  pthread_mutex_lock(mutex);
  *alive--;
  pthread_mutex_unlock(mutex);

  return NULL;
}

/*
* Processes each request coming out of the queue
*/
void* process_request(void* out){
  // Deconstruct the arguments
  struct arg_struct *args = out;
  FILE* outfp = args->file;
  queue* q = args->q;
  pthread_cond_t* cv = args->cv;
  pthread_mutex_t* mutex = args->mutex;
  int* alive = args->alive;

  // Resolve all of the dns records
  while(!queue_is_empty(q) || *alive != 0){
    // CRITICAL SECTION BEGIN: Lookup each domain
    pthread_mutex_lock(mutex);
    char* payload;
    if((payload = queue_pop(q)) == NULL){
  	  fprintf(stderr, "error: queue_pop failed!\n");
    } else {
      // Get the IP address
      char firstipstr[INET6_ADDRSTRLEN];
      dnslookup(payload, firstipstr, sizeof(firstipstr));

      // Check for a valid IP and write to file
      if(strcmp(firstipstr, "UNHANDELED") == 0){
        printf("%s,%s\n", payload, "");
        // Write to file
        fprintf(outfp, "%s,%s\n", payload, "");
        fprintf(stderr, "ERROR: Bogus IP! \n");
      } else {
        printf("%s,%s\n", payload, firstipstr);
        // Write to file
        fprintf(outfp, "%s,%s\n", payload, firstipstr);
      }
      free(payload);
    }

    pthread_cond_signal(cv);
    pthread_mutex_unlock(mutex);
    // CRITICAL SECTION END: Let another process have the mutex
  }

  return NULL;
}
