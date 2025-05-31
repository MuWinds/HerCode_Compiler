#ifndef IR_H
#define IR_H
#include <stdint.h>
#include "ast.h"
// 寄存器常量
typedef enum
{
    REG_EAX,
    REG_EBX,
    REG_ECX,
    REG_EDX,
    REG_ESI,
    REG_EDI,
    REG_ESP,
    REG_EBP
} RegisterID;
// IR 指令类型
typedef enum
{
    IR_MOV,     // 数据传输
    IR_CALL,    // 函数调用
    IR_RET,     // 函数返回
    IR_PUSH,    // 压栈
    IR_POP,     // 出栈
    IR_SYSCALL, // 系统调用
    IR_LABEL,   // 标签
    IR_JMP,     // 无条件跳转
    IR_DATA     // 数据定义
} IRInstructionType;

// 操作数类型
typedef enum
{
    OP_REG,
    OP_IMM,   // 立即数
    OP_MEM,   // 内存地址
    OP_LABEL, // 标签地址
    OP_STRING // 字符串引用
} OperandType;

// 操作数结构
typedef struct
{
    OperandType type;
    union
    {
        uint32_t reg;      // 寄存器编号
        int32_t imm;       // 立即数
        uint32_t mem_addr; // 内存地址
        const char *label; // 标签名
        const char *str;   // 字符串内容
    };
} Operand;

// IR 指令结构
typedef struct
{
    IRInstructionType type;
    Operand operands[3]; // 最多三个操作数
    const char *comment; // 调试信息
} IRInstruction;

// IR 程序表示
typedef struct
{
    IRInstruction *instructions;
    int count;
    int capacity;
} IRProgram;

// IR API
IRProgram *create_ir_program();
IRProgram *ast_to_ir(ASTNode **nodes, int count);
void ir_add_instruction(IRProgram *program, IRInstruction inst);
void free_ir_program(IRProgram *program);
#endif