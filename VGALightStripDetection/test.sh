#!/bin/bash
start_time=$(date +%s)
work_path=$(cd `dirname $0`; pwd)
#dirname $0，取得当前执行的脚本文件的父目录
#cd `dirname $0`，进入这个目录(切换当前工作目录)
#pwd，显示当前工作目录(cd执行后的)

library_path="$work_path/lib"

# add share lib path to system
export PATH="${work_path}:$PATH"
export LD_LIBRARY_PATH="${library_path}:$LD_LIBRARY_PATH"

./vga-light-strip-detection "210381723300448" "TUF-RTX3070TI-8G-GAMING-2I3S"

# NVIDIA
# ./vga-light-strip-detection "210992023501743" "ROG-STRIX-RTX3070TI-O8G-GAMING-2I3S"

# AMD
# sudo ./vga-light-strip-detection "210992023501743" "ROG-STRIX-RX6700RT-O12G-GAMING"

end_time=$(date +%s)
cost_time=$[ $end_time-$start_time ]
echo "build kernel time is $(($cost_time/60))min $(($cost_time%60))s"