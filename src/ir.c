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

// 生成函数调用的IR
static void generate_call_ir(IRProgram *program, const char *func_name)
{
    IRInstruction call_inst = {
        .type = IR_CALL,
        .operands = {{.type = OP_LABEL, .label = func_name}},
        .comment = func_name};
    ir_add_instruction(program, call_inst);
}

// 生成say语句的IR
static void generate_say_ir(IRProgram *program, const char *message)
{
    // 保存原始消息
    const char *orig_message = message;

    // 在数据段定义字符串
    IRInstruction data_inst = {
        .type = IR_DATA,
        .operands = {{.type = OP_STRING, .str = orig_message}},
        .comment = "String constant"};
    ir_add_instruction(program, data_inst);

    // 字符串长度
    size_t len = strlen(orig_message);

    // 准备系统调用参数
    // 1. 字符串长度
    IRInstruction push_len = {
        .type = IR_PUSH,
        .operands = {{.type = OP_IMM, .imm = len}},
        .comment = "String length"};
    ir_add_instruction(program, push_len);

    // 2. 字符串地址 (将在汇编生成阶段解决)
    IRInstruction push_addr = {
        .type = IR_PUSH,
        .operands = {{.type = OP_STRING, .str = orig_message}}, // 特殊处理
        .comment = "String address"};
    ir_add_instruction(program, push_addr);

    // 3. 文件描述符 (stdout)
    IRInstruction push_fd = {
        .type = IR_PUSH,
        .operands = {{.type = OP_IMM, .imm = 1}},
        .comment = "File descriptor (stdout)"};
    ir_add_instruction(program, push_fd);

    // 设置系统调用号: write = 4
    IRInstruction mov_eax = {
        .type = IR_MOV,
        .operands = {{.type = OP_REG, .reg = REG_EAX}, {.type = OP_IMM, .imm = 4}}};
    ir_add_instruction(program, mov_eax);

    // 执行系统调用
    IRInstruction syscall = {
        .type = IR_SYSCALL,
        .comment = "System call: write"};
    ir_add_instruction(program, syscall);

    // 清理栈空间 (3个参数 * 4字节 = 12字节)
    IRInstruction clean_stack = {
        .type = IR_POP,
        .operands = {{.type = OP_IMM, .imm = 12}},
        .comment = "Clean up stack"};
    ir_add_instruction(program, clean_stack);
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

    // 收集函数定义
    for (int i = 0; i < count; i++)
    {
        if (nodes[i]->type == STMT_FUNCTION_DEF)
        {
            generate_function_ir(program, nodes[i]);
        }
    }

    // 添加主程序体
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

    // 确保程序结束
    IRInstruction syscall_exit = {
        .type = IR_SYSCALL,
        .operands = {{.type = OP_IMM, .imm = 1}},
        .comment = "Exit syscall"};
    ir_add_instruction(program, syscall_exit);

    return program;
}