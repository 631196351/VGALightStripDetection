cmake_minimum_required(VERSION 3.5)

project(vga-light-strip-detection VERSION 2.0.1.4)

# 静态链接
# set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

# check results and add flag
if(COMPILER_SUPPORTS_CXX11)#
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
elseif(COMPILER_SUPPORTS_CXX0X)#
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

if(CMAKE_HOST_WIN32)
    message(STATUS "CMAKE_HOST_WIN32->" ${CMAKE_HOST_WIN32})
    add_definitions(-DWINDOWS)
    add_definitions(-DVENDER_EXTRA)    #I2CWrap call api style
    add_definitions(-DIMSHOWIMAGE)     #Open imshow
elseif(CMAKE_HOST_UNIX)
    message(STATUS "CMAKE_HOST_UNIX->" ${CMAKE_HOST_UNIX})
    add_definitions(-DLINUX)
endif()

# set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-${CMAKE_BUILD_TYPE})

find_package(OpenCV REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} 
    ${PROJECT_SOURCE_DIR}/AgingLog.cpp
    ${PROJECT_SOURCE_DIR}/CommonColor.cpp
    ${PROJECT_SOURCE_DIR}/ConfigData.cpp
    ${PROJECT_SOURCE_DIR}/ErrorCode.cpp
    ${PROJECT_SOURCE_DIR}/I2CWrap.cpp
    ${PROJECT_SOURCE_DIR}/PreDefine.cpp
    ${PROJECT_SOURCE_DIR}/RandomLitoff.cpp
    ${PROJECT_SOURCE_DIR}/VideoCard.cpp
    ${PROJECT_SOURCE_DIR}/utility.cpp
    ${PROJECT_SOURCE_DIR}/main2.cpp
    ${PROJECT_SOURCE_DIR}/SpdMultipleSinks.cpp)

link_directories(${OpenCV_LIBRARY_DIRS})

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
#target_link_libraries(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/spdlog) 

# test message
message(STATUS "PROJECT_SOURCE_DIR -> " ${PROJECT_SOURCE_DIR})
message(STATUS "PROJECT_BINARY_DIR -> " ${PROJECT_BINARY_DIR})
message(STATUS "CMAKE_BUILD_TYPE -> " ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_CXX_FLAGS -> " ${CMAKE_CXX_FLAGS})
message(STATUS "CMAKE_INSTALL_RPATH -> " ${CMAKE_INSTALL_RPATH})
message(STATUS "CMAKE_INSTALL_RPATH_USE_LINK_RPATH -> " ${CMAKE_INSTALL_RPATH_USE_LINK_RPATH})
message(STATUS "CMAKE_BUILD_WITH_INSTALL_RPATH -> " ${CMAKE_BUILD_WITH_INSTALL_RPATH})
message(STATUS "COMPILE_DEFINITIONS -> " ${COMPILE_DEFINITIONS})

# add_custom_command(
#   TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND echo "Copying toolset folder to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" && cp -r ${PROJECT_SOURCE_DIR}/tool ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
#   COMMAND echo "Scanning and copying dependencies of target vga-light-strip-detection" &&  mkdir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib
#   COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tool/ldd.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib
#   COMMAND echo "Copying libi2c.so.0 to lib" && cp /usr/lib/x86_64-linux-gnu/libi2c.so.0 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib
#   COMMAND echo "Copying 3c.json" && cp ${PROJECT_SOURCE_DIR}/3c.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
#   COMMAND echo "Copying test.sh" && cp ${PROJECT_SOURCE_DIR}/test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
#   COMMAND echo "Tar package" && tar -cf ${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}.tar ${PROJECT_NAME}-${CMAKE_BUILD_TYPE}
#   COMMAND echo "Done"
 
#   VERBATIM)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND echo "Copying toolset folder to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" && cp -r ${PROJECT_SOURCE_DIR}/tool .
  COMMAND echo "Scanning and copying dependencies of target vga-light-strip-detection" &&  mkdir ./lib
  COMMAND ./tool/ldd.sh ./${PROJECT_NAME} ./lib
  COMMAND echo "Copying libi2c.so.0 to lib" && cp /usr/lib/x86_64-linux-gnu/libi2c.so.0 ./lib
  COMMAND echo "Copying 3c.json" && cp ${PROJECT_SOURCE_DIR}/3c.json .
  COMMAND echo "Copying test.sh" && cp ${PROJECT_SOURCE_DIR}/test.sh .
  COMMAND echo "Tar package" && cd ../ && tar -cf ${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}.tar ${PROJECT_NAME}-${CMAKE_BUILD_TYPE}
  COMMAND echo "Done"
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  VERBATIM)


# add_custom_command(
#   TARGET ${PROJECT_NAME} POST_BUILD
#   COMMAND tar -cvf ${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}.tar ../release_static
#   COMMENT "Tar package"
#   VERBATIM)

##---------------- 以下为用CPack打包成deb包的内容 ----------------##
# 指定deb包的版本号
# set(_VERSION_MAJOR 2)
# set(_VERSION_MINOR 1)
# set(_VERSION_PATCH 4)
# set(CPACK_PACKAGE_VERSION_MAJOR "${_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${_VERSION_MINOR}")
# set(CPACK_PACKAGE_VERSION_PATCH "${_VERSION_PATCH}")
# 指定CPack生成的是deb包（还可以是其他包如 <.7z> <.zip> 等）
# set(CPACK_GENERATOR "DEB")
# 指定包名
# set(CPACK_PACKAGE_NAME "vgalightstripdetection")
# 指定程序名，就是程序安装后的名字
# set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
# 指定运行的计算机架构
# set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
# 指定所需的依赖
# file(GLOB_RECURSE DEPENDS_LIBRARY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib)
# message(STATUS "The DEPENDS_LIBRARY->" ${DEPENDS_LIBRARY})

# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgrpc++")
# 设置包作者的联系方式
# set(CPACK_PACKAGE_CONTACT "sheldon_li@asus.com")
# 设置维护人
# set(CPACK_DEBIAN_PACKAGE_MAINTAINER "members of support@gmail.cn")

# 指定需要安装的文件
# INSTALL(TARGETS LinkLibraryDemo PrinterInt PrinterChar
        # RUNTIME DESTINATION /tmp/cmake_demo
        #LIBRARY DESTINATION /tmp/cmake_demo    # 通常，编译器只会在 </usr/lib> 和 </lib> 中找头文件，如果
        #ARCHIVE DESTINATION /tmp/cmake_demo    # 将库文件放在其他目录，可能会引起找不到库文件错误
        # )
# SET(CMAKE_INSTALL_PREFIX "/usr/local/vga-light-strip-detection")
# install(DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib/" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
# install(DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tool/" DESTINATION "${CMAKE_INSTALL_PREFIX}/tool" PATTERN "*" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE)
# install(FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/3c.json" DESTINATION "${CMAKE_INSTALL_PREFIX}")
# install(PROGRAMS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test.sh" DESTINATION "${CMAKE_INSTALL_PREFIX}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE)
# install(PROGRAMS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/VGALightStripDetection" DESTINATION "${CMAKE_INSTALL_PREFIX}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE)

# include(CPack)
