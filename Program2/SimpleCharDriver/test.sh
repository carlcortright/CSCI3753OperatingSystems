make -C /lib/modules/$(uname -r)/build M=$PWD modules
sudo rmmod simple_char_driver
sudo insmod simple_char_driver.ko
