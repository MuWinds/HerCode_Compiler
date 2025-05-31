cmake_minimum_required(VERSION 3.10)
project(HercodeCompiler C)

# 设置C标准
set(CMAKE_C_STANDARD 11)

# 包含目录
include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.c")

# 生成可执行文件
add_executable(hercode_compiler ${SOURCES})
add_compile_options(-Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -O2 -Os)

install(TARGETS hercode_compiler DESTINATION bin)