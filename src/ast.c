#include "ast.h"
#include <stdlib.h>
#include <string.h>

ASTNode *create_say_node(const char *str)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = STMT_SAY;
    node->value = _strdup(str);
    node->body = NULL;
    node->body_count = 0;
    node->c_code = NULL;
    return node;
}

ASTNode *create_c_block_node(const char *c_code)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = STMT_C_BLOCK;
    node->value = NULL;
    node->body = NULL;
    node->body_count = 0;
    node->c_code = _strdup(c_code ? c_code : "");
    return node;
}

ASTNode *create_function_def_node(const char *name, ASTNode **body, int body_count)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = STMT_FUNCTION_DEF;
    node->value = _strdup(name);
    node->body = malloc(sizeof(ASTNode *) * body_count);
    node->body_count = body_count;

    for (int i = 0; i < body_count; i++)
    {
        node->body[i] = body[i];
    }

    return node;
}

ASTNode *create_function_call_node(const char *func_name)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = STMT_FUNCTION_CALL;
    node->func_name = _strdup(func_name);
    node->value = _strdup(func_name); // 兼容 codegen 逻辑
    node->body = NULL;
    node->body_count = 0;
    node->c_code = NULL;
    return node;
}

void free_node(ASTNode *node)
{
    if (!node)
        return;
    if (node->func_name)
        free(node->func_name);
    if (node->body)
    {
        for (int i = 0; i < node->body_count; ++i)
            free_node(node->body[i]);
        free(node->body);
    }
    if (node->value)
        free(node->value);
    if (node->c_code)
        free(node->c_code);
    free(node);
}

ASTNode *create_block_node(ASTNode **nodes, int count)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
        return NULL;

    node->type = STMT_C_BLOCK; // 或者根据实际需要设置类型
    node->value = NULL;
    node->body = calloc(count, sizeof(ASTNode *));
    if (!node->body)
    {
        free(node);
        return NULL;
    }

    node->body_count = count;
    for (int i = 0; i < count; i++)
    {
        node->body[i] = nodes[i];
    }

    node->c_code = NULL;
    return node;
}