cmake_minimum_required(VERSION 3.12)
project(server)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE "Release")  
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)  # IPO优化

# 包含目录设置
include_directories(
    /usr/include  # 显式添加常见路径
    /usr/local/include    # 第三方库常见安装位置
)


add_library(common STATIC
    ../../common/logger.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_SOURCE_DIR}/../../common
)

# 查找依赖库
find_package(Threads REQUIRED)

# 添加可执行文件
add_executable(
    server
    server.cpp 
)

target_link_libraries(server
    PRIVATE 
    Threads::Threads 
    common
    jsoncpp
    muduo_net
    muduo_base
    fmt
)

# 添加 client 可执行文件
add_executable(
    client
    client.cpp
)

target_link_libraries(client
    PRIVATE 
    Threads::Threads
    common
    jsoncpp
    muduo_net
    muduo_base
    fmt
)