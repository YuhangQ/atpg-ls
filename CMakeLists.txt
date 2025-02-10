cmake_minimum_required(VERSION 3.20)

project(atpg-ls)

# 子模块
add_subdirectory(tg-pro)
add_subdirectory(src)

target_link_libraries(atpg atpg_backend sat_solver)