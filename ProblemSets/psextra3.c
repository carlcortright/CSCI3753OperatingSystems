/*
* Monitor based solution to the rowboat problem
*/
monitor rowboat{
  int hackersWaiting = 0;
  int employeesWaiting = 0;
  int onboard = 0;

  // There can be at most 3 hackers or 3 employees waiting
  condition hackers[3];
  condition employees[3];
  condition rowing;

  /*
  * Method called when an employee arrives
  */
  void EmployeeArrives(){
    if(employeesWaiting == 3){
      /*
      * Have all 4 employees board the boat
      * This will happen in order, as the threads will
      * re-enter the entry queue.
      * https://www.cs.mtu.edu/~shene/NSF-3/e-Book/MONITOR/CV.html
      */
      employees[0].signal();
      employees[1].signal();
      employees[2].signal();
      onboard = 3;
      BoardBoat();
      RowBoat();
      // Release all of the threads that were waiting for Rowboat()
      rowing.signal();
      // Signal other threads to resume execution and reset boat
      hackers[0].signal();
      hackers[1].signal();
      hackers[2].signal();
      onboard = 0;
    } else if(hackersWaiting == 2 && employeesWaiting == 1) {
      // Board 2 hackers and 2 employees
      hackers[0].signal();
      hackers[1].signal();
      employees[0].signal();
      onboard = 3;
      BoardBoat();
      // Release all threads that were waiting for RowBoat()
      RowBoat();
      rowing.signal();
      // Realease all other waiting threads and reset boat
      hackers[2].signal();
      employees[1].signal();
      employees[2].signal()
      onboard = 0;
    } else {
      employeesWaiting++;
      employees[employeesWaiting - 1].wait();
      if(onboard != 3){
        BoardBoat();
        // Wait until RowBoat() has returned
        rowing.wait();
      }
      employeesWaiting--;
    }
  }

  /*
  * Method called when a hacker arrives.
  * This is just a mirror of EmployeeArrives() except for hackers
  */
  void HackerArrives(){
    if(hackersWaiting == 3){
      hackers[0].signal();
      hackers[1].signal();
      hackers[2].signal();
      onboard = 3;
      BoardBoat();
      RowBoat();
      // Release all of the threads that were waiting for Rowboat()
      rowing.signal();
      // Signal other threads to resume execution and reset boat
      employees[0].signal();
      employees[1].signal();
      employees[2].signal();
      onboard = 0;
    } else if(hackersWaiting == 1 && employeesWaiting == 2) {
      // Board 2 hackers and 2 employees
      employees[0].signal();
      employees[1].signal();
      hackers[0].signal();
      onboard = 3;
      BoardBoat();
      // Release all threads that were waiting for RowBoat()
      RowBoat();
      rowing.signal();
      // Realease all other waiting threads and reset boat
      employees[2].signal();
      hackers[1].signal();
      hackers[2].signal()
      onboard = 0;
    } else {
      hackersWaiting++;
      hackers[employeesWaiting - 1].wait();
      if(onboard != 3){
        BoardBoat();
        // Wait until RowBoat() has returned
        rowing.wait();
      }
      hackersWaiting--;
    }
  }
}
