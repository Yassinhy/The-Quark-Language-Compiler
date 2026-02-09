#ifndef UTILS_H
#define UTILS_H
#include "includes.h"
#include "definetions.h"
#include <stddef.h>

typedef enum
{
    // Control tokens
    TOK_NONE, // 0 - No token/error
    TOK_EOF,  // 1 - End of file

    // Assignment (lowest precedence)
    TOK_EQUAL, // 2 - =

    // Logical operators
    TOK_OR,  // 3 - ||
    TOK_AND, // 4 - &&

    // Comparison
    TOK_EQ, // 5 - ==
    TOK_NE, // 6 - !=
    TOK_LT, // 7 - <
    TOK_GT, // 8 - >
    TOK_LE, // 9 - <=
    TOK_GE, // 10 - >=

    // Arithmetic
    TOK_ADD,     // 11 - +
    TOK_SUB,     // 12 - -
    TOK_MUL,     // 13 - *
    TOK_DIV,     // 14 - /
    TOK_PERCENT, // 15 - %

    // Unary operators (high precedence)
    TOK_NOT,    // 16 - !
    TOK_BITNOT, // 17 - ~
    TOK_NEGATE, // 18 - - (unary)

    // Primary expressions (highest)
    TOK_NUMBER,     // 19
    TOK_STRING,     // 20
    TOK_IDENTIFIER, // 21
    TOK_TRUE,       // 22
    TOK_FALSE,      // 23
    TOK_NULL,       // 24

    // Grouping
    TOK_LPAREN,   // 25 - (
    TOK_RPAREN,   // 26 - )
    TOK_LBRACE,   // 27 - {
    TOK_RBRACE,   // 28 - }
    TOK_LBRACKET, // 29 - [
    TOK_RBRACKET, // 30 - ]

    // Punctuation
    TOK_SEMICOLON, // 31 - ;
    TOK_COMMA,     // 32 - ,
    TOK_DOT,       // 33 - .
    TOK_COLON,     // 34 - :
    TOK_QUESTION,  // 35 - ?

    // Keywords
    TOK_EXIT,     // 36 - exit
    TOK_LET,      // 37 - let
    TOK_IF,       // 38 - if
    TOK_ELSE,     // 39 - else
    TOK_WHILE,    // 40 - while
    TOK_FOR,      // 41 - for
    TOK_FN,       // 42 - fn
    TOK_RETURN,   // 43 - return
    TOK_BREAK,    // 44 - break
    TOK_CONTINUE, // 45 - continue

    // Bitwise operators (medium precedence)
    TOK_BIT_OR,      // 46 - |
    TOK_BIT_AND,     // 47 - &
    TOK_BIT_XOR,     // 48 - ^
    TOK_SHIFT_LEFT,  // 49 - <<
    TOK_SHIFT_RIGHT, // 50 - >>

    // TOKEN OF DATA TYPES
    TOK_DATATYPE, // 51 - data type like :int, :bool, :string
    TOK_VOID,     // 52 - void data type
} TokenType;

typedef enum
{
    DATA_TYPE_INT = 0,
    DATA_TYPE_FLOAT = 1,
    DATA_TYPE_DOUBLE = 2,
    DATA_TYPE_CHAR = 3,
    DATA_TYPE_LONG = 4,
    DATA_TYPE_BOOL = 5,
    DATA_TYPE_STRING = 6,
    DATA_TYPE_VOID = 7,
    DATA_TYPE_UNDEFINED = 8,
    DATA_TYPE_POINTER = 9,
} Data_type;

// Precedense values
typedef enum
{
    PREC_NONE = 0,
    PREC_EQUAL = 1,
    PREC_OR = 2,
    PREC_AND = 3,
    PREC_BITWISE = 4,
    PREC_EQUALITY = 5,
    PREC_COMPARISON = 6,
    PREC_BITSHIFT = 7,
    PREC_TERM = 8,
    PREC_FACTOR = 9,
    PREC_UNARY = 10,
    PREC_CALL = 11,
    PREC_PRIMARY = 12
} Precedence;

typedef enum
{
    ERROR_SYNTAX = 1,
    ERROR_TYPE_MISMATCH,
    ERROR_UNDEFINED_VARIABLE,
    ERROR_UNDEFINED_FUNCTION,
    ERROR_ARGUMENT_COUNT,
    ERROR_DIVISION_BY_ZERO,
    ERROR_MEMORY_ALLOCATION,
    ERROR_RUNTIME,
    ERROR_INTERNAL,
    ERROR_UNDEFINED,
} error_code;

typedef struct token
{
    size_t line;
    TokenType type;
    union
    {
        struct
        {
            char *starting_value;
            size_t length;
        } str_value;
        int int_value;
        Data_type data_type;
    };
} token;

// parser structure
typedef struct Parser
{
    token *tokens;  // Array of tokens from tokenizer
    size_t current; // Current position in token array
    size_t token_count;
    // error reporting
    size_t current_line; // Current line number
} Parser;

typedef enum
{
    EXPR_INT,
    EXPR_BOOL,
    EXPR_STRING,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_IDENTIFIER,
    EXPR_FUNCTION_CALL
} ExpressionType;

typedef enum
{
    STORE_IN_REGISTER,
    STORE_IN_FLOAT_REGISTER,
    STORE_IN_STACK,
    STORE_AS_PARAM,
    // MAY ADD STORE FROM POINTER for generic let x = 8; statements later
} variable_storage_type;

typedef enum
{
    XMM0,
    XMM1,
    XMM2
} float_register;

typedef enum
{
    RDI,
    RSI,
    RDX,
    RCX,
    R8,
    R9
} normal_register;

typedef struct symbol_node
{
    char *var_name;
    uint32_t var_name_size;
    variable_storage_type where_it_is_stored;
    Data_type data_type;
    struct symbol_node *next;
    union
    {
        uint64_t offset;
        uint64_t param_offset;
        normal_register register_location;
        float_register which_float_register;
    };
} symbol_node;

typedef struct expression
{
    ExpressionType type;   // the type of the expression
    Data_type result_type; // the resulting data type of the expression

    union
    {
        struct // number
        {
            int value;
        } integer;

        struct // string
        {
            char *string;
            size_t length;
        } string;

        struct // boolean
        {
            bool bool_value;
        } boolean;

        struct // variables
        {
            char *name;
            size_t length;
            symbol_node *node_in_table;
            size_t hash;
            Data_type data_type;
        } variable;

        // binary expressions (x + y)
        struct
        {
            bool constant_foldable;
            struct expression *left;
            struct expression *right;
            TokenType op;
        } binary;

        // unary expression (-x or !x)
        struct
        {
            TokenType op; // operator
            struct expression *operand;
        } unary;

        // function call
        struct
        {
            char *name;
            size_t name_length;
            struct expression *arguments;
            size_t parameter_count;
        } func_call;
    };

} expression;

// Statement Types
typedef enum
{
    STMT_EXIT,
    STMT_LET,
    STMT_EXPRESSION,
    STMT_BLOCK,
    STMT_IF,
    STMT_ELIF,
    STMT_WHILE,
    STMT_FOR,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_ASSIGNMENT
} StatementType;

// function symbol table (function_node**)
// function node for function hash table

typedef struct function_node
{
    char *name;
    size_t name_length;
    expression *parameters;
    size_t param_count;
    struct statement *code_block;
    struct function_node *next;
    Data_type return_type;
} function_node;

typedef struct symbol_table
{
    symbol_node **symbol_map;
    struct symbol_table *parent_scope; // for fast recursion to parent scope

    Data_type scope_data_type; // for stuff to know what is the data type it should return

    size_t scope_offset; // for subtracting from rsp
    size_t param_offset; // for parameters

} symbol_table;

typedef struct statement
{
    StatementType type;
    union
    {
        // exit statement
        struct
        {
            expression *exit_code;
        } stmnt_exit;

        // let statement
        struct
        {
            char *name;
            size_t name_length;
            expression *value;
            size_t hash;
        } stmnt_let;

        // assign an existing variable sth
        struct
        {
            char *name;
            size_t name_length;
            size_t hash;
            expression *value;
        } stmnt_assign;

        // expressions that could also be considered statements (like i++, function calls)
        struct
        {
            expression *value;
        } stmnt_expression;

        // statement block, {some statements} like the ones of for loops, functions, if statements, etc
        struct
        {
            struct statement **statements;
            size_t statement_count;
            symbol_table *table;
        } stmnt_block;

        // if, else and else if
        struct
        {
            expression *condition;     // probably a bin operation, but all could work
            struct statement *then;    // what to do
            struct statement *or_else; // or else what to do. could be nested for else if or NULL if there is nothing
            int return_percent;
        } stmnt_if;

        // while
        struct
        {
            expression *condition;
            struct statement *body;
            size_t counter; // for labels
        } stmnt_while;

        // for
        struct
        {
            struct statement *initializer; // initializer
            expression *condition;
            expression *increment;
            struct statement *body;
        } stmnt_for;

        // function declaration
        struct
        {
            char *name;
            size_t name_length;
            function_node *function_node;
            size_t index;
        } stmnt_function_declaration;

        // return
        struct
        {
            expression *value; // NULL for "return;"
            Data_type return_data_type;
        } stmnt_return;

        struct
        {
            // no info needed really
        } stmt_break;
    };

} statement;

typedef enum
{
    NODE_STATEMENT,
    NODE_EXPRESSION
} NodeType;

// Node storage
typedef struct node
{
    NodeType type;

    union
    {
        expression *expr;
        statement *stmnt;
    };

} node;

typedef struct AST
{
    Parser *parser;
    node **nodes;
    node **function_nodes;
    size_t function_node_count;
    size_t node_count;
} AST;

// Arena
typedef struct
{
    size_t capacity;
    size_t current_size;
    uint8_t *data;
} Arena;

typedef struct
{
    uint32_t capacity;
    uint32_t current_size;
    struct symbol_table **storage;
} symbol_table_stack;

typedef struct
{

    // If statement counter
    size_t if_statements;

    size_t* end_ifs_stack;
    size_t end_ifs_current;
    size_t end_ifs_capacity;

    // While statements counter
    size_t while_statements;

    size_t* end_whiles_stack;
    size_t end_whiles_current;
    size_t end_whiles_capacity;
} counters;

typedef struct
{
    Arena *token_arena;       // For tokens and lexer data
    Arena *statements_arena;  // For AST nodes and parser data
    Arena *expressions_arena; // For string literals and identifiers
    Arena *symbol_arena;      // For symbol table (if you keep it)

    // parser
    Parser *parser;

    // AST
    AST *ast;

    // Symbol Stack
    symbol_table_stack *symbol_table_stack;

    // function map
    function_node **function_map;

    // Buffer
    char buffer[16 * 1024];
    size_t capacity;
    size_t currentsize;

    counters *counters;

} Compiler;

// Arrays:
extern const int Data_type_sizes[];

extern const char* reg[];

extern const normal_register registers[];

extern const Precedence presedences[];


typedef enum {
    _32_EAX,
    _32_EBX,
    _32_ECX,
    _32_EDX,
    _32_ESI,
    _32_EDI,
    _32_R8D,
    _32_R9D,
} reg32;

typedef enum {
    _64_RAX,
    _64_RBX,
    _64_RCX,
    _64_RDX,
    _64_RSI,
    _64_RDI,
    _64_R8,
    _64_R9,
} reg64;



// file readung utility
char *readfile(const char *filename, size_t *file_length);
void print_token(TokenType type);

#endif