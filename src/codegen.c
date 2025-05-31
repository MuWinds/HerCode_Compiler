#include "codegen.h"
#include "ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FunctionDef **global_functions = NULL;
static int global_function_count = 0;

const char *reg_names[] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp"};

void generate_x86_asm(IRProgram *program, FILE *output)
{
    // 输出汇编头
    fprintf(output, "BITS 32\n");
    fprintf(output, "section .data\n");

    // 处理.data段
    for (int i = 0; i < program->count; i++)
    {
        IRInstruction inst = program->instructions[i];

        switch (inst.type)
        {
        case IR_DATA:
            if (inst.operands[0].type == OP_STRING)
            {
                fprintf(output, "%s\n", inst.operands[0].str);
            }
            break;
        default:
            // 其他指令不属于.data段
            break;
        }
    }

    // 开始.text段
    fprintf(output, "\nsection .text\n");
    fprintf(output, "global _start\n");
    fprintf(output, "_start:\n");

    // 处理.text段指令
    for (int i = 0; i < program->count; i++)
    {
        IRInstruction inst = program->instructions[i];

        switch (inst.type)
        {
        case IR_DATA:
            // 这些在.data段已经处理
            break;

        case IR_MOV:
            fprintf(output, "    mov %s, %d\n",
                    reg_names[inst.operands[0].reg],
                    inst.operands[1].imm);
            break;

        case IR_PUSH:
            if (inst.operands[0].type == OP_IMM)
            {
                fprintf(output, "    push %d\n", inst.operands[0].imm);
            }
            break;

        case IR_CALL:
            fprintf(output, "    call %s\n", inst.operands[0].label);
            break;

        case IR_RET:
            fprintf(output, "    ret\n");
            break;

        case IR_JMP:
            fprintf(output, "    jmp %s\n", inst.operands[0].label);
            break;

        case IR_LABEL:
            fprintf(output, "%s:\n", inst.operands[0].label);
            break;

        case IR_SYSCALL:
            fprintf(output, "    int 0x80\n");
            break;

        case IR_POP:
            fprintf(output, "    add esp, %d\n", inst.operands[0].imm);
            break;

        default:
            fprintf(output, "    ; Unsupported IR instruction: %d\n", inst.type);
            break;
        }

        // 添加注释
        if (inst.comment)
        {
            fprintf(output, "    ; %s\n", inst.comment);
        }
    }
}