#!/bin/bash
#vim:set sw=4 ts=4 et:
help()
{
    echo "0 : set_off "
    echo "1 : set_red "
    echo "2 : set_green "
    echo "3 : set_blue "
    echo "4 : read_ledNum"
    echo "5 : set_all_led"
    echo "6 : set_single_led"
    exit 0
}

work_path=$(dirname $(readlink -f $0))
i2ctransfer="$work_path/i2c-tools/i2ctransfer";
i2cset="$work_path/i2c-tools/i2cset";

read_ledNum()
{
    `$i2ctransfer -f -y 0 w3@0x67 0x00 0x1D 0x0C 1>/dev/null`
    led_count=`$i2ctransfer -f -y 0 w1@0x67 0x08 r1`;
    echo $led_count > "$work_path/led_count.txt";
    # return $led_count
}

# delete_led_count_file()
# {
#     rm -f ./led_count.txt 1>/dev/null;
# }

set_setting()
{
    $i2cset -f -y 0 0x67 0x09 0x80
    $i2cset -f -y 0 0x67 0x0A 0x21
    $i2cset -f -y 0 0x67 0x01 0x01
    $i2cset -f -y 0 0x67 0x09 0x81
    $i2cset -f -y 0 0x67 0x0A 0x60
}

set_application()
{
    $i2cset -f -y 0 0x67 0x09 0x80
    $i2cset -f -y 0 0x67 0x0A 0x2F
    $i2cset -f -y 0 0x67 0x01 0x01
}

set_red()
{
    for i in {1..12}
    do
        $i2ctransfer -f -y 0 w8@0x67 0x03  0x06 0xff 0x00 0x00 0xff 0x00 0x00
    done
}
set_green()
{
    for i in {1..12}
    do
        $i2ctransfer -f -y 0 w8@0x67 0x03  0x06 0x00 0xff 0x00 0x00 0xff 0x00
    done
}
set_blue()
{
    for i in {1..12}
    do
        $i2ctransfer -f -y 0 w8@0x67 0x03  0x06 0x00 0x00 0xff 0x00 0x00 0xff
    done
}
set_off()
{
    for i in {1..12}
    do
        $i2ctransfer -f -y 0 w8@0x67 0x03  0x06 0x00 0x00 0x00 0x00 0x00 0x00
    done
}

target_led_red=$2
target_led_green=$3
target_led_blue=$4
target_led=$5

set_all_led()
{
    # get led count
    $i2ctransfer -f -y 0 w3@0x67 0x00 0x1D 0x0C 1>/dev/null
    led_count=`$i2ctransfer -f -y 0 w1@0x67 0x08 r1`;

    $i2cset -f -y 0 0x67 0x09 0x80	#Set Address High byte
    $i2cset -f -y 0 0x67 0x0A 0x21	#Set Address Low byte
    $i2cset -f -y 0 0x67 0x01 0x01	#Set Static effect

    for((i=0;i<$led_count;i++));
    do
        let led_addr=$i*3+0x60
        $i2cset -f -y 0 0x67 0x09 0x81
        $i2cset -f -y 0 0x67 0x0A $led_addr
        $i2ctransfer -f -y 0 w5@0x67 0x03 0x03 $target_led_red $target_led_blue $target_led_green	# 2021.10.20 Blue and green are opposite
    done
    
    $i2cset -f -y 0 0x67 0x09 0x80	#Set Address High byte
    $i2cset -f -y 0 0x67 0x0A 0x2F	#Set Address Low byte
    $i2cset -f -y 0 0x67 0x01 0x01	#Setting Apply
}

set_single_led()
{
    let led_addr=$target_led*3+0x60
    echo $led_addr
    
    $i2cset -f -y 0 0x67 0x09 0x80	#Set Address High byte
    $i2cset -f -y 0 0x67 0x0A 0x21	#Set Address Low byte
    $i2cset -f -y 0 0x67 0x01 0x01	#Set Static effect
    
    $i2cset -f -y 0 0x67 0x09 0x81
    $i2cset -f -y 0 0x67 0x0A $led_addr
    $i2ctransfer -f -y 0 w5@0x67 0x03 0x03 $target_led_red $target_led_blue $target_led_green
    
    $i2cset -f -y 0 0x67 0x09 0x80	#Set Address High byte
    $i2cset -f -y 0 0x67 0x0A 0x2F	#Set Address Low byte
    $i2cset -f -y 0 0x67 0x01 0x01	#Setting Apply
}

while [ -n "$1" ];do
    case $1 in
        0) set_setting;set_off;set_application;exit 0;;
        1) set_setting;set_red;set_application;exit 0;;
        2) set_setting;set_green;set_application;exit 0;;
        3) set_setting;set_blue;set_application;exit 0;;
        4) read_ledNum;exit 0;;
        5) set_all_led;exit 0;;
		6) set_single_led;exit 0;;
        # 7) delete_led_count_file;exit 0;;
        *) help;
    esac
done