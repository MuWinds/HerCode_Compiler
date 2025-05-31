cmake_minimum_required(VERSION 3.10)
project(HercodeCompiler C)

# 设置C标准
set(CMAKE_C_STANDARD 11)

# 包含目录
include_directories(include)

# 明确列出所有源文件
set(SOURCE_FILES
    src/main.c
    src/lexer.c
    src/parser.c
    src/ast.c
    src/ir.c        # 包含IR生成器的实现
    src/codegen.c  # 包含x86汇编生成器的实现
)

# 生成可执行文件
add_executable(hercode_compiler ${SOURCE_FILES})
add_compile_options(-Wall -Werror -Wstrict-prototypes -Wmissing-prototypes -O2 -Os)

install(TARGETS hercode_compiler DESTINATION bin)