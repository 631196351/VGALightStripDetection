## Project Dependents
### 工具链准备
> [编译器] sudo apt-get install build-essential
> 
> [必须的] sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
> 
> [可选的] sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper
### 静态编译OpenCV
> cd opencv-4.5.1
> 
> mkdir && cd build
> 
> cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_JPEG=ON \
    -DBUILD_PNG=ON \
    -DBUILD_PROTOBUF=ON \
    -DBUILD_TIFF=ON \
    -DBUILD_ZLIB=ON \
    -DBUILD_WEBP=ON \
    -DBUILD_JASPER=ON
> 
> make -j8
>
>sudo make Install

### 控灯工具依赖显卡驱动
> 
> N卡依赖 /tool/nv-led.sh, and /tool/i2c-tools/* 工具链， 及 /usr/lib/x86_64-linux-gnu/libi2c.so.0 库文件
> 
> A 卡依赖 /tool/agt_mcu, and /tool/amd-led.sh


## 开发过程
> 1. 在Linux环境下调整显卡曝光熟悉不同于在Windows环境下
> 在Linux下底层通过v4l2 来控制camera， 曝光属性∈[0, 5000]
>
> 2. 3c.json 文件字符编码注意事项,一般要设定成UTF-8, 若文件并非 UTF-8 编码，可以把字节流用 EncodedInputStream 包装

## 编译过程
> mkdir build && cd build
> 
> cmake ..
> 
> make -j8
> 
> 编译完会立即把项目打包成tar
> 

## 调试过程
> gdb 调试
> 
> 需要cd 到 可执行文件所在目录，否则无法打开3c.json 文件
> 
> gdb VGALightStripeDetect
> 
> set args "210992023501743" "ROG-STRIX-RTX3070TI-O8G-GAMING-2I3S"
> 
> b ConfigData.cpp:174    //在该文件的174行打断点
> 
> r               // 运行， 运行至第一个断点处
> 
> s               // 单步跟踪 F11
> 
> n               // 单步跟踪 F10

## 部署过程
> 部署到其他平台时，在运行前需要call 
> 
> /tool/give-me-device-permissions.sh 
> 来让普通用户获取显卡和相机的使用权限
>
> 然后在调用test.sh 来执行程式，需要注意的是目前程式的PPID, ModelName是写死的， 真正部署时需要上游tool 来call并传参给它

## 尚存问题
> A 卡因agt_mcu 太慢的问题， 测22颗灯需要跑5分40秒+
> 
> A/N 卡都依赖驱动，否则无法工作