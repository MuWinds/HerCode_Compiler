#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Lexer *new_lexer(char *source)
{
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->pos = 0;
    lexer->current_char = source[0];
    lexer->current_indent = 0;
    lexer->indent_stack[0] = 0; // 初始化缩进栈（第0级=0）
    lexer->indent_top = 0;
    lexer->pending_dedents = 0;
    return lexer;
}

void advance(Lexer *lexer)
{
    lexer->pos++;
    lexer->current_char = lexer->source[lexer->pos];
}

Token *new_token(TokenType type, const char *value)
{
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->value = value ? _strdup(value) : NULL; // 允许NULL值
    printf("[LEXER] 新建token: 类型=%d, 值=\"%s\"", type, value ? value : "NULL");
    return token;
}

Token *next_token(Lexer *lexer)
{
    printf("[LEXER] 当前字符: %c, 位置: %d\n", lexer->current_char, lexer->pos);

    // 处理待生成的DEDENT
    if (lexer->pending_dedents > 0)
    {
        lexer->pending_dedents--;
        printf("[LEXER] 生成待处理的DEDENT (%d个)\n", lexer->pending_dedents);
        return new_token(TOKEN_DEDENT, NULL);
    }

    // 处理文件结束情况
    if (lexer->current_char == '\0')
    {
        // 文件结束时处理剩余缩进
        if (lexer->indent_top > 0)
        {
            printf("[LEXER] 文件结束，生成剩余缩进的DEDENT (%d个)\n", lexer->indent_top);
            lexer->indent_top--;
            lexer->pending_dedents = lexer->indent_top;
            return new_token(TOKEN_DEDENT, NULL);
        }
        printf("[LEXER] 文件结束，返回EOF token\n");
        return new_token(TOKEN_EOF, NULL);
    }

    while (lexer->current_char != '\0')
    {
        if (lexer->current_char == '#')
        {
            while (lexer->current_char != '\n' && lexer->current_char != '\0')
            {
                advance(lexer);
            }
            printf("[LEXER] 跳过注释\n");
            continue; // 跳过注释后继续处理其他token
        }
        // 处理单字符分隔符
        switch (lexer->current_char)
        {
        case ':':
            advance(lexer);
            return new_token(TOKEN_COLON, ":");
        case ';':
            advance(lexer);
            return new_token(TOKEN_SEMI, ";");
        case '{':
            advance(lexer);
            return new_token(TOKEN_LBRACE, "{");
        case '}':
            advance(lexer);
            return new_token(TOKEN_RBRACE, "}");
        case '\n':
            return handle_newline_and_indent(lexer);
        default:
            break;
        }

        if (isspace(lexer->current_char))
        {
            advance(lexer);
            continue;
        }

        if (isalpha(lexer->current_char) || lexer->current_char == '_')
        {
            char buffer[256];
            int i = 0;
            // 允许字母、数字和下划线
            while (isalnum(lexer->current_char) || lexer->current_char == '_')
            {
                if (i < 255)
                {
                    buffer[i++] = lexer->current_char;
                    advance(lexer);
                }
                else
                    // 标识符太长，跳过剩余部分
                    advance(lexer);
            }
            buffer[i] = '\0';
            printf("[LEXER] 标识符: %s\n", buffer);
            if (strcmp(buffer, "__c__") == 0)
            {
                // 跳过空白
                while (lexer->current_char == ' ' || lexer->current_char == '\t') advance(lexer);
                if (lexer->current_char == '{')
                {
                    // 进入C代码块
                    int brace_level = 1;
                    advance(lexer); // 跳过第一个{
                    int start = lexer->pos;
                    while (lexer->current_char != '\0' && brace_level > 0)
                    {
                        if (lexer->current_char == '{') brace_level++;
                        else if (lexer->current_char == '}') brace_level--;
                        advance(lexer);
                    }
                    int end = lexer->pos - 1; // 不包含最后一个}
                    int len = end - start;
                    char* c_code = (char*)malloc(len + 1);
                    strncpy(c_code, lexer->source + start, len);
                    c_code[len] = '\0';
                    return new_token(TOKEN_C_BLOCK, c_code);
                }
                // 如果不是 { ，作为普通标识符处理
            }

            if (strcmp(buffer, "say") == 0)
                return new_token(TOKEN_SAY, "say");
            if (strcmp(buffer, "start") == 0 && lexer->current_char == ':')
            {
                advance(lexer);
                return new_token(TOKEN_START, "start:");
            }
            // 检查函数关键字
            if (strcmp(buffer, "function") == 0)
                return new_token(TOKEN_FUNCTION, "function");
            if (strcmp(buffer, "end") == 0)
                return new_token(TOKEN_END, "end");
            return new_token(TOKEN_IDENTIFIER, buffer);
        }

        if (lexer->current_char == '"')
        {
            advance(lexer);
            char buffer[256];
            int i = 0;
            while (lexer->current_char != '"' && lexer->current_char != '\0')
            {
                buffer[i++] = lexer->current_char;
                advance(lexer);
            }
            if (lexer->current_char == '"')
                advance(lexer);
            buffer[i] = '\0';
            return new_token(TOKEN_STRING, buffer);
        }

        printf("[LEXER] 未知token 在 %d 位置, 字符 = '%c'\n", lexer->pos, lexer->current_char);
        Token *unknown = new_token(TOKEN_UNKNOWN, (char[]){lexer->current_char, '\0'});
        advance(lexer);
        return unknown;
    }

    // 文件结束时处理剩余缩进
    if (lexer->indent_top > 0)
    {
        lexer->indent_top--;
        lexer->pending_dedents = lexer->indent_top;
        return new_token(TOKEN_DEDENT, NULL);
    }
    return new_token(TOKEN_EOF, "");
}

// 处理换行和缩进
Token *handle_newline_and_indent(Lexer *lexer)
{
    // 跳过当前换行符
    if (lexer->current_char == '\n')
    {
        advance(lexer);
    }

    // 检查是否到达EOF
    if (lexer->current_char == '\0')
    {
        printf("[LEXER] 文件结束，不生成token\n");
        return NULL;
    }

    int new_indent = 0;

    // 计算当前行的缩进
    while (lexer->current_char == ' ' || lexer->current_char == '\t')
    {
        new_indent++;
        advance(lexer);

        // 检查是否到达行尾或文件尾
        if (lexer->current_char == '\0')
        {
            printf("[LEXER] 文件结束，不生成token\n");
            return NULL;
        }
    }

    // 添加调试信息
    printf("[LEXER] 新行: new_indent=%d, current_indent_stack=%d\n",
           new_indent, lexer->indent_stack[lexer->indent_top]);

    // 如果遇到连续换行符或文件结束
    if (lexer->current_char == '\n' || lexer->current_char == '\0')
    {
        printf("[LEXER] 遇到换行或文件结束，返回NEWLINE token\n");
        return new_token(TOKEN_NEWLINE, NULL);
    }

    int current_indent = lexer->indent_stack[lexer->indent_top];

    // 处理缩进级别变化
    if (new_indent > current_indent)
    {
        lexer->indent_top++;
        lexer->indent_stack[lexer->indent_top] = new_indent;
        return new_token(TOKEN_INDENT, NULL);
    }
    else if (new_indent < current_indent)
    {
        // 计算需要退出的缩进层级数
        int levels_to_dedent = 0;
        while (lexer->indent_top > 0 && new_indent < lexer->indent_stack[lexer->indent_top])
        {
            lexer->indent_top--;
            levels_to_dedent++;
        }

        // 返回一个 DEDENT 符号
        // 设置 pending_dedents（确保不为负），next_token() 可能需要生成的额外 DEDENT
        if (levels_to_dedent > 0) {
            lexer->pending_dedents = levels_to_dedent - 1;
        } else {
            // new_indent >= current_indent 或者栈为空
            // 本应由 handle_newline_and_indent 中更早的条件捕获（安全起见设置为 0）
            lexer->pending_dedents = 0;
        }
        
        printf("[LEXER] 生成DEDENT: new_indent=%d, old_current_indent=%d, levels_dedented=%d, pending_dedents_set_to=%d\n",
               new_indent, current_indent, levels_to_dedent, lexer->pending_dedents);

        return new_token(TOKEN_DEDENT, NULL);
    }
    else
    {
        return new_token(TOKEN_NEWLINE, NULL);
    }
}