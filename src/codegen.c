#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
void hercode_print(const char* msg) {
    DWORD written;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), msg, (DWORD)strlen(msg), &written, NULL);
}
void hercode_exit(int code) {
    ExitProcess(code);
}
#else
#include <unistd.h>
void hercode_print(const char* msg) {
    write(1, msg, strlen(msg));
}
void hercode_exit(int code) {
    exit(code);
}
#endif
static FunctionDef **global_functions = NULL;
static int global_function_count = 0;
static int global_functions_capacity = 0;

// 字符串转义函数
char *escape_string(const char *input)
{
    if (!input)
        return _strdup("");

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

void generate_c_code(ASTNode **nodes, int count, FILE *output)
{
    printf("[CODEGEN] generate_c_code 被调用, node_count=%d\n", count);

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
                    fprintf(stderr, "内存分配失败\n");
                    exit(1);
                }
                global_functions = new_functions;
                global_functions_capacity = new_capacity;
            }

            FunctionDef *def = malloc(sizeof(FunctionDef));
            def->name = _strdup(nodes[i]->value);
            def->body = nodes[i]->body;
            def->body_count = nodes[i]->body_count;

            global_functions[global_function_count++] = def;
        }
    }

    // 生成函数声明（所有函数都返回void）
    fprintf(output, "\n/* 函数声明 */\n");
    for (int i = 0; i < global_function_count; i++)
        fprintf(output, "void function_%s();\n", global_functions[i]->name);
    // 生成main函数
    fprintf(output, "\nint main() {\n");
    for (int i = 0; i < count; i++)
    {
        if (nodes[i]->type == STMT_SAY) {
            char *escaped = escape_string(nodes[i]->value);
            fprintf(output, "    printf(\"%s\\n\");\n", escaped);
            free(escaped);
        }
        else if (nodes[i]->type == STMT_FUNCTION_CALL)
        {
            fprintf(output, "    function_%s();\n", nodes[i]->value);
        }
        else if (nodes[i]->type == STMT_C_BLOCK || nodes[i]->type == AST_C_BLOCK)
        {
            if (nodes[i]->c_code)
            {
                fprintf(output, "%s\n", nodes[i]->c_code);
            }
        }
    }
    fprintf(output, "    return 0;\n}\n");

    // 生成函数实现
    fprintf(output, "\n/* 函数实现 */\n");
    for (int i = 0; i < global_function_count; i++)
    {
        FunctionDef *def = global_functions[i];
        fprintf(output, "void function_%s() {\n", def->name);

        for (int j = 0; j < def->body_count; j++)
        {
            ASTNode *stmt = def->body[j];
            printf("[CODEGEN] 函数 '%s' stmt %d: type=%d\n", def->name, j, stmt->type);
            if (stmt->type == STMT_SAY)
            {
                char *escaped = escape_string(stmt->value);
                fprintf(output, "    printf(\"%s\\n\");\n", escaped);
                free(escaped);
            }
            else if (stmt->type == STMT_FUNCTION_CALL)
            {
                fprintf(output, "    function_%s();\n", stmt->value);
            }
            else if (stmt->type == STMT_C_BLOCK || stmt->type == AST_C_BLOCK)
            {
                if (stmt->c_code)
                {
                    fprintf(output, "%s\n", stmt->c_code);
                }
            }
        }

        fprintf(output, "}\n\n");
    }

    // 如果没有任何节点，生成一个空的main函数，避免C文件为空
    if (count == 0) {
        fprintf(output, "int main() { return 0; }\n");
    }
    // 清理
    for (int i = 0; i < global_function_count; i++)
    {
        free(global_functions[i]);
    }
    free(global_functions);
    global_function_count = 0;
}

#ifdef _MSC_VER
#include <process.h>
#endif

void compile(char *c_filename, char *output_name)
{
    char cmd[512];
#ifdef _MSC_VER
    sprintf(cmd, "cl /utf-8 /nologo /Fe:%s %s", output_name, c_filename);
#else
    sprintf(cmd, "gcc -O2 -o %s %s", output_name, c_filename);
#endif
    system(cmd);
}