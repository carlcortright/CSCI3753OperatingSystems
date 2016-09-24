#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>
#define BUFFER_SIZE 1024

/* Add module author and licence */
MODULE_AUTHOR("Carl Cortright");
MODULE_LICENSE("GPL");

static char device_buffer[BUFFER_SIZE];
int num_opens = 0;
int num_closes = 0;
int dev_file_bytes = 0;

ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
  // Print to the log that we are reading from the character driver
  printk(KERN_ALERT "Reading from character driver...");

  // Check to make sure we aren't reading something that is outside of our buffer
  if(*offset >= sizeof(device_buffer)){
    printk(KERN_ALERT "Offset out of buffer range. Stopping read.");
    return 1;
  }
  if(*offset + length > sizeof(device_buffer)){
    length = sizeof(device_buffer) - *offset;
  }

  printk("");
  printk(device_buffer);
  // Copy length bytes from kernel space to user space
  int err_code;
  err_code = copy_to_user(buffer, device_buffer + *offset, length);

  printk("User buffer: ");
  printk("");
  printk(buffer);
  // Print the number of bytes copied
  if (err_code == 0){
    printk(KERN_ALERT "Read %d bytes", length);
  } else {
    printk(KERN_ALERT "Error encountered during simple_char_driver_read during copy_to_user!!");
    return 1;
  }
  
  return 0;
}


ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
  // Print to the log that we are writing from the character driver
  printk(KERN_ALERT "Writing from character driver...");

  // Check to make sure we aren't writing something that is outside of our buffer
  if(*offset >= sizeof(device_buffer)){
    printk(KERN_ALERT "Offset out of buffer range. Aborting write!");
    return 1;
  }
  if(*offset + length > sizeof(device_buffer)){
    printk(KERN_ALERT "Trying to write past the size of the buffer. Aborting write!");
    return 1;
  }

  // Make sure the offset is the number of bytes in the device file
  int position;
  if(*offset != dev_file_bytes){
    *offset = dev_file_bytes;
  }

  // Copy length bytes from user space to kernel space
  int err_code;
  err_code = copy_from_user(device_buffer + *offset, buffer, length);
  
  // Print the number of bytes copied
  if (err_code == 0){
    printk(KERN_ALERT "Wrote %d bytes", length);
  } else {
    printk(KERN_ALERT "Error encountered during simple_char_driver_write during copy_from_user!!");
    return 1;
  }  

  dev_file_bytes += length;
  
  return length;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
  /* print to the log file that the device is opened and also print the number of times this device has been opened until now*/
  printk(KERN_ALERT "Device opening!");

  num_opens++;
  printk(KERN_ALERT "This device has been opened: %d times", num_opens);
  return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
  /* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
  printk(KERN_ALERT "Device closing!");

  num_closes++;
  printk(KERN_ALERT "This device has been closed: %d times", num_closes);
  return 0;
}

struct file_operations simple_char_driver_file_operations = {
  .owner   = THIS_MODULE,
  /* add the function pointers to point to the corresponding file operations. look at the file fs.h in the linux souce code*/	
  .read = simple_char_driver_read,
  .write = simple_char_driver_write,
  .open = simple_char_driver_open,
  .release = simple_char_driver_close
};

static int simple_char_driver_init(void)
{
  /* print to the log file that the init function is called.*/
  printk(KERN_ALERT "Registering simple_char_driver");
  
  /* register the device */
  register_chrdev(240, "simple_char_driver", &simple_char_driver_file_operations);
  
  return 0;
}

static int simple_char_driver_exit(void)
{
  /* print to the log file that the exit function is called.*/
  printk(KERN_ALERT "Unregistering simple_char_driver");
  
  /* unregister  the device using the register_chrdev() function. */
  unregister_chrdev(240, "simple_char_driver");
  
  return 0;
}

/* add module_init and module_exit to point to the corresponding init and exit function*/
module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
