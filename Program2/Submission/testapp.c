#include <stdio.h>
#include <fcntl.h>

void device_read();
void device_write();


/*
 * Test app for our simple_char_driver
 */
int main(){
  int in_test;
  char in[100];

  in_test = 1;
  while(in_test){
    printf("Press 'r' to read from device \n");
    printf("Press 'w' to write to device \n");
    printf("Press 'e' to exit \n");
    printf("Press anything to keep reading and writing to device \n");
    printf("Enter a command: ");
    scanf("%s", &in);
    char switcheroon = in[0];
    switch(switcheroon){
    case 'r':
      device_read();
      break;
    case 'w':
      device_write();
      break;
    case 'e':
      in_test = 0;
      break;
    }
  }
}

/*
 * Reads the data from the device file
 */
void device_read(){
  printf("Data read from device: \n");
  char buffer[1024];

  // Print what is in the file
  int fp = open("/dev/simple_char_device", O_RDWR);
  read(fp, buffer, 1024);
  close(fp);
  printf("%s \n", buffer);

}

/*
 * Writes some user generated data to the device 
 */ 
void device_write(){
  char input[1024];
  printf("Enter the data you want to write to the device: ");
  scanf("%s", input);

  int fp = open("/dev/simple_char_device", O_RDWR);
  write(fp, input, strlen(input));
  close(fp);
}
