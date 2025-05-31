#include "codegen.h"
#include "ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FunctionDef **global_functions = NULL;
static int global_function_count = 0;

// 寄存器名称映射
const char *reg_names[] = {
    [REG_EAX] = "eax",
    [REG_EBX] = "ebx",
    [REG_ECX] = "ecx",
    [REG_EDX] = "edx",
    [REG_ESI] = "esi",
    [REG_EDI] = "edi",
    [REG_ESP] = "esp",
    [REG_EBP] = "ebp"};

// 安全转义字符串函数
void safe_fprintf_string(FILE *output, const char *str)
{
    putc('"', output);
    for (const char *p = str; *p; p++)
    {
        switch (*p)
        {
        case '\n':
            fputs("\\n", output);
            break;
        case '\r':
            fputs("\\r", output);
            break;
        case '\t':
            fputs("\\t", output);
            break;
        case '\"':
            fputs("\\\"", output);
            break;
        case '\\':
            fputs("\\\\", output);
            break;
        default:
            if (*p < 32 || *p > 126)
            {
                fprintf(output, "\\x%02x", (unsigned char)*p);
            }
            else
            {
                putc(*p, output);
            }
        }
    }
    putc('"', output);
}

void generate_x86_asm(IRProgram *program, FILE *output)
{
    // 输出汇编头
    fprintf(output, "BITS 32\n");

    // 输出数据段
    fprintf(output, "section .data\n");

    int str_count = 0;
    const char **string_labels = malloc(program->count * sizeof(char *));
    int *str_index_map = calloc(program->count, sizeof(int));

    // 第一遍：处理所有字符串数据
    for (int i = 0; i < program->count; i++)
    {
        IRInstruction inst = program->instructions[i];

        if (inst.type == IR_DATA && inst.operands[0].type == OP_STRING)
        {
            string_labels[str_count] = malloc(32);
            sprintf((char *)string_labels[str_count], "str_%d", str_count);
            // 输出: 标签 db 字符串, 0
            fprintf(output, "%s db ", string_labels[str_count]);
            safe_fprintf_string(output, inst.operands[0].str);
            fprintf(output, ", 0\n");
            str_index_map[i] = str_count + 1;
            str_count++;
        }
        else
        {
            str_index_map[i] = 0;
        }
    }

    // 输出文本段
    fprintf(output, "\nsection .text\n");
    fprintf(output, "global _start\n");
    fprintf(output, "_start:\n");

    // 第二遍：处理所有指令
    for (int i = 0; i < program->count; i++)
    {
        IRInstruction inst = program->instructions[i];
        const char *comment = inst.comment;

        // 添加注释
        if (comment && *comment)
        {
            fprintf(output, "    ; %s\n", comment);
        }

        switch (inst.type)
        {
        case IR_DATA:
            // 字符串数据已在.data段处理，跳过
            break;

        case IR_LABEL:
            // 标签定义 (函数入口)
            fprintf(output, "%s:\n", inst.operands[0].label);
            break;

        case IR_MOV:
            if (inst.operands[0].type == OP_REG && inst.operands[1].type == OP_IMM)
            {
                fprintf(output, "    mov %s, %d\n",
                        reg_names[inst.operands[0].reg],
                        inst.operands[1].imm);
            }
            break;

        case IR_PUSH:
            if (inst.operands[0].type == OP_IMM)
            {
                fprintf(output, "    push %d\n", inst.operands[0].imm);
            }
            else if (inst.operands[0].type == OP_LABEL)
            {
                fprintf(output, "    push %s\n", inst.operands[0].label);
            }
            else if (str_index_map[i] > 0)
            {
                // 处理与字符串相关的push
                fprintf(output, "    push %s\n", string_labels[str_index_map[i] - 1]);
            }
            break;

        case IR_CALL:
            fprintf(output, "    call %s\n", inst.operands[0].label);
            break;

        case IR_RET:
            fprintf(output, "    ret\n");
            break;

        case IR_SYSCALL:
            fprintf(output, "    int 0x80\n");
            break;

        case IR_POP:
            // 清理栈空间
            fprintf(output, "    add esp, %d\n", inst.operands[0].imm);
            break;

        case IR_JMP:
            fprintf(output, "    jmp %s\n", inst.operands[0].label);
            break;

        default:
            // 跳过不支持的类型
            break;
        }
    }

    // 确保添加程序退出
    fprintf(output, "    ; Exit syscall\n");
    fprintf(output, "    mov eax, 1\n");
    fprintf(output, "    mov ebx, 0\n");
    fprintf(output, "    int 0x80\n");

    // 清理资源
    for (int i = 0; i < str_count; i++)
    {
        free((void *)string_labels[i]);
    }
    free(string_labels);
    free(str_index_map);
}