#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage long sys_simple_add(int number1, int number2, int* result){
  int final_result;
  printk("Number 1 = %d", number1);
  printk("Number 2 = %d", number2);
  final_result = number1 + number2;
  *result = final_result;
  printk("Result = %d", *result);
  return 0;
}
