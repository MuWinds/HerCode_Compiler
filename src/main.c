#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "ast.h"

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "文件打开失败");
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
        fprintf(stderr, "Usage: %s <source_file> [--out=output_name] [-r]\n", argv[0]);
        return 1;
    }

    // 解析参数
    char output_name[256] = "hercode.exe";
    int remove_temp = 0;
    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "--out=", 6) == 0) {
            strncpy(output_name, argv[i] + 6, sizeof(output_name) - 1);
            output_name[sizeof(output_name) - 1] = '\0';
            // 自动补全.exe后缀
            size_t len = strlen(output_name);
            if (len < 4 || strcmp(output_name + len - 4, ".exe") != 0) {
                if (len < sizeof(output_name) - 4) {
                    strcat(output_name, ".exe");
                }
            }
        } else if (strcmp(argv[i], "-r") == 0) {
            remove_temp = 1;
        }
    }

    // 读取整个文件
    char *hercode_source = read_file(argv[1]);
    if (!hercode_source)
    {
        fprintf(stderr, "读取文件失败: %s\n", argv[1]);
        return 1;
    }

    // 输出分离结果用于调试
    printf("HerCode 源代码:\n%s\n", hercode_source);

    // 创建词法分析器和解析器
    Lexer *lexer = new_lexer(hercode_source);
    Parser *parser = new_parser(lexer);

    // 解析程序
    int node_count;
    printf("[MAIN] 调用 parse_program...\n");
    ASTNode **nodes = parse_program(parser, &node_count);
    printf("[DEBUG] parse_program 返回 node_count=%d, nodes ptr=%p\n", node_count, (void*)nodes);
    if (!nodes) {
        fprintf(stderr, "解析失败！\n");
        return 1;
        // 不再提前return，继续执行后续流程
    }
    printf("总共解析 %d 个节点\n", node_count);

    // 生成C代码
    printf("[MAIN] 准备生成 C 代码...\n");
    for (int i = 0; i < node_count; i++) {
        printf("[MAIN][DEBUG] 节点[%d] 类型 = %d\n", i, nodes[i] ? nodes[i]->type : -1);
    }
        // 生成C文件名，与最终exe同名但扩展名为.c
    char c_filename[512];
    strncpy(c_filename, output_name, sizeof(c_filename) - 1);
    c_filename[sizeof(c_filename) - 1] = '\0';
    char *dot = strrchr(c_filename, '.');
    if (dot) {
        strcpy(dot, ".c");
    } else {
        strcat(c_filename, ".c");
    }

    FILE *c_file = fopen(c_filename, "wb");
    if (!c_file)
    {
        fprintf(stderr, "创建 C 文件失败");
        return 1;
    }
    // 手动写入 UTF-8 BOM
    unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    fwrite(bom, 1, 3, c_file);
    printf("[MAIN] 调用 generate_c_code...\n");
    generate_c_code(nodes, node_count, c_file);
    fflush(c_file);
    printf("[MAIN] C 代码生成完成。\n");
    fclose(c_file);
    // 检查文件是否存在
    FILE *check_file = fopen(c_filename, "r");
    if (check_file) {
        printf("[MAIN] %s 成功生成！\n", c_filename);
        fclose(check_file);
    } else {
        printf("[MAIN] %s 生成失败！\n", c_filename);
    }

    // 编译
    compile(c_filename, output_name);

    // 按需清理临时文件
    if (remove_temp) {
        remove(c_filename);
#ifdef _MSC_VER
        char objfile[512];
        strncpy(objfile, c_filename, sizeof(objfile) - 1);
        objfile[sizeof(objfile) - 1] = '\0';
        char *dot2 = strrchr(objfile, '.');
        if (dot2) strcpy(dot2, ".obj");
        remove(objfile);
        printf("已删除 %s 和 %s\n", c_filename, objfile);
#else
        printf("已删除 %s\n", c_filename);
#endif
    }

    free_parser(parser);

    for (int i = 0; i < node_count; i++)
    {
        free_node(nodes[i]);
    }
    free(nodes);

    printf("成功生成: %s\n", output_name);
    return 0;
}