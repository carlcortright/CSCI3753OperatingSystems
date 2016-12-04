/*
* Solution to the x1...5 synchronization problem
*/
include <stdio>

// Global semaphores and counters
semaphore x1, x2, x3, x3, x4, x5;

int main(){
  // Create 5 processes to play with
  int x[5];
  int pid;

  for(int i = 0; i < 5; i++){
    pid = fork();
    if(pid != 0){
      x[i] = pid;
      break;
    }
  }

  // Create artificial sync point for each process
  switch(pid){
    case x[0]:
      // Sync point for process 1
      // Let x4, x2, and x5 go
      signal(x1);
      signal(x1);
      signal(x1);
      break;
    case x[1]:
      wait(x1);
      // Sync point for process 2
      break;
    case x[2]:
      wait(x4);
      // Sync point for process 3
      signal(x3);
      signal(x3);
      break;
    case x[3]:
      wait(x1);
      wait(x3);
      // Sync point for process 4
      signal(x4);
      break;
    case x[4]:
      wait(x1);
      wait(x3);
      wait(x4);
      // Sync point for process 5
      break;
    default:
      // Parent process, wait for all other processes to finish
      wait(NULL);
      printf("Parent exiting...");
  }
}
