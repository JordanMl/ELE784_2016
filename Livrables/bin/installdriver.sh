sudo insmod ./charDriver.ko
sudo chmod 666 /dev/charDriverDev_node
sudo chmod 666 ./application
lsmod | grep charDriver
