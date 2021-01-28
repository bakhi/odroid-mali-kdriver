#!/bin/bash

sudo cat /sys/kernel/debug/mali0/regs_history > ./trace/$1_regs_history

# no timeline in this r7p0
# sudo cat /sys/kernel/debug/mali0/timeline_history > ./trace/$1_timeline_history

sudo cat /sys/kernel/debug/mali0/rbuf_history > ./trace/$1_rbuf_history

#scp ./trace/* bakhi:~/projects/tgx/Bifrost-kdriver/trace
scp ./trace/* mac:/Users/bakhi/Projects/tgx/Bifrost-kdriver/trace
