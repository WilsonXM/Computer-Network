cmake_minimum_required(VERSION 3.15)
project(Server LANGUAGES CXX)

# 生成可执行文件，包含所有.cpp文件
add_executable( server server.cpp protocol.cpp )

# 为生成可执行文件，提供头文件包含目录
target_include_directories( server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/protocol.h )

# 静态链接库
target_link_libraries( server PRIVATE ws2_32 )