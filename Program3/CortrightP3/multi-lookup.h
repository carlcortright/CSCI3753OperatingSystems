/*
* Header file for our multi-lookup dns resolver
*
* Author: Carl Cortright
* Date: 10/15/2016
*/

#ifndef MULTI
#define MULTI

#include <pthread.h>
#include "queue.h"


/*
* Struct to hold the arguments to pass to a resolver thread
*/
typedef struct arg_struct {
  FILE* file;
  queue* q;
  pthread_cond_t* cv;
  pthread_mutex_t* mutex;
  int* alive;
} args;

/*
* Adds each request from the file to the queue q
*/
void* add_request(void* in);

/*
* Processes each request coming out of the queue
*/
void* process_request(void* out);

#endif
