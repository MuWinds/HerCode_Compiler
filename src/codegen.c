#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FunctionDef **global_functions = NULL;
static int global_function_count = 0;
static int global_functions_capacity = 0;

// 字符串转义函数
char *escape_string(const char *input)
{
    if (!input)
        return strdup("");

    // 计算需要多少额外空间
    size_t len = strlen(input);
    size_t extra = 0;
    for (const char *c = input; *c; c++)
    {
        if (*c == '\\' || *c == '"')
            extra++;
    }

    // 分配内存
    char *output = malloc(len + extra + 1);
    if (!output)
        return NULL;

    char *dst = output;
    for (const char *src = input; *src; src++)
    {
        if (*src == '\\' || *src == '"')
        {
            *dst++ = '\\'; // 添加转义字符
        }
        *dst++ = *src;
    }
    *dst = '\0';
    return output;
}

void generate_c_code(const char *c_header, ASTNode **nodes, int count, FILE *output)
{
    // 写入C头文件部分
    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "#include <stdlib.h>\n");
    fprintf(output, "#include <string.h>\n");
    fprintf(output, "#include <math.h>\n");
    fprintf(output, "#include <time.h>\n");
    fprintf(output, "#include <ctype.h>\n");
    fprintf(output, "#include <float.h>\n");
    fprintf(output, "#include <assert.h>\n");
    fprintf(output, "#include <errno.h>\n");
    fprintf(output, "#include <stddef.h>\n");
    fprintf(output, "#include <signal.h>\n");
    fprintf(output, "#include <setjmp.h>\n");
    fprintf(output, "#include <locale.h>\n\n");

    // 首先收集所有函数定义
    for (int i = 0; i < count; i++)
    {
        if (nodes[i]->type == STMT_FUNCTION_DEF)
        {
            // 检查是否需要扩容
            if (global_function_count >= global_functions_capacity)
            {
                int new_capacity = global_functions_capacity == 0 ? 8 : global_functions_capacity * 2;
                FunctionDef **new_functions = realloc(global_functions, new_capacity * sizeof(FunctionDef *));
                if (!new_functions)
                {
                    fprintf(stderr, "Memory allocation failed\n");
                    exit(1);
                }
                global_functions = new_functions;
                global_functions_capacity = new_capacity;
            }

            FunctionDef *def = malloc(sizeof(FunctionDef));
            def->name = strdup(nodes[i]->value);
            def->body = nodes[i]->body;
            def->body_count = nodes[i]->body_count;

            global_functions[global_function_count++] = def;
        }
    }

    // 生成函数声明（所有函数都返回void）
    fprintf(output, "\n/* Function declarations */\n");
    for (int i = 0; i < global_function_count; i++)
        fprintf(output, "void function_%s();\n", global_functions[i]->name);
    // 生成main函数
    fprintf(output, "\nint main() {\n");
    // 如果有外部C代码头文件，写入它
    if (c_header != NULL)
    {
        // 逐行处理 c_header
        const char *start = c_header;
        const char *end;
        while ((end = strchr(start, '\n')) != NULL)
        { // 找到换行符
            // 输出：制表符 + 当前行（不含换行符）
            fprintf(output, "\t%.*s\n", (int)(end - start), start);
            start = end + 1; // 移到下一行
        }
        // 输出剩余部分（最后一行）
        if (*start != '\0')
        {
            fprintf(output, "\t%s\n", start);
        }
    }
    for (int i = 0; i < count; i++)
    {
        if (nodes[i]->type == STMT_SAY)
            fprintf(output, "    printf(\"%%s\\n\", \"%s\");\n", nodes[i]->value);
        else if (nodes[i]->type == STMT_FUNCTION_CALL)
            fprintf(output, "    function_%s();\n", nodes[i]->value);
    }
    fprintf(output, "    return 0;\n}\n");

    // 生成函数实现
    fprintf(output, "\n/* Function implementations */\n");
    for (int i = 0; i < global_function_count; i++)
    {
        FunctionDef *def = global_functions[i];
        fprintf(output, "void function_%s() {\n", def->name);

        for (int j = 0; j < def->body_count; j++)
        {
            ASTNode *stmt = def->body[j];

            if (stmt->type == STMT_SAY)
            {
                char *escaped = escape_string(stmt->value);
                fprintf(output, "    printf(\"%%s\\n\", \"%s\");\n", escaped);
                free(escaped);
            }
            else if (stmt->type == STMT_FUNCTION_CALL)
            {
                fprintf(output, "    function_%s();\n", stmt->value);
            }
        }

        fprintf(output, "}\n\n");
    }

    // 清理
    for (int i = 0; i < global_function_count; i++)
    {
        free(global_functions[i]);
    }
    free(global_functions);
    global_function_count = 0;
}

void compile(char *c_filename, char *output_name)
{
    char cmd[256];
    sprintf(cmd, "gcc -O2 -o %s %s", output_name, c_filename);
    system(cmd);
}
