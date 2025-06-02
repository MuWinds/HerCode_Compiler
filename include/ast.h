#ifndef AST_H
#define AST_H

typedef enum
{
    STMT_SAY,
    STMT_FUNCTION_DEF,  // 函数定义
    STMT_FUNCTION_CALL, // 函数调用
} NodeType;

typedef struct ASTNode
{
    NodeType type;
    char *value; // 对于函数，存储函数名

    // 函数定义的函数体
    struct ASTNode **body;
    int body_count;
} ASTNode;

ASTNode *create_say_node(char *str);
void free_node(ASTNode *node);
ASTNode *create_function_call_node(char *name);
ASTNode *create_function_def_node(char *name, ASTNode **body, int body_count);
ASTNode *create_block_node(ASTNode **nodes, int count);
#endif
