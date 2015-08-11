#! /bin/sh
rmmod bbswitch_nv
rmmod bbswitch
echo 0000:00:01.1 > /sys/bus/pci/drivers/pcieport/unbind
