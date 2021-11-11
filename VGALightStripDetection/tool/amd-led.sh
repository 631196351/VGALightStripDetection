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
agt="$work_path/agt_mcu";
# echo $agt;

read_ledNum()
{
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x1D 1>/dev/null`;	#Set Address High byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,0x0C 1>/dev/null`;	#Set Address Low byte
    tmp=`$agt -i=0 -oemi2cr:0,0xCE,0x08`;
    echo ${tmp##*:} > "$work_path/led_count.txt";
}

# delete_led_count_file()
# {
#     rm -f ./led_count.txt 1>/dev/null;
# }

target_led_red=$(printf '0x%X' "$2")
target_led_blue=$(printf '0x%X' "$3")
target_led_green=$(printf '0x%X' "$4")
target_led=$5

set_all_led()
{
    # get led count
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x1D 1>/dev/null`;	#Set Address High byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,0x0C 1>/dev/null`;	#Set Address Low byte
    tmp=`$agt -i=0 -oemi2cr:0,0xCE,0x08`;
    led_count=${tmp##*:}
    
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x80 1>/dev/null`;	#Set Address High byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,0x21 1>/dev/null`;	#Set Address Low byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x01,0x01 1>/dev/null`;	#Set Static effect
    
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x81 1>/dev/null`;
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,0x60 1>/dev/null`;	#Set RGB duties
    for((i=0;i<$led_count;i++));
    do
        `$agt -i=0 -oemi2cw:0,0xCE,0x03,0x06,$target_led_red,$target_led_green,$target_led_blue,$target_led_red,$target_led_green,$target_led_blue 1>/dev/null`;
    done
    
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x80 1>/dev/null`;	#Set Address High byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,0x2F 1>/dev/null`;	#Set Address Low byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x01,0x01 1>/dev/null`;	#Setting Apply
}

set_single_led()
{
    let led_addr=$target_led*3+0x60
    led_addr=$(printf '0x%X' "$led_addr")
    echo $led_addr
    
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x80 1>/dev/null`;	#Set Address High byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,0x21 1>/dev/null`;	#Set Address Low byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x01,0x01 1>/dev/null`;	#Set Static effect
    
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x81 1>/dev/null`;
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,$led_addr 1>/dev/null`;	#Set RGB duties
    `$agt -i=0 -oemi2cw:0,0xCE,0x03,0x03,$target_led_red,$target_led_green,$target_led_blue 1>/dev/null`;
    
    `$agt -i=0 -oemi2cw:0,0xCE,0x09,0x80 1>/dev/null`;	#Set Address High byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x0A,0x2F 1>/dev/null`;	#Set Address Low byte
    `$agt -i=0 -oemi2cw:0,0xCE,0x01,0x01 1>/dev/null`;	#Setting Apply
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
        *) help;;
    esac
done
