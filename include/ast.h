#ifndef AST_H
#define AST_H

typedef enum
{
    STMT_SAY,
    STMT_FUNCTION_DEF,  // 函数定义
    STMT_FUNCTION_CALL, // 函数调用
    STMT_C_BLOCK,       // 内嵌C代码块
    AST_C_BLOCK         // C代码块
} NodeType;

typedef struct ASTNode
{
    NodeType type;
    char *value; 
    char *func_name;
    struct ASTNode **body;
    int body_count;
    char *c_code; // 内联C代码
} ASTNode;

ASTNode *create_say_node(const char *value);
ASTNode *create_c_block_node(const char *c_code);
void free_node(ASTNode *node);
ASTNode *create_function_call_node(const char *func_name);
ASTNode *create_function_def_node(const char *func_name, ASTNode **body, int body_count);
ASTNode *create_block_node(ASTNode **stmts, int count);
#endif
