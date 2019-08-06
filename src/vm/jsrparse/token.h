#ifndef TOKEN_H
#define TOKEN_H

extern const char *tokNames[];

#define IS_ASSIGN(tok) (tok <= TOK_MOD_EQ && tok >= TOK_EQUAL)
#define IS_COMPUND_ASSIGN(tok) (tok <= TOK_MOD_EQ && tok > TOK_EQUAL)

#define _COMPOUND_ASS_TO_OP_OFF (TOK_PLUS - TOK_PLUS_EQ)
#define COMPUND_ASS_TO_OP(ass) (ass + _COMPOUND_ASS_TO_OP_OFF)

typedef enum {
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LSQUARE,
    TOK_RSQUARE,

    TOK_BANG,
    TOK_BANG_EQ,
    TOK_COMMA,
    TOK_DOT,
    TOK_VARARG,

    TOK_EQUAL,
    TOK_PLUS_EQ,
    TOK_MINUS_EQ,
    TOK_DIV_EQ,
    TOK_MULT_EQ,
    TOK_MOD_EQ,

    TOK_EQUAL_EQUAL,
    TOK_GT,
    TOK_GE,
    TOK_LT,
    TOK_LE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_DIV,
    TOK_MULT,
    TOK_MOD,
    TOK_POW,
    TOK_HASH,
    TOK_HASH_HASH,

    TOK_COLON,
    TOK_SEMICOLON,
    TOK_IN,

    TOK_THEN,
    TOK_DO,
    TOK_BEGIN,
    TOK_END,

    TOK_IDENTIFIER,
    TOK_STRING,
    TOK_NUMBER,

    TOK_AND,
    TOK_CLASS,
    TOK_ELSE,
    TOK_FALSE,
    TOK_NAT,
    TOK_FUN,
    TOK_FOR,
    TOK_IF,
    TOK_ELIF,
    TOK_NULL,
    TOK_OR,
    TOK_PRINT,
    TOK_RETURN,
    TOK_IMPORT,
    TOK_AS,
    TOK_IS,
    TOK_SUPER,
    TOK_TRUE,
    TOK_VAR,
    TOK_WHILE,
    TOK_CONTINUE,
    TOK_BREAK,

    TOK_TRY,
    TOK_EXCEPT,
    TOK_ENSURE,
    TOK_RAISE,

    TOK_UNTERMINATED_STR,
    TOK_NEWLINE,
    TOK_ERR,
    TOK_EOF
} TokenType;

typedef struct Token {
    TokenType type;
    const char *lexeme;
    int length;
    int line;
} Token;

#endif