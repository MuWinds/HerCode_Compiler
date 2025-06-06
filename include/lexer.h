#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_EOF,
    TOKEN_UNKNOWN = 1,
    TOKEN_C_BLOCK, // 支持 __c__ { ... } 内联C代码块
    TOKEN_SAY,
    TOKEN_STRING,
    TOKEN_SEMI, // 不再使用
    TOKEN_START,
    TOKEN_END,
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_COLON,
    TOKEN_FUNCTION,  // function 关键字
    TOKEN_LBRACE,    // {
    TOKEN_RBRACE,    // }
    TOKEN_IDENTIFIER // 普通标识符
} TokenType;

typedef struct Token
{
    TokenType type;
    char *value;
} Token;

typedef struct Lexer
{
    char *source;
    int pos;
    char current_char;
    int current_indent;    // 当前行的缩进（空格数）
    int indent_stack[100]; // 缩进级别的栈，用于记录每一层的缩进量
    int indent_top;        // 栈顶指针
    int pending_dedents;   // 待生成的DEDENT数量（当遇到减少缩进时，需要生成多个DEDENT）
} Lexer;

Lexer *new_lexer(char *source);
void free_lexer(Lexer *lexer);
Token *next_token(Lexer *lexer);
Token *handle_newline_and_indent(Lexer *lexer);

#endif