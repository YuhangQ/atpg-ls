cmake_minimum_required(VERSION 3.20)

project(atpg)

set(CMAKE_CXX_STANDARD 17)

# 设置编译器优化选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native -flto -static")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -static")

# 设置源文件和头文件的路径
aux_source_directory(${PROJECT_SOURCE_DIR} SOURCES)
set(INCLUDES ${PROJECT_SOURCE_DIR})

# 添加头文件
include_directories(${INCLUDES}, ${PROJECT_SOURCE_DIR}/../tg-pro/src)

# 生成可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})