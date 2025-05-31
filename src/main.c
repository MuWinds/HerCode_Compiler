#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "ast.h"
#include "ir.h"

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("File opening failed");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

void separate_header(const char *source, const char *magic_string,
                     char **c_header, char **hercode_source)
{
    *c_header = NULL;
    *hercode_source = NULL;

    char *magic_pos = strstr(source, magic_string);
    if (magic_pos == NULL)
    {
        return; // 没有找到特殊字符串
    }

    // 确保特殊字符串在行首
    if (magic_pos != source)
    {
        char *prev_char = magic_pos - 1;
        if (*prev_char != '\n' && *prev_char != '\r')
        {
            return; // 不在行首
        }
    }

    // 查找行结束位置
    char *line_end = strchr(magic_pos, '\n');
    if (line_end == NULL)
    {
        // 如果没有换行符，特殊字符串后没有内容
        size_t header_size = magic_pos - source;
        *c_header = malloc(header_size + 1);
        if (*c_header)
        {
            strncpy(*c_header, source, header_size);
            (*c_header)[header_size] = '\0';
        }
        *hercode_source = ""; // 空字符串
        return;
    }

    // 计算C头部分的大小
    size_t header_size = magic_pos - source;
    *c_header = malloc(header_size + 1);
    if (*c_header)
    {
        strncpy(*c_header, source, header_size);
        (*c_header)[header_size] = '\0';
    }

    // HerCode部分从下一行开始
    *hercode_source = line_end + 1;

    // 特殊处理CRLF换行
    if (*line_end == '\n' && line_end > magic_pos && *(line_end - 1) == '\r')
    {
        // 如果前面有CR，跳过它
        *hercode_source = line_end;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <source_file> [output_name]\n", argv[0]);
        return 1;
    }

    // 读取源文件
    char *source = read_file(argv[1]);
    if (!source)
    {
        fprintf(stderr, "Error reading file: %s\n", argv[1]);
        return 1;
    }
    // 尝试分离C头部分（已废弃，但要对齐需求所以还得保留）
    char *c_header = NULL;
    char *hercode_source = NULL;
    separate_header(source, "Hello! Her World", &c_header, &hercode_source);

    // 验证分离结果
    if (hercode_source == NULL)
    {
        hercode_source = source; // 如果分离失败，使用整个文件
    }

    // 输出分离结果用于调试
    printf("HerCode Source to Parse:\n%s\n", hercode_source);
    // 创建词法分析器和解析器
    Lexer *lexer = new_lexer(hercode_source);
    Parser *parser = new_parser(lexer);

    // 解析程序
    int node_count;
    ASTNode **nodes = parse_program(parser, &node_count);

    // 生成中间表示
    IRProgram *ir_program = ast_to_ir(nodes, node_count);

    // 生成目标代码
    char *output_name = argc >= 3 ? argv[2] : "output";

    // 生成汇编文件
    FILE *asm_file = fopen("temp.asm", "w");
    if (!asm_file)
    {
        perror("Error creating assembly file");
        return 1;
    }
    generate_x86_asm(ir_program, asm_file);
    fclose(asm_file);

    // 汇编和链接命令
    char cmd[1024];

    // 检测操作系统
#ifdef _WIN32
    // Windows 平台命令
    sprintf(cmd, "nasm -f win32 temp.asm -o temp.obj && "
                 "gcc -static temp.obj -o %s",
            output_name);
#else
    // Linux/macOS 平台命令
    sprintf(cmd, "nasm -f elf32 temp.asm -o temp.o && "
                 "ld -m elf_i386 temp.o -o %s",
            output_name);
#endif

    int result = system(cmd);
    if (result != 0)
    {
        fprintf(stderr, "Assembly and linking failed. Command: %s\n", cmd);
        return 1;
    }

    // 清理临时文件
    remove("temp.asm");
#ifdef _WIN32
    remove("temp.obj");
#else
    remove("temp.o");
#endif

    // 清理资源
    free_ir_program(ir_program);
    for (int i = 0; i < node_count; i++)
    {
        free_node(nodes[i]);
    }
    free(nodes);
    free_parser(parser);
    free(source);

    printf("Compilation successful. Output: %s\n", output_name);
    return 0;
}