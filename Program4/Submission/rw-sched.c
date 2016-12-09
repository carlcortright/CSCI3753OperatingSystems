/*
 * File: rw-sched.c
 * Author: Carl Cortright
 * Base Code: Andy Sayer, Dhivakant Mishra
 *
 * Testing different schedulers on read/write ops
 *
 * Usage: ./rw-sched <number of processes> <policy> <vary priority> <transfer size> <block size>
 */

/* Include Flags */
#define _GNU_SOURCE

/* System Includes */
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

/* Defines */
#define MAXFILENAMELENGTH 80
#define DEFAULT_INPUTFILENAME "rwinput"
#define DEFAULT_OUTPUTFILENAMEBASE "rwoutput"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_TRANSFERSIZE 1024*100
#define DEBUG 0

/* Function Decarations */
int rw();

int main(int argc, char* argv[]){

  // Parameters for the scheduler
  struct sched_param param;
  int policy;
  int children;
  int vary = 0;
  // Parameters for the io operation
  int transfersize;
  int blocksize;

  // Get the number of times to fork the Process
  if (argc < 1){
    printf("ERROR: You must specify the number of child processes to spawn\n");
    exit(1);
  } else {
    children = atol(argv[1]);
  }

  /* Set policy if supplied */
  if(argc > 2){
    if(!strcmp(argv[2], "SCHED_OTHER")){
        policy = SCHED_OTHER;
    }
    else if(!strcmp(argv[2], "SCHED_FIFO")){
        policy = SCHED_FIFO;
    }
    else if(!strcmp(argv[2], "SCHED_RR")){
        policy = SCHED_RR;
    }
    else{
        fprintf(stderr, "Unhandeled scheduling policy\n");
        exit(EXIT_FAILURE);
    }
  }

  /* Set default policy if not supplied */
  if(argc < 3){
     policy = SCHED_OTHER;
  }

  /* Vary priorities if supplied */
  if(argc > 3){
    if(!strcmp(argv[3], "true")){
      vary = 1;
    }
  }

  /* Set supplied transfer size or default if not supplied */
  if(argc < 4){
     transfersize = DEFAULT_TRANSFERSIZE;
  }
  else{
    transfersize = atol(argv[4]);
    if(transfersize < 1){
        fprintf(stderr, "Bad transfersize value\n");
        exit(EXIT_FAILURE);
    }
  }
  /* Set supplied block size or default if not supplied */
  if(argc < 5){
     blocksize = DEFAULT_BLOCKSIZE;
  }
  else{
    blocksize = atol(argv[5]);
    if(blocksize < 1){
        fprintf(stderr, "Bad blocksize value\n");
        exit(EXIT_FAILURE);
    }
  }

  /* Set process to max prioty for given scheduler */
  param.sched_priority = sched_get_priority_max(policy);

  /* Set new scheduler policy */
  if (DEBUG == 1){
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
  }
  if(sched_setscheduler(0, policy, &param)){
  	perror("Error setting scheduler policy");
  	exit(EXIT_FAILURE);
  }
  // Print relevant debug info
  if (DEBUG == 1){
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));
    printf("Forking %d children...\n", children);
  }

  /*
  * Copy some data from /dev/random to a unique file for each process. This will
  * be slow, but it is necessary for the experiment.
  */
  char inFileNames[children][MAXFILENAMELENGTH];
  for(int i = 0; i < children; i++){
    snprintf(inFileNames[i], MAXFILENAMELENGTH, "%s%d", "in", i);
    rw(transfersize, blocksize, "/dev/urandom", inFileNames[i]);
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
  int pid;
  int pids[children];
  for(int i = 0; i < children; i++){
    pid = fork();
    if (pid == 0) {
      // vary nice values if SCHED_OTHER
      if (policy == SCHED_OTHER && vary) {
        nice(i);
      }

      // We are a child, so call rw
      rw(transfersize, blocksize, inFileNames[i], "out");

      exit(0);
    } else {
      // Vary the priorities if instructed
      if(vary && policy != SCHED_OTHER){
        int max = sched_get_priority_max(policy);
        int priority_val = i % max;
        setpriority(PRIO_PROCESS, pid, priority_val);
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
    printf("rw-sched,");
    printf("%s,", argv[2]);
    printf("%d,", children);
    if(vary == 1){ printf("true,"); }
    else { printf("false,"); }
    printf("%d,", transfersize);
    printf("%d,", blocksize);
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

/*
* Reads and writes some data from a file
*/
int rw(int transfersizeArg, int blocksizeArg, char* inputFilenameArg, char* outputFilenameArg){

  int rv;
  int inputFD;
  int outputFD;
  char inputFilename[MAXFILENAMELENGTH];
  char outputFilename[MAXFILENAMELENGTH];
  char outputFilenameBase[MAXFILENAMELENGTH];

  ssize_t transfersize = transfersizeArg;
  ssize_t blocksize = blocksizeArg;
  char* transferBuffer = NULL;
  ssize_t buffersize;

  ssize_t bytesRead = 0;
  ssize_t totalBytesRead = 0;
  int totalReads = 0;
  ssize_t bytesWritten = 0;
  ssize_t totalBytesWritten = 0;
  int totalWrites = 0;
  int inputFileResets = 0;

  // Copy the input and output file names
  strncpy(inputFilename, inputFilenameArg, MAXFILENAMELENGTH);
  strncpy(outputFilename, outputFilenameArg, MAXFILENAMELENGTH);
  strncpy(outputFilenameBase, outputFilenameArg, MAXFILENAMELENGTH);

  /* Confirm blocksize is multiple of and less than transfersize*/
  if(blocksize > transfersize){
  	fprintf(stderr, "blocksize can not exceed transfersize\n");
  	exit(EXIT_FAILURE);
  }
  if(transfersize % blocksize){
  	fprintf(stderr, "blocksize must be multiple of transfersize\n");
  	exit(EXIT_FAILURE);
  }

  /* Allocate buffer space */
  buffersize = blocksize;
  if(!(transferBuffer = malloc(buffersize*sizeof(*transferBuffer)))){
  	perror("Failed to allocate transfer buffer");
  	exit(EXIT_FAILURE);
  }

  /* Open Input File Descriptor in Read Only mode */
  if((inputFD = open(inputFilename, O_RDONLY | O_SYNC)) < 0){
    printf(inputFilename);
  	perror("Failed to open input file");
  	exit(EXIT_FAILURE);
  }

  /* Open Output File Descriptor in Write Only mode with standard permissions*/
  rv = snprintf(outputFilename, MAXFILENAMELENGTH, "%s-%d",
    outputFilenameBase, getpid());
  if(rv > MAXFILENAMELENGTH){
  	fprintf(stderr, "Output filename length exceeds limit of %d characters.\n",
  		MAXFILENAMELENGTH);
  	exit(EXIT_FAILURE);
  }
  else if(rv < 0){
  	perror("Failed to generate output filename");
  	exit(EXIT_FAILURE);
  }
  if((outputFD =
  	open(outputFilename,
  	     O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
  	     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0){
  	perror("Failed to open output file");
  	exit(EXIT_FAILURE);
  }

  /* Print Status */
  if(DEBUG == 1){
    fprintf(stdout, "Reading from %s and writing to %s\n",
      inputFilename, outputFilename);
  }

  /* Read from input file and write to output file*/
  do{
  	/* Read transfersize bytes from input file*/
  	bytesRead = read(inputFD, transferBuffer, buffersize);
  	if(bytesRead < 0){
      perror("Error reading input file");
      exit(EXIT_FAILURE);
  	}
  	else{
      totalBytesRead += bytesRead;
      totalReads++;
  	}

  	/* If all bytes were read, write to output file*/
  	if(bytesRead == blocksize){
      bytesWritten = write(outputFD, transferBuffer, bytesRead);
      if(bytesWritten < 0){
    		perror("Error writing output file");
    		exit(EXIT_FAILURE);
      }
      else{
    		totalBytesWritten += bytesWritten;
    		totalWrites++;
      }
  	}
  	/* Otherwise assume we have reached the end of the input file and reset */
  	else{
      if(lseek(inputFD, 0, SEEK_SET)){
    		perror("Error resetting to beginning of file");
    		exit(EXIT_FAILURE);
      }
      inputFileResets++;
  	}

  }while(totalBytesWritten < transfersize);

  /* Output some possibly helpfull info to make it seem like we were doing stuff */
  if (DEBUG == 1){
    fprintf(stdout, "Read:    %zd bytes in %d reads\n",
      totalBytesRead, totalReads);
    fprintf(stdout, "Written: %zd bytes in %d writes\n",
      totalBytesWritten, totalWrites);
    fprintf(stdout, "Read input file in %d pass%s\n",
      (inputFileResets + 1), (inputFileResets ? "es" : ""));
    fprintf(stdout, "Processed %zd bytes in blocks of %zd bytes\n",
      transfersize, blocksize);
  }

  /* Free Buffer */
  free(transferBuffer);

  /* Close Output File Descriptor */
  if(close(outputFD)){
  	perror("Failed to close output file");
  	exit(EXIT_FAILURE);
  }

  /* Close Input File Descriptor */
  if(close(inputFD)){
  	perror("Failed to close input file");
  	exit(EXIT_FAILURE);
  }

  strncpy(outputFilenameArg, outputFilename, MAXFILENAMELENGTH);

  return EXIT_SUCCESS;
}
