cmake_minimum_required(VERSION 3.10)
project(HerCode_Compiler C)

if (MSVC)
    add_compile_options(/WX- /wd4090)
endif()

# 设置C标准
set(CMAKE_C_STANDARD 11)

# 包含目录
include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.c")

# 生成可执行文件
add_executable(hercode_compiler ${SOURCES})
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -O2 -Os)
endif()

install(TARGETS hercode_compiler DESTINATION bin)