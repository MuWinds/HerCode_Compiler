#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
        free_token(parser->current_token);
        parser->current_token = next_token(parser->lexer);
    }
    else
    {
        printf("Syntax error: Expected token type %d (%s), but got token type %d (%s)\n",
               type, token_type_to_string(type), parser->current_token->type, token_type_to_string(parser->current_token->type));
        exit(1);
    }
}
ASTNode *parse_statement(Parser *parser)
{
    // 跳过无关token
    while (parser->current_token->type == TOKEN_DEDENT ||
           parser->current_token->type == TOKEN_NEWLINE ||
           parser->current_token->type == TOKEN_INDENT)
    {

        // 更新缩进状态
        if (parser->current_token->type == TOKEN_INDENT)
        {
            parser->current_indent++;
        }
        else if (parser->current_token->type == TOKEN_DEDENT)
        {
            parser->current_indent--;
        }

        eat(parser, parser->current_token->type);
    }

    // 打印调试信息
    printf("[PARSER] parse_statement token: %s (%d)\n",
           token_type_to_string(parser->current_token->type),
           parser->current_token->type);

    // 识别不同语句类型
    switch (parser->current_token->type)
    {
    case TOKEN_SAY:
        return parse_say_statement(parser);
    case TOKEN_FUNCTION:
        return parse_function_definition(parser);
    case TOKEN_IDENTIFIER:
        return parse_function_call(parser);
    }

    // 未知语句类型
    fprintf(stderr, "Syntax error: Unknown statement. Got token %d (%s)\n",
            parser->current_token->type,
            token_type_to_string(parser->current_token->type));
    exit(1);
}

ASTNode *parse_say_statement(Parser *parser)
{
    eat(parser, TOKEN_SAY); // 消耗'say' token

    // 确保下一个token是字符串
    if (parser->current_token->type != TOKEN_STRING)
    {
        fprintf(stderr, "Syntax error: Expected string after 'say'\n");
        exit(1);
    }

    char *str_value = parser->current_token->value ? strdup(parser->current_token->value) : strdup("");

    eat(parser, TOKEN_STRING); // 消耗字符串token

    return create_say_node(str_value);
}

ASTNode *parse_function_definition(Parser *parser)
{
    printf("[PARSER] Parsing function definition\n");

    // 消耗 function 关键字
    eat(parser, TOKEN_FUNCTION);

    // 检查函数名
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Syntax error: Expected function name after 'function'. Got token %d (%s)\n",
                parser->current_token->type,
                token_type_to_string(parser->current_token->type));
        exit(1);
    }
    char *func_name = strdup(parser->current_token->value);
    eat(parser, TOKEN_IDENTIFIER);
    printf("  Function name: '%s'\n", func_name);

    // 检查冒号
    if (parser->current_token->type != TOKEN_COLON)
    {
        fprintf(stderr, "Syntax error: Expected colon after function name. Got token %d (%s)\n",
                parser->current_token->type,
                token_type_to_string(parser->current_token->type));
        exit(1);
    }
    eat(parser, TOKEN_COLON);

    // 处理函数定义后的可能空白
    int first_token = 1;

    // 解析函数体
    ASTNode **body = NULL;
    int body_capacity = 8; // 初始容量
    int body_count = 0;
    parser->current_indent = -1; // 标记函数体缩进级别未设置

    // 初始化body数组
    body = malloc(body_capacity * sizeof(ASTNode *));
    if (!body)
    {
        fprintf(stderr, "Memory allocation failed for function body\n");
        exit(1);
    }

    // 直到遇到end或DEDENT
    while (1)
    {
        // 处理空白token
        while (parser->current_token->type == TOKEN_NEWLINE ||
               parser->current_token->type == TOKEN_INDENT ||
               parser->current_token->type == TOKEN_DEDENT)
        {
            // 第一次遇到缩进，设置当前缩进级别
            if (parser->current_token->type == TOKEN_INDENT &&
                parser->current_indent == -1)
            {
                parser->current_indent = parser->lexer->indent_stack[parser->lexer->indent_top];
                printf("  Function body indent set to: %d\n", parser->current_indent);
            }

            eat(parser, parser->current_token->type);
        }

        // 检查结束条件
        if (parser->current_token->type == TOKEN_END)
        {
            break;
        }

        // 如果遇到DEDENT，检查是否已经返回到函数定义层级
        if (parser->current_token->type == TOKEN_DEDENT &&
            parser->current_indent != -1 &&
            parser->lexer->indent_stack[parser->lexer->indent_top] < parser->current_indent)
        {
            printf("  Exiting function body at indent: %d (current: %d)\n",
                   parser->current_indent, parser->lexer->indent_stack[parser->lexer->indent_top]);
            break;
        }

        // 遇到函数体中的语句
        printf("  Parsing function body statement (%s)\n", token_type_to_string(parser->current_token->type));

        // 检查并扩展数组容量
        if (body_count >= body_capacity)
        {
            body_capacity *= 2;
            ASTNode **new_body = realloc(body, body_capacity * sizeof(ASTNode *));
            if (!new_body)
            {
                fprintf(stderr, "Memory reallocation failed for function body\n");
                exit(1);
            }
            body = new_body;
        }

        // 解析语句并存储
        ASTNode *stmt = parse_statement(parser);
        body[body_count++] = stmt;
    }

    // 消耗end关键字
    if (parser->current_token->type == TOKEN_END)
    {
        eat(parser, TOKEN_END);
    }
    else
    {
        fprintf(stderr, "Syntax error: Expected 'end' to close function definition. Got %d (%s)\n",
                parser->current_token->type,
                token_type_to_string(parser->current_token->type));
        exit(1);
    }

    // 重置缩进级别
    parser->current_indent = 0;
    printf("Successfully parsed function '%s' with %d statements\n", func_name, body_count);

    ASTNode *result = create_function_def_node(func_name, body, body_count);
    if (!result)
    {
        free(body);
        return NULL;
    }
    return result;
}

ASTNode *parse_function_call(Parser *parser)
{
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Syntax error: Expected function name\n");
        exit(1);
    }

    char *func_name = strdup(parser->current_token->value);
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
        // 处理行内Token
        while (parser->current_token->type == TOKEN_NEWLINE ||
               parser->current_token->type == TOKEN_INDENT)
        { // 添加对缩进Token的处理
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
                fprintf(stderr, "Memory reallocation failed for block nodes\n");
                exit(1);
            }
            nodes = new_nodes;
        }
        nodes[*count] = parse_statement(parser);
        (*count)++;
    }
    ASTNode *block = create_block_node(nodes, *count);
    if (!block)
    {
        for (int i = 0; i < *count; i++)
        {
            free_node(nodes[i]);
        }
        free(nodes);
        return NULL;
    }
    free(nodes);
    return block;
}

ASTNode **parse_program(Parser *parser, int *count)
{
    *count = 0;
    ASTNode **nodes = NULL;
    int nodes_capacity = 0;

    // 允许函数定义出现在程序开头
    while (parser->current_token->type != TOKEN_EOF)
    {
        // 跳过缩进和换行符
        while (parser->current_token->type == TOKEN_NEWLINE ||
               parser->current_token->type == TOKEN_INDENT ||
               parser->current_token->type == TOKEN_DEDENT)
        {
            eat(parser, parser->current_token->type);
        }

        // 检查是否达到文件末尾
        if (parser->current_token->type == TOKEN_EOF)
        {
            break;
        }

        // 检查是否遇到start关键字
        if (parser->current_token->type == TOKEN_START)
        {
            break;
        }

        // 解析函数定义
        if (*count >= nodes_capacity)
        {
            nodes_capacity = nodes_capacity ? nodes_capacity * 2 : 8;
            ASTNode **new_nodes = realloc(nodes, nodes_capacity * sizeof(ASTNode *));
            if (!new_nodes)
            {
                fprintf(stderr, "Memory reallocation failed for program nodes\n");
                exit(1);
            }
            nodes = new_nodes;
        }

        // 解析其他语句（包括函数定义）
        ASTNode *node = parse_statement(parser);
        if (node)
        {
            nodes[(*count)++] = node;
        }
    }

    // 程序必须以start开始
    if (parser->current_token->type != TOKEN_START)
    {
        fprintf(stderr, "Syntax error: Program must contain 'start:' block\n");
        exit(1);
    }
    eat(parser, TOKEN_START); // 消耗start token

    // 处理可选的换行符
    while (parser->current_token->type == TOKEN_NEWLINE)
    {
        eat(parser, TOKEN_NEWLINE);
    }

    // 必须有缩进
    if (parser->current_token->type != TOKEN_INDENT)
    {
        fprintf(stderr, "Syntax error: Expected indentation after 'start:'\n");
        exit(1);
    }
    eat(parser, TOKEN_INDENT);
    parser->current_indent++;

    // 解析程序主体
    while (parser->current_token->type != TOKEN_EOF)
    {
        // 处理缩出（从当前缩进级别退出）
        if (parser->current_token->type == TOKEN_DEDENT)
        {
            eat(parser, TOKEN_DEDENT);
            parser->current_indent--;

            // 当缩进级别回到0时，准备退出程序块
            if (parser->current_indent == 0)
            {
                break;
            }
            continue;
        }

        // 跳过换行符
        if (parser->current_token->type == TOKEN_NEWLINE)
        {
            eat(parser, TOKEN_NEWLINE);
            continue;
        }

        // 处理end关键字（提前退出）
        if (parser->current_token->type == TOKEN_END)
        {
            break;
        }

        // 确保不超过最大语句数
        if (*count >= nodes_capacity)
        {
            nodes_capacity = nodes_capacity ? nodes_capacity * 2 : 8;
            nodes = realloc(nodes, nodes_capacity * sizeof(ASTNode *));
        }

        // 解析语句
        nodes[(*count)++] = parse_statement(parser);
    }

    // 在缩出循环后，跳过所有换行符和DEDENT
    while (parser->current_token->type == TOKEN_NEWLINE ||
           parser->current_token->type == TOKEN_DEDENT)
    {
        eat(parser, parser->current_token->type);
    }

    // 处理end关键字
    if (parser->current_token->type == TOKEN_EOF)
    {
        fprintf(stderr, "Syntax error: Program must end with 'end'\n");
        exit(1);
    }

    if (parser->current_token->type != TOKEN_END)
    {
        fprintf(stderr, "Syntax error: Expected 'end' at end of program. Got token type %d (%s)\n",
                parser->current_token->type,
                token_type_to_string(parser->current_token->type));
        exit(1);
    }
    eat(parser, TOKEN_END);

    // 确保缩进级别回到0
    if (parser->current_indent != 0)
    {
        // 处理剩余的缩出标记
        while (parser->current_token->type == TOKEN_DEDENT)
        {
            eat(parser, TOKEN_DEDENT);
            parser->current_indent--;
        }

        // 如果还有剩余的缩进级别
        if (parser->current_indent != 0)
        {
            fprintf(stderr, "Syntax error: Missing dedent at end of program (indent level=%d)\n",
                    parser->current_indent);
            exit(1);
        }
    }

    return nodes;
}
