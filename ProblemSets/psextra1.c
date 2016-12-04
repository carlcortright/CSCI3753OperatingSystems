/*
* Mutex implemented using the CAS instrution
*/
typedef int mutex;

/*
* Wait before entering the critical zone
*/
int wait(mutex* m){
  while(!CAS(m, 0, 1)){ /* Wait for the mutex to be free*/ }
}

/*
* Signal that another process can enter the critical zone
*/
int signal(mutex* m){
  // If the mutex is is use, set it to be not in use
  CAS(m, 1, 0);
}
