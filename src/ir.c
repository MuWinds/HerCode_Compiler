#include "ir.h"
#include <stdlib.h>
#include <string.h>

IRProgram *create_ir_program()
{
    IRProgram *program = malloc(sizeof(IRProgram));
    program->count = 0;
    program->capacity = 128;
    program->instructions = malloc(program->capacity * sizeof(IRInstruction));
    return program;
}

void ir_add_instruction(IRProgram *program, IRInstruction inst)
{
    if (program->count >= program->capacity)
    {
        program->capacity *= 2;
        program->instructions = realloc(program->instructions,
                                        program->capacity * sizeof(IRInstruction));
    }
    program->instructions[program->count++] = inst;
}

void free_ir_program(IRProgram *program)
{
    free(program->instructions);
    free(program);
}

// 为函数调用生成IR
static void generate_call_ir(IRProgram *program, const char *func_name)
{
    // 准备系统调用参数
    IRInstruction call_inst = {
        .type = IR_CALL,
        .operands = {{.type = OP_LABEL, .label = func_name}},
        .comment = "Function call"};
    ir_add_instruction(program, call_inst);
}

// 为say语句生成IR
static void generate_say_ir(IRProgram *program, const char *message)
{
    // 在.data段定义字符串
    IRInstruction data_inst = {
        .type = IR_DATA,
        .operands = {{.type = OP_STRING, .str = message}},
        .comment = message};
    ir_add_instruction(program, data_inst);

    // 准备系统调用参数
    IRInstruction push_addr_inst = {
        .type = IR_PUSH,
        .operands = {{.type = OP_IMM, .imm = 1}}, // 字符串长度
        .comment = "Prepare string length"};
    ir_add_instruction(program, push_addr_inst);

    IRInstruction push_msg_inst = {
        .type = IR_PUSH,
        .operands = {{.type = OP_LABEL, .label = message}},
        .comment = "Prepare string address"};
    ir_add_instruction(program, push_msg_inst);

    IRInstruction push_fd_inst = {
        .type = IR_PUSH,
        .operands = {{.type = OP_IMM, .imm = 1}}, // stdout
        .comment = "Prepare fd (stdout)"};
    ir_add_instruction(program, push_fd_inst);

    // write() 系统调用
    IRInstruction syscall_inst = {
        .type = IR_SYSCALL,
        .operands = {{.type = OP_IMM, .imm = 4}}, // write syscall number
        .comment = "System call: write"};
    ir_add_instruction(program, syscall_inst);

    // 清理栈
    IRInstruction clean_stack_inst = {
        .type = IR_POP,
        .operands = {{.type = OP_IMM, .imm = 0}},
        .comment = "Clean up stack"};
    ir_add_instruction(program, clean_stack_inst);
}

// 生成函数定义的IR
static void generate_function_ir(IRProgram *program, ASTNode *node)
{
    // 函数标签
    IRInstruction label_inst = {
        .type = IR_LABEL,
        .operands = {{.type = OP_LABEL, .label = node->value}},
        .comment = node->value};
    ir_add_instruction(program, label_inst);

    // 函数体
    for (int i = 0; i < node->body_count; i++)
    {
        ASTNode *stmt = node->body[i];

        if (stmt->type == STMT_SAY)
        {
            generate_say_ir(program, stmt->value);
        }
        else if (stmt->type == STMT_FUNCTION_CALL)
        {
            generate_call_ir(program, stmt->value);
        }
    }

    // 函数返回
    IRInstruction ret_inst = {
        .type = IR_RET,
        .comment = "Function return"};
    ir_add_instruction(program, ret_inst);
}

// AST转IR的主函数
IRProgram *ast_to_ir(ASTNode **nodes, int count)
{
    IRProgram *program = create_ir_program();

    // 添加入口点
    IRInstruction entry_label = {
        .type = IR_LABEL,
        .operands = {{.type = OP_LABEL, .label = "_start"}},
        .comment = "Program entry point"};
    ir_add_instruction(program, entry_label);

    // 首先处理函数定义
    for (int i = 0; i < count; i++)
    {
        if (nodes[i]->type == STMT_FUNCTION_DEF)
        {
            generate_function_ir(program, nodes[i]);
        }
    }

    // 然后处理主程序体
    for (int i = 0; i < count; i++)
    {
        if (nodes[i]->type != STMT_FUNCTION_DEF)
        {
            if (nodes[i]->type == STMT_SAY)
            {
                generate_say_ir(program, nodes[i]->value);
            }
            else if (nodes[i]->type == STMT_FUNCTION_CALL)
            {
                generate_call_ir(program, nodes[i]->value);
            }
        }
    }

    // 程序结束
    IRProgram *final_program = create_ir_program();

    // 添加.data段
    IRInstruction section_data = {
        .type = IR_DATA,
        .operands = {{.type = OP_STRING, .str = ".data"}},
        .comment = "Data section start"};
    ir_add_instruction(final_program, section_data);

    // 添加.text段
    IRInstruction section_text = {
        .type = IR_DATA,
        .operands = {{.type = OP_STRING, .str = ".text"}},
        .comment = "Text section start"};
    ir_add_instruction(final_program, section_text);

    // 添加全局入口点
    IRInstruction global_start = {
        .type = IR_DATA,
        .operands = {{.type = OP_STRING, .str = ".global _start"}},
        .comment = "Export entry point"};
    ir_add_instruction(final_program, global_start);

    // 合并IR指令
    for (int i = 0; i < program->count; i++)
    {
        ir_add_instruction(final_program, program->instructions[i]);
    }

    // 添加程序退出
    IRInstruction mov_eax = {
        .type = IR_MOV,
        .operands = {{.type = OP_REG, .reg = REG_EAX},
                     {.type = OP_IMM, .imm = 1}},
        .comment = "Exit syscall"};
    ir_add_instruction(final_program, mov_eax);

    IRInstruction xor_ebx = {
        .type = IR_MOV,
        .operands = {{.type = OP_REG, .reg = REG_EBX},
                     {.type = OP_IMM, .imm = 0}},
        .comment = "Exit code 0"};
    ir_add_instruction(final_program, xor_ebx);

    IRInstruction int80 = {
        .type = IR_SYSCALL,
        .operands = {{.type = OP_IMM, .imm = 0x80}},
        .comment = "Execute syscall"};
    ir_add_instruction(final_program, int80);

    free_ir_program(program);
    return final_program;
}