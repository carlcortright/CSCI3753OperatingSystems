
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdio.h>

#define SYS_helloworld 326
#define SYS_simple_add 327

int main(){
  int rc = syscall(SYS_helloworld);
  int res = 0;
  int* result = &res;
  syscall(SYS_simple_add, 1, 41, result);
  printf("Result from system call = %d \n", *result);
  return 0;
}
