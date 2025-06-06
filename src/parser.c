#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_UNKNOWN:
        return "UNKNOWN";
    case TOKEN_SAY:
        return "SAY";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_SEMI:
        return "SEMI";
    case TOKEN_END:
        return "END";
    case TOKEN_NEWLINE:
        return "NEWLINE";
    case TOKEN_INDENT:
        return "INDENT";
    case TOKEN_DEDENT:
        return "DEDENT";
    case TOKEN_FUNCTION:
        return "FUNCTION";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_COLON:
        return "COLON";
    case TOKEN_C_BLOCK:
        return "C_BLOCK";
    case TOKEN_START:
        return "START";
    default:
        return "UNRECOGNIZED";
    }
}

Parser *new_parser(Lexer *lexer)
{
    Parser *parser = malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = next_token(lexer);
    parser->current_indent = 0; // 初始缩进深度为0
    return parser;
}

void free_parser(Parser *parser)
{
    free_token(parser->current_token);
    free(parser->lexer);
    free(parser);
}

void free_token(Token *token)
{
    if (token)
    {
        free(token->value);
        free(token);
    }
}

void eat(Parser *parser, TokenType type)
{
    if (parser->current_token->type == type)
    {
        if (type == TOKEN_START) {
            int pos = parser->lexer->pos;
            size_t src_len = strlen(parser->lexer->source);
            int left = pos - 10; if (left < 0) left = 0;
            int right = pos + 10; if ((size_t)right > src_len) right = (int)src_len;
            char buf[64] = {0};
            size_t copy_len = right - left;
            if (copy_len >= sizeof(buf)) copy_len = sizeof(buf) - 1;
            strncpy(buf, parser->lexer->source + left, copy_len);
            buf[copy_len] = '\0';
            printf("[PARSER][EAT-BEFORE] TOKEN_START: lexer->pos=%d, src[周围]=\"%s\", token_type=%d (%s), value='%s'\n", pos, buf, parser->current_token->type, token_type_to_string(parser->current_token->type), parser->current_token->value ? parser->current_token->value : "NULL");
        }
        free_token(parser->current_token);
        parser->current_token = next_token(parser->lexer);
        if (type == TOKEN_START) {
            int pos = parser->lexer->pos;
            size_t src_len = strlen(parser->lexer->source);
            int left = pos - 10; if (left < 0) left = 0;
            int right = pos + 10; if ((size_t)right > src_len) right = (int)src_len;
            char buf[64] = {0};
            size_t copy_len = right - left;
            if (copy_len >= sizeof(buf)) copy_len = sizeof(buf) - 1;
            strncpy(buf, parser->lexer->source + left, copy_len);
            buf[copy_len] = '\0';
            printf("[PARSER][EAT-AFTER] TOKEN_START: lexer->pos=%d, src[周围]=\"%s\", token_type=%d (%s), value='%s'\n", pos, buf, parser->current_token->type, token_type_to_string(parser->current_token->type), parser->current_token->value ? parser->current_token->value : "NULL");
        }
    }
    else
    {
        printf("[PARSER] 语法错误：期望 token type %d (%s)，但实际 token type %d (%s)\n",
               type, token_type_to_string(type), parser->current_token->type, token_type_to_string(parser->current_token->type));
        fprintf(stderr, "[PARSER ERROR] Fatal error encountered, returning NULL而不中断主流程\n");
        return;
    }
}

// 前置声明
ASTNode *parse_c_block_statement(Parser *parser);

// =====================
// 解析单条 HerCode 语句
// =====================
ASTNode *parse_statement(Parser *parser)
{
    // 调用者（例如 parse_function_definition 或 parse_program）已跳过了前导的换行符（解析器定位在潜在语句的开始位置）
    // parser->current_indent 为这个语句块预期的缩进级别
    // *** 实际的当前缩进在 parser->lexer->indent_stack[parser->lexer->indent_top] 中

    printf("[PARSER][parse_statement] token=%d (%s), expected_indent=%d, lexer_indent=%d\n",
           parser->current_token->type, token_type_to_string(parser->current_token->type),
           parser->current_indent, parser->lexer->indent_stack[parser->lexer->indent_top]);

    // 验证当前符号是否处于当前块的预期缩进级别
    // 这个检查可能过于简单，具体取决于词法分析器如何处理制表符/空格以及混合缩进
    // 目前假设词法分析器提供一致的 indent_level
    // if (parser->lexer->indent_stack[parser->lexer->indent_top] != parser->current_indent) {
    //     fprintf(stderr, "语法错误：语句位于意外的缩进级别。预期缩进 %d，实际缩进 %d，符号为 %s\n",
    //             parser->current_indent, parser->lexer->indent_stack[parser->lexer->indent_top],
    //             token_type_to_string(parser->current_token->type));
    //     // 不要消耗符号，让调用者处理这个结构性错误
    //     return NULL;
    // }

    switch (parser->current_token->type)
    {
    case TOKEN_SAY:
        return parse_say_statement(parser);
    case TOKEN_C_BLOCK:
        return parse_c_block_statement(parser);
    case TOKEN_IDENTIFIER: // 可能是一个函数调用语句
        return parse_function_call(parser);
    // FUNCTION、START、END、DEDENT、EOF、INDENT、NEWLINE等结构性符号
    // 应该在调用 parse_statement 之前由块解析逻辑（例如在 parse_function_definition 或 parse_program 中）处理
    default:
        fprintf(stderr, "[PARSER][parse_statement] 语法错误：在期望语句的地方遇到意外的标记 %s (type %d)。\n",
               token_type_to_string(parser->current_token->type), parser->current_token->type);

        // 保留原始行为，对于未知的语句起始，消耗掉当前符号并返回 NULL
        eat(parser, parser->current_token->type);
        return NULL;
    }
}

// 解析__c__ { ... } 语法块
ASTNode *parse_c_block_statement(Parser *parser)
{
    // 直接处理 TOKEN_C_BLOCK，无需检查 '{'
    ASTNode *node = create_c_block_node(parser->current_token->value);
    eat(parser, TOKEN_C_BLOCK);
    return node;
}

ASTNode *parse_say_statement(Parser *parser)
{
    eat(parser, TOKEN_SAY); // 消耗 'say' token

    // 确保下一个 token 是字符串
    if (parser->current_token->type != TOKEN_STRING)
    {
        fprintf(stderr, "[PARSER] 语法错误：'say' 后面应该是字符串\n");
        fprintf(stderr, "[PARSER ERROR] 遇到致命错误，返回 NULL 而不中断主流程\n");
        printf("[PARSER][parse_program] 返回了 NULL 在 %s:%d (未找到 start: 块)\n", __FILE__, __LINE__);
        return NULL;
    }

    char *str_value = parser->current_token->value ? _strdup(parser->current_token->value) : _strdup("");

    eat(parser, TOKEN_STRING); // 消耗掉字符串 token

    return create_say_node(str_value);
}

ASTNode *parse_function_definition(Parser *parser)
{
    printf("[PARSER] 解析函数定义\n");

    eat(parser, TOKEN_FUNCTION);

    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "[PARSER] 语法错误：'function' 后面应该是函数名。实际上是 token %d (%s)\n",
                parser->current_token->type, token_type_to_string(parser->current_token->type));
        return NULL;
    }
    char *func_name = _strdup(parser->current_token->value);
    if (!func_name) { // 检查 strdup 的结果
        fprintf(stderr, "[PARSER] 内存分配失败，无法存储函数名\n");
        return NULL;
    }
    eat(parser, TOKEN_IDENTIFIER);
    printf("[PARSER] 函数名：'%s'\n", func_name);

    if (parser->current_token->type != TOKEN_COLON)
    {
        fprintf(stderr, "[PARSER] 语法错误：函数名 '%s' 后面应该是冒号。实际上是 token %d (%s)\n",
                func_name, parser->current_token->type, token_type_to_string(parser->current_token->type));
        free(func_name); // 释放分配的名称内存
        return NULL;
    }
    eat(parser, TOKEN_COLON);

    // 跳过任何紧跟冒号后的换行符
    while (parser->current_token->type == TOKEN_NEWLINE)
    {
        eat(parser, TOKEN_NEWLINE);
    }

    int function_body_indent_level = parser->current_indent; // 缩进级别在函数体开始之前

    // 期望函数体进行缩进，除非紧跟着的是下一个块
    if (parser->current_token->type == TOKEN_INDENT)
    {
        eat(parser, TOKEN_INDENT);
        function_body_indent_level = parser->lexer->indent_stack[parser->lexer->indent_top];
        parser->current_indent = function_body_indent_level; // 设置函数体的当前预期缩进
        printf("[PARSER] 函数 '%s' 体的缩进级别设为：%d\n", func_name, function_body_indent_level);
    }
    else
    {
        // 未找到 INDENT（可能是一个空函数或语法错误）
        // *** 如果是 EOF, END, DEDENT, FUNCTION, 或 START，视为空函数
        if (parser->current_token->type == TOKEN_EOF || parser->current_token->type == TOKEN_END ||
            parser->current_token->type == TOKEN_DEDENT || parser->current_token->type == TOKEN_FUNCTION ||
            parser->current_token->type == TOKEN_START)
        {
            printf("[PARSER] 函数 '%s' 为空（无缩进，后面跟着 %s）\n", func_name, token_type_to_string(parser->current_token->type));
            ASTNode *func_node = create_function_def_node(func_name, NULL, 0);
            // func_name 和 body_statements 归属都转到 func_node
            return func_node;
        }
        else
        {
            fprintf(stderr, "[PARSER] 语法错误：函数 '%s' 的函数体应该缩进或有效结束。实际上是 %s\n",
                    func_name, token_type_to_string(parser->current_token->type));
            free(func_name);
            return NULL;
        }
    }

    ASTNode **body_statements = NULL;
    int body_capacity = 8;
    int body_statement_count = 0;
    body_statements = malloc(body_capacity * sizeof(ASTNode *));
    if (!body_statements)
    {
        fprintf(stderr, "[PARSER] 内存分配失败，无法存储函数体语句 '%s'\n", func_name);
        free(func_name);
        return NULL;
    }

    while (1)
    {
        // 跳过函数体内的前导换行符
        while (parser->current_token->type == TOKEN_NEWLINE)
        {
            eat(parser, TOKEN_NEWLINE);
        }

        TokenType current_token_type = parser->current_token->type;

        // 检查函数体结束的条件
        if (current_token_type == TOKEN_EOF ||     // 结束
            current_token_type == TOKEN_FUNCTION || // 新函数的开始
            current_token_type == TOKEN_START)      // 主 'start:' 块的开始
        {
            printf("[PARSER] 函数 '%s' 体隐式结束，由于 %s (token 未被消耗)\n", func_name, token_type_to_string(current_token_type));
            break; // 不消耗这些符号，交由父级解析器 (parse_program) 处理
        }

        if (current_token_type == TOKEN_END) // 函数的显式 end 关键字
        {
            printf("[PARSER] 函数 '%s' 体显式结束 (消耗 END token)\n", func_name);
            eat(parser, TOKEN_END);
            break;
        }

        // 检查缩进是否减少（函数体结束）
        if (current_token_type == TOKEN_DEDENT)
        {
            // 词法分析器的当前 indent_stack 顶部为 DEDENT 处理后的缩进级别
            if (parser->lexer->indent_stack[parser->lexer->indent_top] < function_body_indent_level)
            {
                printf("[PARSER] 函数 '%s' 体隐式结束，由于 DEDENT (body_indent: %d, new_lexer_indent: %d)。消耗 DEDENT。\n",
                       func_name, function_body_indent_level, parser->lexer->indent_stack[parser->lexer->indent_top]);
                eat(parser, TOKEN_DEDENT);
                parser->current_indent = parser->lexer->indent_stack[parser->lexer->indent_top]; // 更新解析器对当前缩进的记录
                break;
            }
            else
            {
                // 这个 DEDENT 不会更改函数体的已建立的缩进级别
                // 如果在函数体之后没有 DEDENT（直接是 EOF 或 END），并且缩进级别未恢复到函数前的级别（错误）
                // 消耗掉并让语句解析逻辑处理或报错
                printf("[PARSER] 函数 '%s' 体内遇到 DEDENT，但新的缩进级别 (%d) 不小于函数体缩进级别 (%d)。消耗 DEDENT。\n",
                       func_name, parser->lexer->indent_stack[parser->lexer->indent_top], function_body_indent_level);
                eat(parser, TOKEN_DEDENT);
                parser->current_indent = parser->lexer->indent_stack[parser->lexer->indent_top];
                // 在这个新的（但仍大于等于函数体缩进级别）缩进处继续解析语句
            }
        }
        
        // 如果当前符号的有效缩进（来自词法分析器）小于 function_body_indent_level，则表示隐式结束
        // 针对那些本身不是 DEDENT 符号但缩进较浅的行
        if (parser->lexer->indent_stack[parser->lexer->indent_top] < function_body_indent_level) {
             printf("[PARSER] 函数 '%s' 体隐式结束，由于缩进级别减少 (body_indent: %d, current_line_indent: %d) 为 token %s (未被消耗)\n",
                   func_name, function_body_indent_level, parser->lexer->indent_stack[parser->lexer->indent_top], token_type_to_string(current_token_type));
            break; // 不要消耗这个符号，交由父级处理
        }

        // 如果发生错误，导致错误的符号可能已经被 parse_statement 或 eat 消耗掉了
        if (body_statement_count >= body_capacity)
        {
            body_capacity *= 2;
            ASTNode **new_body = realloc(body_statements, body_capacity * sizeof(ASTNode *));
            if (!new_body)
            {
                fprintf(stderr, "[PARSER] 内存重新分配失败，无法存储函数体语句 '%s'\n", func_name);
                for (int i = 0; i < body_statement_count; i++) free_node(body_statements[i]);
                free(body_statements);
                free(func_name);
                return NULL;
            }
            body_statements = new_body;
        }

        // 对于直接在函数体中的语句，parser->current_indent 是函数体的缩进级别
        parser->current_indent = function_body_indent_level; 
        ASTNode *stmt = parse_statement(parser);
        if (!stmt)
        {
            fprintf(stderr, "[PARSER] 错误解析函数 '%s' 中的语句 (当前 token: %s)。终止函数解析。\n",
                    func_name, token_type_to_string(parser->current_token->type));
            for (int i = 0; i < body_statement_count; i++) free_node(body_statements[i]);
            free(body_statements);
            free(func_name);
            return NULL;
        }
        body_statements[body_statement_count++] = stmt;
    }

    // 循环结束后，parser->current_indent 为此函数定义之后活动的缩进级别
    // 如果循环因 DEDENT 停止（已经被更新）。否则，这将是词法分析器当前栈顶的值
    parser->current_indent = parser->lexer->indent_stack[parser->lexer->indent_top];
    printf("[PARSER] 函数 '%s' 解析完成。最终解析器缩进级别：%d\n", func_name, parser->current_indent);

    ASTNode *func_node = create_function_def_node(func_name, body_statements, body_statement_count);
    if (!func_node) { 
        fprintf(stderr, "[PARSER] 创建函数节点 '%s' 失败\n", func_name);
        for (int i = 0; i < body_statement_count; i++) free_node(body_statements[i]);
        free(body_statements);
        free(func_name);
        return NULL;
    }

    printf("[PARSER] 成功解析函数 '%s'，包含 %d 语句\n", func_name, body_statement_count);
    return func_node;
}

ASTNode *parse_function_call(Parser *parser)
{
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "[PARSER] 语法错误：期望函数名\n");
        fprintf(stderr, "[PARSER ERROR] 遇到致命错误，返回 NULL 而不中断主流程\n");
        printf("[PARSER][parse_program] 返回 NULL at %s:%d (未找到 start: 块)\n", __FILE__, __LINE__);
        return NULL;
    }

    char *func_name = _strdup(parser->current_token->value);
    eat(parser, TOKEN_IDENTIFIER);

    return create_function_call_node(func_name);
}

ASTNode *parse_block(Parser *parser, int *count)
{
    *count = 0;
    ASTNode **nodes = NULL;
    int nodes_capacity = 0;

    while (1)
    {
        // 终极死循环保护：块体内遇到 TOKEN_START 直接 eat 并 continue
        if (parser->current_token->type == TOKEN_START) {
            printf("[parse_block] 主循环头遇到 TOKEN_START，主动 eat 并 continue\n");
            eat(parser, TOKEN_START);
            continue;
        }
        // 处理行内 Token
        while (parser->current_token->type == TOKEN_NEWLINE ||
               parser->current_token->type == TOKEN_INDENT)
        {
            // 添加对缩进 Token 的处理
            eat(parser, parser->current_token->type);
        }

        // 块结束检查
        if (parser->current_token->type == TOKEN_DEDENT ||
            parser->current_token->type == TOKEN_END)
        {
            break;
        }

        if (*count >= nodes_capacity)
        {
            nodes_capacity = nodes_capacity ? nodes_capacity * 2 : 8;
            ASTNode **new_nodes = realloc(nodes, nodes_capacity * sizeof(ASTNode *));
            if (!new_nodes)
            {
                fprintf(stderr, "[PARSER] 内存重新分配失败，无法存储块节点\n");
                fprintf(stderr, "[PARSER ERROR] 遇到致命错误，返回 NULL 而不中断主流程\n");
                printf("[PARSER][parse_program] 返回 NULL 在 %s:%d (未找到 start: 块)\n", __FILE__, __LINE__);
                return NULL;
            }
            nodes = new_nodes;
        }
        ASTNode *stmt = parse_statement(parser);
        if (!stmt) {
            printf("[PARSER][parse_block] parse_statement 返回 NULL，强制 eat 当前 token type=%d (%s) 并 continue\n",
                   parser->current_token->type, token_type_to_string(parser->current_token->type));
            eat(parser, parser->current_token->type);
            continue;
        }
        nodes[*count] = stmt;
        (*count)++;


    }
    // 假设block节点类型为STMT_SAY
    ASTNode *block = create_block_node(nodes, *count);
    if (!block)
    {
        for (int i = 0; i < *count; i++)
        {
            free_node(nodes[i]);
        }
        free(nodes);
        printf("[PARSER][parse_program] 返回 NULL 在 %s:%d (未找到start:块)\n", __FILE__, __LINE__);
        return NULL;
    }
    free(nodes);
    return block;
}

// =====================
// 解析整个 HerCode 程序
// =====================
ASTNode **parse_program(Parser *parser, int *count)
{
    printf("[DEBUG] 进入 parse_program\n");
    *count = 0;
    ASTNode **nodes = NULL;
    int nodes_capacity = 0;

    // 跳过头部的无关符号
    while (parser->current_token->type == TOKEN_NEWLINE ||
           parser->current_token->type == TOKEN_INDENT ||
           parser->current_token->type == TOKEN_DEDENT)
    {
        eat(parser, parser->current_token->type);
    }

    // 允许全局函数定义
    while (parser->current_token->type == TOKEN_FUNCTION)
    {
        ASTNode *func = parse_function_definition(parser);
        if (func)
        {
            if (*count >= nodes_capacity)
            {
                nodes_capacity = nodes_capacity ? nodes_capacity * 2 : 8;
                nodes = realloc(nodes, nodes_capacity * sizeof(ASTNode *));
            }
            nodes[(*count)++] = func;
        }
        // 函数定义之后，跳过所有尾随的 END 符号和换行符
        // 然后再查找下一个 FUNCTION 符号或 START 符号
        while (parser->current_token->type == TOKEN_NEWLINE ||
               parser->current_token->type == TOKEN_END)
        {
            printf("[PARSER][parse_program] 跳过 top-level %s 在函数定义后\n",
                   token_type_to_string(parser->current_token->type));
            eat(parser, parser->current_token->type);
        }
    }

    // 在所有函数定义解析完毕后（或没有函数定义）
    // 在期望的 START 符号之前，跳过所有残留的换行符或 END 符号
    while (parser->current_token->type == TOKEN_NEWLINE ||
           parser->current_token->type == TOKEN_END)
    {
        printf("[PARSER][parse_program] 跳过 top-level %s 在 START block 检查前\n",
               token_type_to_string(parser->current_token->type));
        eat(parser, parser->current_token->type);
    }

    // 只允许一个 start:
    if (parser->current_token->type != TOKEN_START)
    {
        fprintf(stderr, "[PARSER] 语法错误：程序必须包含 'start:' 块\n");
        return NULL;
    }
    eat(parser, TOKEN_START);
    while (parser->current_token->type == TOKEN_NEWLINE)
        eat(parser, TOKEN_NEWLINE);
    if (parser->current_token->type != TOKEN_INDENT)
    {
        fprintf(stderr, "[PARSER] 语法错误：期望 'start:' 后面有缩进\n");
        return NULL;
    }
    eat(parser, TOKEN_INDENT);
    parser->current_indent++;

    // 主程序块
    while (parser->current_token->type != TOKEN_DEDENT &&
           parser->current_token->type != TOKEN_END &&
           parser->current_token->type != TOKEN_EOF)
    {
        // 跳过冗余的 start: ，防止无限循环
        if (parser->current_token->type == TOKEN_START)
        {
            printf("[PARSER][parse_program] 主循环遇到多余TOKEN_START，主动eat并continue\n");
            eat(parser, TOKEN_START);
            continue;
        }
        ASTNode *stmt = parse_statement(parser);
        if (stmt)
        {
            if (*count >= nodes_capacity)
            {
                nodes_capacity = nodes_capacity ? nodes_capacity * 2 : 8;
                nodes = realloc(nodes, nodes_capacity * sizeof(ASTNode *));
            }
            nodes[(*count)++] = stmt;
        }
        // parse_statement 返回 NULL 说明遇到无效符号，已被自动消耗，无需再次消耗
    }

    // 退出主程序块
    if (parser->current_token->type == TOKEN_DEDENT)
    {
        eat(parser, TOKEN_DEDENT);
        parser->current_indent--;
    }

    // 跳过冗余的NEWLINE/DEDENT
    while (parser->current_token->type == TOKEN_NEWLINE ||
           parser->current_token->type == TOKEN_DEDENT)
    {
        eat(parser, parser->current_token->type);
    }

    // 处理end符号
    if (parser->current_token->type == TOKEN_END)
        eat(parser, TOKEN_END);

    // 结尾缩进检查
    if (parser->current_indent != 0)
    {
        fprintf(stderr, "[PARSER] 语法错误：程序结尾缩进级别不正确 (indent level=%d)\n", parser->current_indent);
    }

    printf("[PARSER][RETURN] node_count=%d\n", *count);
    for (int i = 0; i < *count; i++) {
        printf("[PARSER][RETURN] 节点[%d] 类型=%d\n", i, nodes[i] ? nodes[i]->type : -1);
    }
    return nodes;
}
