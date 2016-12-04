/*
 * File: pi-sched.c
 * Author: Andy Sayler
 * Revised: Dhivakant Mishra
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: 2012/03/07
 * Modify Date: 2012/03/09
 * Modify Date: 2016/31/10
 * Description:
 * 	This file contains a simple program for statistically
 *      calculating pi using a specific scheduling policy.
 *
 * Usage: ./pi-sched <children> <iterations> <policy> <vary priority>
 */

/* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <sched.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>

#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)
#define DEBUG 0

static inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

static inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

double calcPI(int iterations);

int main(int argc, char* argv[]){

  long iterations;
  struct sched_param param;
  int policy;
  int children;
  int vary = 0;

  // Get the number of times to fork the Process
  if (argc < 1){
    printf("ERROR: You must specify the number of child processes to spawn\n");
    exit(1);
  } else {
    children = atol(argv[1]);
  }

  /* Process program arguments to select iterations and policy */
  /* Set default iterations if not supplied */
  if(argc < 3){
    iterations = DEFAULT_ITERATIONS;
  }
  /* Set default policy if not supplied */
  if(argc < 4){
     policy = SCHED_OTHER;
  }
  /* Set iterations if supplied */
  if(argc > 2){
  	iterations = atol(argv[2]);
  	if(iterations < 1){
  	    fprintf(stderr, "Bad iterations value\n");
  	    exit(EXIT_FAILURE);
  	}
  }
  /* Set policy if supplied */
  if(argc > 3){
  	if(!strcmp(argv[3], "SCHED_OTHER")){
  	    policy = SCHED_OTHER;
  	}
  	else if(!strcmp(argv[3], "SCHED_FIFO")){
  	    policy = SCHED_FIFO;
  	}
  	else if(!strcmp(argv[3], "SCHED_RR")){
  	    policy = SCHED_RR;
  	}
  	else{
  	    fprintf(stderr, "Unhandeled scheduling policy\n");
  	    exit(EXIT_FAILURE);
  	}
  }

  /* Vary priorities if supplied */
  if(argc > 4){
    if(!strcmp(argv[4], "true")){
      vary = 1;
    }
  }

  /* Set process to max prioty for given scheduler */
  param.sched_priority = sched_get_priority_max(policy);

  /* Set new scheduler policy */
  if(DEBUG == 1){
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
  }
  if(sched_setscheduler(0, policy, &param)){
  	perror("Error setting scheduler policy");
  	exit(EXIT_FAILURE);
  }
  if (DEBUG == 1){
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));
  }

  // Time the mixed operations
  struct rusage usage;
  struct timeval startUser, endUser;
  struct timeval startSys, endSys;
  getrusage(RUSAGE_CHILDREN, &usage);
  startUser = usage.ru_utime;
  startSys = usage.ru_stime;
  struct timespec begin, end;
  clock_gettime(CLOCK_MONOTONIC, &begin);

  /* Fork the process N times */
  if(DEBUG == 1){
      printf("Forking %d children...\n", children);
  }
  int pid;
  double piCalc;
  for(int i = 0; i < children; i++){
    pid = fork();
    if(pid == 0){
      // We are a child, so calcPI
      piCalc = calcPI(iterations);

      /* Print result */
      if(DEBUG){
        fprintf(stdout, "pi = %f \n", piCalc);
      }
      exit(0);
    } else {
      // Vary the priorities if instructed
      if(vary){
        int max = sched_get_priority_max(policy);
        if (policy == SCHED_OTHER){
          // Change nice values instead
          nice(i);
        } else {
          // vary the priority
          int priority_val = i % max;
          setpriority(PRIO_PROCESS, pid, priority_val);
        }
      }
    }
  }

  // If at the end we are the parent, wait for all the children to be reaped
  wait(children);
  clock_gettime(CLOCK_MONOTONIC, &end);
  double end_time = ((double)(end.tv_sec)) + ((double)(end.tv_nsec / 10000000) / 100); // sec + decimal
  double begin_time = ((double)(begin.tv_sec)) + ((double)(begin.tv_nsec / 10000000) / 100); // sec + decimal
  double time_spent = end_time - begin_time;
  getrusage(RUSAGE_CHILDREN, &usage);
  endUser = usage.ru_utime;
  endSys = usage.ru_stime;
  if (DEBUG == 1){
    printf("User Start Time: %ld.%ld\n", startUser.tv_sec, startUser.tv_usec);
    printf("User End Time: %ld.%ld\n", endUser.tv_sec, endUser.tv_usec);
    printf("Sys Start Time: %ld.%ld\n", startSys.tv_sec, startSys.tv_usec);
    printf("Sys End Time: %ld.%ld\n", endSys.tv_sec, endSys.tv_usec);
  } else {
    // Print out a vector in standard csv format with relevant info
    printf("pi-sched,");
    printf("%s,", argv[3]);
    printf("%d,", children);
    if(vary == 1){ printf("true,"); }
    else { printf("false,"); }
    printf("%d,", iterations);
    printf("%ld.%ld,", startUser.tv_sec, startUser.tv_usec);
    printf("%ld.%ld,", endUser.tv_sec, endUser.tv_usec);
    printf("%ld.%ld,", startSys.tv_sec, startSys.tv_usec);
    printf("%ld.%ld,", endSys.tv_sec, endSys.tv_usec);
    printf("%lf\n", time_spent);
  }

  if (DEBUG == 1){
    printf("Parent terminating \n");
  }
  return 0;
}

/* Calculate pi using statistical methode across all iterations*/
double calcPI(int iterations){
  double x, y;
  double inCircle = 0.0;
  double inSquare = 0.0;
  double pCircle = 0.0;
  double piCalc = 0.0;

  for(int i=0; i<iterations; i++){
  	x = (random() % (RADIUS * 2)) - RADIUS;
  	y = (random() % (RADIUS * 2)) - RADIUS;
  	if(zeroDist(x,y) < RADIUS){
  	    inCircle++;
  	}
  	inSquare++;
  }

  /* Finish calculation */
  pCircle = inCircle/inSquare;
  piCalc = pCircle * 4.0;

  return piCalc;
}
