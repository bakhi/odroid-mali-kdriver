#!/bin/bash

make -j8

sudo rmmod mali_kbase
# cp mali_kbase.ko /lib/modules/$(uname -r)/kernel/drivers/gpu/arm/midgard/
# sudo depmod -a
# sudo modprobe mali_kbase
sudo insmod mali_kbase.ko
