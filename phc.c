#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>


typedef char* string;
#define upper 4
FILE* output = NULL;
size_t glbl_offset = 0;


// gcc -pg test.c -o test
// ./test
// gprof test gmon.out

typedef enum {
    // Control tokens
    TOK_NONE,           // 0 - No token/error
    TOK_EOF,            // 1 - End of file
    
    // Assignment (lowest precedence)
    TOK_EQUAL,     // 2 - =
    
    // Logical operators  
    TOK_OR,             // 3 - ||
    TOK_AND,            // 4 - &&
    
    // Comparison
    TOK_EQ,             // 5 - ==
    TOK_NE,             // 6 - !=
    TOK_LT,             // 7 - <
    TOK_GT,             // 8 - >
    TOK_LE,             // 9 - <=  
    TOK_GE,             // 10 - >=
    
    // Arithmetic
    TOK_ADD,            // 11 - +
    TOK_SUB,            // 12 - -
    TOK_MUL,            // 13 - *
    TOK_DIV,            // 14 - /
    TOK_PERCENT,        // 15 - %
    
    // Unary operators (high precedence)
    TOK_NOT,            // 16 - !
    TOK_BITNOT,         // 17 - ~
    TOK_NEGATE,         // 18 - - (unary)
    
    // Primary expressions (highest)
    TOK_NUMBER,         // 19
    TOK_STRING,         // 20
    TOK_IDENTIFIER,     // 21
    TOK_TRUE,           // 22
    TOK_FALSE,          // 23
    TOK_NULL,           // 24
    
    // Grouping
    TOK_LPAREN,         // 25 - (
    TOK_RPAREN,         // 26 - )
    TOK_LBRACE,         // 27 - {
    TOK_RBRACE,         // 28 - }
    TOK_LBRACKET,       // 29 - [
    TOK_RBRACKET,       // 30 - ]
    
    // Punctuation  
    TOK_SEMICOLON,      // 31 - ;
    TOK_COMMA,          // 32 - ,
    TOK_DOT,            // 33 - .
    TOK_COLON,          // 34 - :
    TOK_QUESTION,       // 35 - ?
    
    // Keywords
    TOK_EXIT,           // 36 - exit
    TOK_LET,            // 37 - let
    TOK_IF,             // 38 - if
    TOK_ELSE,           // 39 - else
    TOK_WHILE,          // 40 - while
    TOK_FOR,            // 41 - for
    TOK_FN,             // 42 - fn
    TOK_RETURN,         // 43 - return
    TOK_BREAK,          // 44 - break
    TOK_CONTINUE,       // 45 - continue
    
    // Bitwise operators (medium precedence)
    TOK_BIT_OR,         // 46 - |
    TOK_BIT_AND,        // 47 - &
    TOK_BIT_XOR,        // 48 - ^
    TOK_SHIFT_LEFT,     // 49 - <<
    TOK_SHIFT_RIGHT,    // 50 - >>

    // TOKEN OF DATA TYPES
    TOK_DATATYPE,      // 51 - data type like :int, :bool, :string
    TOK_VOID,           // 52 - void data type
} TokenType;


typedef enum {
    DATA_TYPE_INT,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_CHAR,
    DATA_TYPE_LONG,
    DATA_TYPE_BOOL,
    DATA_TYPE_STRING,
    DATA_TYPE_VOID,
    DATA_TYPE_UNDEFINED,
    DATA_TYPE_POINTER,
} Data_type;

const int Data_type_sizes[] = {
    [DATA_TYPE_INT] = 4,
    [DATA_TYPE_FLOAT] = 4,
    [DATA_TYPE_DOUBLE] = 8,
    [DATA_TYPE_CHAR] = 1,
    [DATA_TYPE_LONG] = 8,
    [DATA_TYPE_BOOL] = 1,
    [DATA_TYPE_STRING] = 8,
    [DATA_TYPE_VOID] = 0,
    [DATA_TYPE_UNDEFINED] = 0,
    [DATA_TYPE_POINTER] = 8,
};

// Precedense values
typedef enum {
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

typedef enum{
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

const Precedence presedences[] = {
    // Control tokens - stops parsing
    [TOK_NONE] = PREC_NONE,           // 0
    [TOK_EOF] = PREC_NONE,            // 0
    [TOK_SEMICOLON] = PREC_NONE,      // 0
    
    // Assignment - lowest precedence
    [TOK_EQUAL] = PREC_EQUAL,  // 1  (=)
    
    // Logical OR
    [TOK_OR] = PREC_OR,                  // 2  (||)
    
    // Logical AND  
    [TOK_AND] = PREC_AND,                // 3  (&&)


    // Bitwise OR/XOR/AND
    [TOK_BIT_OR] = PREC_BITWISE,         // 4  (|)
    [TOK_BIT_XOR] = PREC_BITWISE,        // 4  (^)
    [TOK_BIT_AND] = PREC_BITWISE,        // 4  (&)
    
    // Equality comparison
    [TOK_EQ] = PREC_EQUALITY,            // 5  (==)
    [TOK_NE] = PREC_EQUALITY,            // 5  (!=)
    
    // Relational comparison
    [TOK_LT] = PREC_COMPARISON,          // 6  (<)
    [TOK_GT] = PREC_COMPARISON,          // 6  (>)
    [TOK_LE] = PREC_COMPARISON,          // 6  (<=)
    [TOK_GE] = PREC_COMPARISON,          // 6  (>=)
    
    // Bit shifts
    [TOK_SHIFT_LEFT] = PREC_BITSHIFT,    // 7  (<<)
    [TOK_SHIFT_RIGHT] = PREC_BITSHIFT,   // 7  (>>)
    
    // Addition/Subtraction
    [TOK_ADD] = PREC_TERM,               // 8  (+)
    [TOK_SUB] = PREC_TERM,               // 8  (-)
    
    // Multiplication/Division/Modulo
    [TOK_MUL] = PREC_FACTOR,             // 9  (*)
    [TOK_DIV] = PREC_FACTOR,             // 9  (/)
    [TOK_PERCENT] = PREC_FACTOR,         // 9  (%)
    
    // Unary operators - high precedence
    [TOK_NOT] = PREC_UNARY,              // 10 (!)
    [TOK_BITNOT] = PREC_UNARY,           // 10 (~)
    [TOK_NEGATE] = PREC_UNARY,           // 10 (- unary)
    
    // Function calls, member access - highest
    [TOK_DOT] = PREC_CALL,               // 11 (.)
    [TOK_LPAREN] = PREC_CALL,            // 11 ( function calls)
    [TOK_LBRACKET] = PREC_CALL,          // 11 ([ array access)
    
    // Primary expressions - no operators
    [TOK_NUMBER] = PREC_NONE,
    [TOK_STRING] = PREC_NONE,
    [TOK_IDENTIFIER] = PREC_NONE,
    [TOK_TRUE] = PREC_NONE,
    [TOK_FALSE] = PREC_NONE,
    [TOK_NULL] = PREC_NONE,
    
    // Punctuation - no precedence
    [TOK_COMMA] = PREC_NONE,
    [TOK_COLON] = PREC_NONE,
    [TOK_QUESTION] = PREC_NONE,
    [TOK_LBRACE] = PREC_NONE,
    [TOK_RBRACE] = PREC_NONE,
    [TOK_RPAREN] = PREC_NONE,
    [TOK_RBRACKET] = PREC_NONE,
    
    // Keywords - no precedence
    [TOK_EXIT] = PREC_NONE,
    [TOK_LET] = PREC_NONE,
    [TOK_IF] = PREC_NONE,
    [TOK_ELSE] = PREC_NONE,
    [TOK_WHILE] = PREC_NONE,
    [TOK_FOR] = PREC_NONE,
    [TOK_FN] = PREC_NONE,
    [TOK_RETURN] = PREC_NONE,
    [TOK_BREAK] = PREC_NONE,
    [TOK_CONTINUE] = PREC_NONE,
};



//GLOBAL VARIABLES
size_t file_length = 0;
size_t token_count = 0;


typedef struct token
{
    size_t line;
    TokenType type;
    union
    {
        struct test
        {
            string starting_value;
            size_t length;
        } str_value;
        int int_value;
        Data_type data_type;
    };
}token;

// parser structure
typedef struct Parser {
    token* tokens;           // Array of tokens from tokenizer
    size_t current;           // Current position in token array
    // error reporting
    size_t current_line;      // Current line number
} Parser;


typedef enum {
    EXPR_INT,
    EXPR_BOOL,
    EXPR_STRING,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_IDENTIFIER,
    EXPR_FUNCTION_CALL
}ExpressionType;

typedef enum {
    STORE_IN_REGISTER,
    STORE_IN_FLOAT_REGISTER,
    STORE_IN_STACK,
    STORE_AS_PARAM,
    //MAY ADD STORE FROM POINTER for generic let x = 8; statements later
} variable_storage_type;

typedef enum {
    XMM0,
    XMM1,
    XMM2
} float_register;

typedef enum {
    RDI,
    RSI,
    RDX,
    RCX,
    R8,
    R9
} normal_register;

typedef struct symbol_node {
    string var_name;
    uint32_t var_name_size;
    variable_storage_type where_it_is_stored;
    Data_type data_type;
    struct symbol_node* next;
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
    ExpressionType type; //the type of the expression
    Data_type result_type; // the resulting data type of the expression

    union
    {
        struct //number
        {
            int value;
        } integer;

        struct // string
        {
            string string;
            size_t length;
        } string;
        
        struct // boolean
        {
            bool bool_value;
        } boolean;

        struct // variables
        {
            string name;
            size_t length;
            symbol_node* node_in_table;
            size_t hash;
            Data_type data_type;
        } variable;
        
        
        // binary expressions (x + y)
        struct
        {
            bool constant_foldable;
            struct expression* left;
            struct expression* right;
            TokenType operator;
        } binary;
        
        // unary expression (-x or !x)
        struct
        {
            TokenType operator;
            struct expression* operand;
        } unary;
        
        // function call
        struct
        {
            string name;
            size_t name_length;
            struct expression* arguments;
            size_t parameter_count;
        } func_call;
        
    };
    
}expression;


// Statement Types
typedef enum {
    STMT_EXIT,
    STMT_LET,
    STMT_EXPRESSION,
    STMT_BLOCK,
    STMT_IF,
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
    string name;
    size_t name_length;
    expression* parameters;
    size_t param_count;
    struct statement* code_block;
    struct function_node* next;
    Data_type return_type;
} function_node;

typedef struct symbol_table{
    symbol_node** symbol_map;
    struct symbol_table* parent_scope; // for fast recursion to parent scope

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
            expression* exit_code;
        } stmnt_exit;

        //let statement
        struct
        {
            string name;
            size_t name_length;
            expression* value;
            size_t hash;
        } stmnt_let;

        // assign an existing variable sth
        struct
        {
            string name;
            size_t name_length;
            size_t hash;
            expression* value;
        } stmnt_assign;
        

        // expressions that could also be considered statements (like i++, function calls)
        struct
        {
            expression* value;
        } stmnt_expression;
        
        // statement block, {some statements} like the ones of for loops, functions, if statements, etc
        struct
        {
            struct statement** statements;
            size_t statement_count;
            symbol_table* table;
        } stmnt_block;

        // if, else and else if
        struct
        {
            expression* condition;
            struct statement* then;         // what to do
            struct statement* or_else;      // or else what to do. could be nested for else if or NULL if there is nothing
        }stmnt_if;
        
        // while
        struct
        {
            expression* condition;
            struct statement* body;
        }stmnt_while;
        
        // for
        struct
        {
            struct statement* initializer;      // initializer
            expression* condition;
            expression* increment;
            struct statement* body;
        }stmnt_for;

        // function declaration
        struct
        {
            string name;
            size_t name_length;
            function_node* function_node;
            size_t index;
        } stmnt_function_declaration;
        

        // return
        struct
        {
            expression* value;  // NULL for "return;"
        } stmnt_return;
        
    };
    
}statement;



typedef enum {
    NODE_STATEMENT,
    NODE_EXPRESSION
} NodeType;

// Node storage
typedef struct node{
    NodeType type;
    
    union
    {
        expression* expr;
        statement* stmnt;
    };
    
}node;

typedef struct AST
{
    Parser* parser;
    node** nodes;
    size_t node_count;
}AST;






#define token_size sizeof(token)
#define node_size sizeof(node)
#define statement_size sizeof(statement)
#define expression_size sizeof(expression)
#define BUCKETS_GLOBAL_SYMBOLTABLE 32
#define BUCKETS_FUNCTION_TABLE 32
size_t default_capacity = 1024 * 1024 * 4;

// Arena
typedef struct {
    size_t capacity;
    size_t current_size;
    uint8_t* data;
} Arena;

static void panic(error_code error_code, string message);
Arena* initialize_arena(size_t capacity);
void free_arena(Arena* arena);

static inline size_t hash_function(const string var_name, size_t var_name_length)
{
    return ((var_name[0] + 'a') * var_name_length + var_name[var_name_length - 1] * var_name_length / 2);
}

//////////////
 char* reg[] = {"rdi", "rsi", "rdx", "rcx", "r8 ", "r9 "};
 const normal_register registers[] = {
    // Control tokens - stops parsing
    [0] = RDI,           // 0
    [1] = RSI,            // 0
    [2] = RDX,
    [3] = RCX,
    [4] = R8,
    [5] = R9
 };


typedef struct {
    uint32_t capacity;
    uint32_t current_size;
    struct symbol_table** storage;
} symbol_table_stack;

#define BUCKETS_IN_EACH_SYMBOL_MAP 16

// peek top scope
static inline symbol_table* peek_symbol_stack(void);


// create new scope
static inline void enter_new_scope(void);

// push variable to current scope
static inline symbol_node* add_var_to_current_scope(expression* variable, variable_storage_type storage_type, normal_register reg_location);

// find variable in scopes
static inline symbol_node* find_variable(uint32_t hash, string var_name, uint32_t var_name_length);

/////////////

typedef struct {
    Arena* token_arena;     // For tokens and lexer data
    Arena* statements_arena;       // For AST nodes and parser data  
    Arena* expressions_arena;    // For string literals and identifiers
    Arena* symbol_arena;    // For symbol table (if you keep it)

    // Symbol Stack
    symbol_table_stack* symbol_table_stack; 

    // function map
    function_node** function_map;

    // Buffer
    char buffer[16 * 1024];
    size_t capacity;
    size_t currentsize;
} Compiler;

Compiler* init_compiler_arenas(size_t file_length);

void free_global_arenas(Compiler* arenas) {
    if (arenas->token_arena) free_arena(arenas->token_arena);
    if (arenas->statements_arena) free_arena(arenas->statements_arena);
    if (arenas->expressions_arena) free_arena(arenas->expressions_arena);
    if(arenas->symbol_arena) free_arena(arenas->symbol_arena);
    if (arenas->symbol_table_stack) {
        if (arenas->symbol_table_stack->storage) {
            free(arenas->symbol_table_stack->storage);
        }
        free(arenas->symbol_table_stack);
    }
    free(arenas);
}



// GLOBAL AST, parser and tokens** POINTER FOR PANIC FUNCTIONALITY
AST* glbl_ast = NULL;
Parser* glbl_parser = NULL;
token* glbl_tokens = NULL;

//initialize compiler
Compiler* compiler = NULL;

// Parser functions
static Parser* make_parser(Compiler* compiler);
static inline token* peek(Parser* parser, size_t offset);
static inline token* advance(Parser* parser);
static inline bool match(Parser* parser, TokenType type);

// Parsing functions
static inline node* parse_statement(Parser* parser);
static inline node* parse_expression(Parser* parser, int prev_presedence, bool constant_foldable);
static inline node* parse_exit_node(Parser* parser);
static inline node* parse_let_node(Parser* parser);
static inline node* parse_assignment_node(Parser* parser);
static inline node* parse_function_node(Parser* parser);

AST* parse(Parser* parser);
node* parse_prefix(Parser* parser, bool constant_foldable);

//node generation
node* create_number_node(int value);
node* create_bin_node(node* left, TokenType operator, Parser* parser, bool constant_foldable);

//File handling
static string readfile(const string filename);

// Tokenizer functions
static void tokenize(const string source, Compiler* compiler);
static inline int identify_token(const string value, size_t length, token* the_token);

// Code generation
void write_to_buffer(string code, size_t code_length, FILE* output);
void generate_assembly(const AST* AST);
void generate_statement_code(statement* stmt, size_t* num_len);
static inline string _u64_to_str(size_t number, size_t* num_len);
static inline void nums_to_str(size_t number, size_t* num_len, FILE* output);

//evaluation functions
int evaluate_bin(expression* binary_exp);
void evaluate_expression(expression* expr);

// Free functions
static inline void free_ast(AST* ast);
static inline void free_parser(Parser* parser);

// Arena functions
bool arena_realloc(Arena* arena, size_t data_size);
void* arena_alloc(Arena* arena, size_t data_size);
void arena_reset(Arena* arena);
static inline void exit_current_scope(void) {
    if (compiler->symbol_table_stack->current_size > 1) {
        compiler->symbol_table_stack->current_size--;
    } else {
        panic(ERROR_INTERNAL, "Tried to exit global scope");
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]){
    printf("x\n");
    clock_t start = clock();

    printf("lol\n");
    if (argc != 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    //read file
    string source = readfile(argv[1]);
    printf("no\n");
    //initialize compiler arenas
    compiler = init_compiler_arenas(file_length);

    //tokenize
    tokenize(source, compiler);  // This sets token_count
    printf("hi\n");
    glbl_tokens = (token*)compiler->token_arena->data;
    if (token_count == 0) {
        panic(ERROR_INTERNAL, "ERROR: No tokens created! Check your tokenizer.");
    }
    
    //initialize symbol tables
    

    //create parser
    Parser* parser = make_parser(compiler);
    glbl_parser = parser;
    parser->tokens = (token*)compiler->token_arena->data;
    for (int i = 0; i < token_count; i++)
    {
        printf("token %i: %i\n", i, peek(parser, i)->type);
    }
    
    //parse tokens into AST
    AST* ast = arena_alloc(compiler->statements_arena, sizeof(AST));
    glbl_ast = ast;
    ast->nodes = arena_alloc(compiler->statements_arena, sizeof(node*) * token_count);  // Allocate pointer array
    ast->node_count = 0;
    ast->parser = parser;
    size_t i = 0;
    printf("checkpoint 1\n");
    while (parser->current < token_count - 1) {
        node* parsed_node = parse_statement(parser);
        i++;
        if (parsed_node) {
            ast->nodes[ast->node_count++] = parsed_node;
        } else {
            break;
        }
        if (peek(parser, 0)->type == TOK_SEMICOLON)
        {
            advance(parser);
        }
    }

    
    if (ast->nodes == NULL) {
        panic(ERROR_INTERNAL, "ERROR: AST creation failed!");
    }

    generate_assembly((const AST*)ast);
    
    
    free(source);
    free_global_arenas(compiler);
    
    clock_t end = clock();
    system("nasm -f elf64 output.asm && ld output.o -o output");
    double time_spent = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Code executed in %.6f seconds\n", time_spent);
    
    return 0;
}


////////////////////////////////////////////////////    FREEING FUNCTIONS    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static inline void free_parser(Parser* parser){
    free(parser);
}


static inline void free_ast (AST* ast)
{
    free(ast->parser);
    free(ast->nodes);
    free(ast);
}

static void panic(error_code error_code, string message)
{
    switch (error_code) {
        case ERROR_SYNTAX:
            fprintf(stderr, "Syntax error");
            break;
        case ERROR_TYPE_MISMATCH:
            fprintf(stderr, "Type mismatch");
            break;
        case ERROR_UNDEFINED_VARIABLE:
            fprintf(stderr, "Undefined variable");
            break;
        case ERROR_UNDEFINED_FUNCTION:
            fprintf(stderr, "Undefined function");
            break;
        case ERROR_ARGUMENT_COUNT:
            fprintf(stderr, "Wrong number of arguments");
            break;
        case ERROR_DIVISION_BY_ZERO:
            fprintf(stderr, "Division by zero");
            break;
        case ERROR_MEMORY_ALLOCATION:
            fprintf(stderr, "Memory allocation failed");
            break;
        case ERROR_RUNTIME:
            fprintf(stderr, "Runtime error");
            break;
        case ERROR_INTERNAL:
            fprintf(stderr, "Internal compiler error");
            break;
        default:
            fprintf(stderr, "Unknown error");
            break;
    }
    if(glbl_parser){
        fprintf(stderr, " at line %zu: \n", glbl_parser->current_line);
    }
    fprintf(stderr, "\n%s\n", message);
    fprintf(stderr, "compilation process stopped\n");

    free_global_arenas(compiler);

    if (glbl_ast)
    {
        fprintf(stderr, "freeing the AST\n");
        free(glbl_ast->nodes);
        free(glbl_ast->parser);
        free(glbl_ast);
        exit(error_code);
    }

    else if (glbl_parser)
    {
        fprintf(stderr, "freeing the parser\n");
        free(glbl_parser);
        exit(error_code);
    }

    else
    {
        fprintf(stderr, "nothing to free\n");
        exit(error_code);
    }
    

}

////////////////////////////////////////////////////////////// ARENA ////////////////////////////////////////////
Compiler* init_compiler_arenas(size_t file_length) {
    Compiler* arenas = malloc(sizeof(Compiler));
    arenas->token_arena = initialize_arena((file_length + 1) * token_size);
    arenas->statements_arena = initialize_arena(file_length * (expression_size + node_size));
    arenas->expressions_arena = initialize_arena(file_length * expression_size);
    arenas->symbol_arena = initialize_arena(default_capacity);

    // make the stack struct
    arenas->symbol_table_stack = malloc(sizeof(symbol_table_stack));
    if (!arenas->symbol_table_stack) panic(ERROR_MEMORY_ALLOCATION, "Symbol table stack allocation failed");
    
    // make the initial stack (16 scopes and could grow dynamicaly)
    arenas->symbol_table_stack->storage = malloc(16 * sizeof(symbol_table*));
    if (!arenas->symbol_table_stack->storage) panic(ERROR_MEMORY_ALLOCATION, "Symbol table stack allocation failed");
    
    // initialize capacity
    arenas->symbol_table_stack->capacity = 16;
    
    // initialize global stack
    arenas->symbol_table_stack->storage[0] = arena_alloc(arenas->symbol_arena, sizeof(symbol_table));
    arenas->symbol_table_stack->storage[0]->parent_scope = NULL;
    arenas->symbol_table_stack->storage[0]->scope_offset = 0;
    arenas->symbol_table_stack->storage[0]->symbol_map = arena_alloc(arenas->symbol_arena, BUCKETS_GLOBAL_SYMBOLTABLE * sizeof(symbol_node*));
    memset(arenas->symbol_table_stack->storage[0]->symbol_map, 0, BUCKETS_GLOBAL_SYMBOLTABLE * sizeof(symbol_node*));

    arenas->symbol_table_stack->current_size = 1;

    arenas->function_map = arena_alloc(arenas->symbol_arena, BUCKETS_FUNCTION_TABLE * sizeof(function_node**));
    memset(arenas->function_map, 0, BUCKETS_FUNCTION_TABLE * sizeof(function_node**));

    arenas->capacity = 16 * 1024;
    arenas->currentsize = 0;
    return arenas;
}


Arena* initialize_arena(size_t capacity) {
    Arena* arena = malloc(sizeof(Arena));
    if (!arena) return NULL;
    
    arena->data = malloc(capacity);
    if (!arena->data) {
        free(arena);
        return NULL;
    }
    
    arena->capacity = capacity;
    arena->current_size = 0;
    return arena;
}


bool arena_realloc(Arena* arena, size_t data_size) {
    printf("I was called\n");
    panic(0, "arena realloc was called");
    return false;
}

void* arena_alloc(Arena* arena, size_t old_data_size) {
    size_t data_size = (old_data_size + 7) & ~7; // Align to 8 bytes

    if (arena->current_size + data_size > arena->capacity) {
        if (!arena_realloc(arena, data_size)) {
            return NULL;
        }
    }
    void* data = &arena->data[arena->current_size];
    arena->current_size += data_size;
    return data;
}

void arena_reset(Arena* arena) {
    arena->current_size = 0;
}

void free_arena(Arena* arena) {
    if (arena) {
        free(arena->data);
        free(arena);
    }
}



/////////////////////////////////////////////////       FILE READING      ////////////////////////////////////////////////////////////////////////////////////////////////////////////
static string readfile(const string filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate exact size needed
    string content = malloc(size + 1);
    if (content == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }
    
    // Read entire file at once
    file_length = fread(content, 1, size, file);
    content[file_length] = '\0';
    
    fclose(file);
    return content;
}


static inline symbol_table* peek_symbol_stack(void) {
    if (compiler->symbol_table_stack->current_size == 0) {
        return NULL;
    }
    symbol_table* current_table = compiler->symbol_table_stack->storage[compiler->symbol_table_stack->current_size - 1];
    if (!current_table) {
        return NULL;
    }
    return current_table;
}

static inline void enter_new_scope(void) {
    if (!compiler) panic(ERROR_UNDEFINED, "Compiler not initialized");
    if (compiler->symbol_table_stack->current_size + 1 >= compiler->symbol_table_stack->capacity) {
        compiler->symbol_table_stack->storage = realloc(compiler->symbol_table_stack->storage, compiler->symbol_table_stack->capacity * 2 * sizeof(symbol_table*));
        compiler->symbol_table_stack->capacity *= 2;
    }
    if (!compiler->symbol_table_stack->storage) panic(ERROR_MEMORY_ALLOCATION, "Entered too many scopes for your memory");
    symbol_table* new_table = arena_alloc(compiler->symbol_arena, sizeof(symbol_table));
    if (!new_table) panic(ERROR_MEMORY_ALLOCATION, "New symbol table allocation failed");
    
    new_table->parent_scope = peek_symbol_stack();
    
    new_table->scope_offset = 0;

    new_table->symbol_map = arena_alloc(compiler->symbol_arena, 16 * sizeof(symbol_node*));
    if (!new_table->symbol_map) panic(ERROR_MEMORY_ALLOCATION, "new scope unable to be declared, not enough memory");
    memset(new_table->symbol_map, 0, 16 * sizeof(symbol_node*));
    
    compiler->symbol_table_stack->storage[compiler->symbol_table_stack->current_size] = new_table;
    compiler->symbol_table_stack->current_size++;
}


static inline symbol_node* add_var_to_current_scope(expression* variable, variable_storage_type storage_type, normal_register reg_location) {
    symbol_node** new_symbol;
    size_t hash = hash_function(variable->variable.name, variable->variable.length);
    if (peek_symbol_stack()->parent_scope == NULL)
    {
        new_symbol = &peek_symbol_stack()->symbol_map[hash % BUCKETS_GLOBAL_SYMBOLTABLE];
    }
    else
    {
        new_symbol = &peek_symbol_stack()->symbol_map[hash % BUCKETS_IN_EACH_SYMBOL_MAP];
    }
    
    while (*new_symbol != NULL) {
        if ((*new_symbol)->var_name_size == variable->variable.length && 
            !strncmp((*new_symbol)->var_name, variable->variable.name, variable->variable.length)) {
            panic(ERROR_INTERNAL, "Variable already declared in this scope");
        }
        new_symbol = &(*new_symbol)->next;
    }

    (*new_symbol) = arena_alloc(compiler->symbol_arena, sizeof(symbol_node));
    (*new_symbol)->next = NULL;
    (*new_symbol)->var_name = variable->variable.name;
    (*new_symbol)->var_name_size = variable->variable.length;
    (*new_symbol)->data_type = variable->variable.data_type;
    (*new_symbol)->where_it_is_stored = storage_type;
    switch (storage_type)
    {
    case STORE_IN_STACK:
        peek_symbol_stack()->scope_offset += Data_type_sizes[variable->variable.data_type];
        (*new_symbol)->offset = peek_symbol_stack()->scope_offset;
        break;
    case STORE_IN_REGISTER:
        (*new_symbol)->register_location = reg_location;
        break;
    case STORE_AS_PARAM:
        peek_symbol_stack()->param_offset += Data_type_sizes[variable->variable.data_type];
        (*new_symbol)->param_offset = 8 + peek_symbol_stack()->scope_offset; // 8 for the return pointer
        break;
    case STORE_IN_FLOAT_REGISTER:
        //TO DO     
        panic(ERROR_INTERNAL, "Float register storage not implemented yet");
        break;
    default:
        break;
    }
    return (*new_symbol);
}

static inline symbol_node* find_variable(uint32_t hash, string var_name, uint32_t var_name_length) {
    printf("find_variable called\n");
    if(!compiler || !compiler->symbol_table_stack) panic(ERROR_UNDEFINED, "Scope Stack not initialized.");
    if(!compiler->symbol_table_stack->storage) panic(ERROR_UNDEFINED, "Scope Stack storage not initialized.");
    printf("searching for variable: %.*s\n", var_name_length, var_name);
    int64_t i = compiler->symbol_table_stack->current_size - 1;
    printf("number of scopes: %lu", i);
    while (i >= 0)
    {
        symbol_table* current_table = compiler->symbol_table_stack->storage[i];
        symbol_node* searcher;
        if (current_table->parent_scope == NULL)
        {
            searcher = current_table->symbol_map[hash % BUCKETS_GLOBAL_SYMBOLTABLE];
        }
        else{
            searcher = current_table->symbol_map[hash % BUCKETS_IN_EACH_SYMBOL_MAP];
        }
        while (searcher != NULL)
        {
            if (searcher->var_name_size == var_name_length)
            {
                if (strncmp(searcher->var_name, var_name, var_name_length) == 0)
                {
                    return searcher;
                }
                
            }
            searcher = searcher->next;
        }
        i--;
    }
    return NULL;
}

static inline void append_function_to_func_map(function_node* function_node_input) {
    
    function_node** bucket = &(compiler->function_map[hash_function(function_node_input->name, function_node_input->name_length) % BUCKETS_FUNCTION_TABLE]);
    while (*bucket != NULL) {
        if ((*bucket)->name_length == function_node_input->name_length && 
            !strncmp((*bucket)->name, function_node_input->name, function_node_input->name_length)) {
            panic(ERROR_INTERNAL, "Function already declared before");
        }
        bucket = &(*bucket)->next;
    }
    *bucket = function_node_input;
    function_node_input->next = NULL; 
}

static inline function_node* find_function_symbol_node (string function_name, size_t function_name_length) {
    
    function_node** bucket = &(compiler->function_map[hash_function(function_name, function_name_length) % BUCKETS_FUNCTION_TABLE]);
    while (*bucket != NULL) {
        if ((*bucket)->name_length == function_name_length && 
            !strncmp((*bucket)->name, function_name, function_name_length)) {
            return *bucket;
        }
        bucket = &(*bucket)->next;
    }
    panic(ERROR_UNDEFINED_FUNCTION, "Function not defined");
    return NULL; 
}


static const uint8_t CHAR_TYPE[256] = {
    ['0'] = 11, ['1'] = 11, ['2'] =11, ['3'] = 11, ['4'] = 11, ['5'] = 11, ['6'] = 11, ['7'] = 11, ['8'] = 11, ['9'] = 11,
    ['_'] = 1,
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1, ['g'] = 1, ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1, ['m'] = 1, ['n'] = 1, ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1, ['s'] = 1, ['t'] = 1, ['u'] = 1, ['v'] = 1, ['w'] = 1, ['x'] = 1, ['y'] = 1, ['z'] = 1,
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1, ['G'] = 1, ['H'] = 1, ['I'] = 1, ['J'] = 1, ['K'] = 1, ['L'] = 1, ['M'] = 1, ['N'] = 1, ['O'] = 1, ['P'] = 1, ['Q'] = 1, ['R'] = 1, ['S'] = 1, ['T'] = 1, ['U'] = 1, ['V'] = 1, ['W'] = 1, ['X'] = 1, ['Y'] = 1, ['Z'] = 1,
    ['='] = 2, ['+'] = 4, ['-'] = 5, ['*'] = 6, ['/'] = 7, 
    [' '] = 3, ['\n'] = 3, ['\t'] = 3, ['\r'] = 3,
    ['('] = 8, [')'] = 9, [';'] = 10, [':'] = 12, [','] = 13, ['{'] = 14, ['}'] = 15,

};


/////////////////////////////////////////////////// TOKENIZER /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void tokenize(const string source, Compiler* compiler){
    printf("?\n");
    token* tokens = arena_alloc(compiler->token_arena, sizeof(token) * (file_length + 1));


    token_count = 0;
    size_t i = 0;
    size_t line_number = 1;
    printf("check\n");
    while (i < file_length) {
        if (source[i] == '\0') break;

        while (i < file_length && CHAR_TYPE[source[i]] == 3) {
            if (source[i] == '\n') line_number++;
            i++;
        }
        if (i >= file_length) break;
        
        // Process token at position i
        size_t token_start = i;
        printf("char: %c\n", source[i]);
        printf("%i\n", CHAR_TYPE[source[i]]);
        switch (CHAR_TYPE[source[i]])
        {
        case 11:{
            int value = source[i] - '0';
            i++;
            while (i < file_length && (source[i] >= '0' && source[i] <= '9')) {
                value = value * 10 + (source[i] - '0');
                i++;
            }
            tokens[token_count].line = line_number;
            tokens[token_count].int_value = value;
            tokens[token_count].type = TOK_NUMBER;
            token_count++;
            break;
        }
        case 2:{
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_EQUAL;
            token_count++;
            i++;
            break;
        }

        case 5:{
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_SUB;
            token_count++;
            i++;
            break;
        }

        case 4:{
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_ADD;
            token_count++;
            i++;
            break;
        }

        case 6: {
        
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_MUL;
            token_count++;
            i++;
            break;
        }
        case 7: {
        
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_DIV;
            token_count++;
            i++;
            break;
        }

        case 8: {
        
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_LPAREN; // LPAREN           
            token_count++;
            i++;
            break;
        }
        
        case 9:{
        
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_RPAREN; // RPAREN            
            token_count++;
            i++;
            break;
        }

        case 1 :{
            // Identifiers can START only with letters or underscore
            while (i < file_length && CHAR_TYPE[source[i]] == 1) {
                i++;
            }
            tokens[token_count].line = line_number;
            tokens[token_count].str_value.starting_value = &source[token_start];
            tokens[token_count].str_value.length = i - token_start;
            identify_token(tokens[token_count].str_value.starting_value, tokens[token_count].str_value.length, &tokens[token_count]);

            token_count++;
            break;
        }

        case 10:{
        
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_SEMICOLON; // SEMICOLUMN            
            token_count++;
            i++;
            break;
        }
        case 13: {
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_COMMA; // COMMA            
            token_count++;
            i++;
            break;
        }

        case 14: {
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_LBRACE; // {            
            token_count++;
            i++;
            break;
        }

        case 15: {
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_RBRACE; // }           
            token_count++;
            i++;
            break;
        }

        case 12:{
        
            tokens[token_count].line = line_number;         
            i++;
            switch (source[i])
            {
                case 'i':
                    if (source[i + 1] == 'n' && source[i + 2] == 't') {
                        tokens[token_count].type = TOK_DATATYPE;
                        tokens[token_count].data_type = DATA_TYPE_INT;
                        i += 3;
                    }
                    break;
                case 'b':
                    if (source[ i + 1] == 'o' && source[ i + 2] == 'o' && source[ i + 3] == 'l') {
                        tokens[token_count].type = TOK_DATATYPE;
                        tokens[token_count].data_type = DATA_TYPE_BOOL;
                        i += 4;
                    }
                    break;
                case 'c':
                    if (source[ i + 1] == 'h' && source[ i + 2] == 'a' && source[ i + 3] == 'r') {
                        tokens[token_count].type = TOK_DATATYPE;
                        tokens[token_count].data_type = DATA_TYPE_CHAR;
                        i += 4;
                    }
                    break;
                case 'l':
                    if (source[ i + 1] == 'o' && source[ i + 2] == 'n' && source[ i + 3] == 'g') {
                        tokens[token_count].type = TOK_DATATYPE;
                        tokens[token_count].data_type = DATA_TYPE_LONG;
                        i += 4;
                    }
                    break;
                case 'd':
                    if (source[ i + 1] == 'o' && source[ i + 2] == 'u' && source[ i + 3] == 'b' && source[ i + 4] == 'l' && source[ i + 5] == 'e') {
                        tokens[token_count].type = TOK_DATATYPE;
                        tokens[token_count].data_type = DATA_TYPE_DOUBLE;
                        i += 6;
                    }
                break;
            default:
                panic(ERROR_SYNTAX, "unidentified data type at line");
            }
            token_count++;
            break;
        }

        default: {
            printf("Error: Unrecognized character '%c' (ASCII %d) at line %zu, position %zu\n", 
           source[i], source[i], line_number, i);
            panic(ERROR_UNDEFINED, "unidentified token");
        }
    }}

    tokens[token_count].line = line_number;
    tokens[token_count].type = TOK_EOF; // end of file            
    token_count++;
}

static inline int identify_token(const string value, size_t length, token* the_token){
    switch (length){
        case 4:
            if (value[0] == 'e' && value[1] == 'x' && value[2] == 'i' && value[3] == 't') {
                the_token->type = TOK_EXIT;
                return 0;
            }
            if (value[0] == 'v' && value[1] == 'o' && value[2] == 'i' && value[3] == 'd') {
                the_token->type = TOK_VOID;
                return 0;
            }
            break;
            
        case 3:
            if (value[0] == 'l' && value[1] == 'e' && value[2] == 't') {
                the_token->type = TOK_LET;
                return 1;
            }
            break;
            
        case 2:
            if (value[0] == 'f' && value[1] == 'n') {
                the_token->type = TOK_FN;
                return 0;
            }
        break;
    }
    the_token->type = TOK_IDENTIFIER;
    return 0;
}


static Parser* make_parser(Compiler* compiler){
    Parser* parser = arena_alloc(compiler->statements_arena, sizeof(Parser));
    if (!parser)
    {
        panic(ERROR_MEMORY_ALLOCATION, "Parser allocation failed");
    }
    
    parser->tokens = (token*)compiler->token_arena->data;
    parser->current = 0;
    parser->current_line = 1;
    return parser;
}

static inline token* peek(Parser* parser, size_t offset){
    if (parser->current + offset >= token_count){
        fprintf(stderr, "peek surpassed tokens\n");
        return NULL;
    }
    return &parser->tokens[parser->current + offset];
}

static inline token* advance(Parser* parser){
    if (parser-> current >= token_count){
        return NULL;
    }
    return &parser->tokens[parser->current++];
}

static inline bool match(Parser* parser, TokenType type){
    token* current_token = peek(parser, 0);
    return current_token && current_token->type == type;
}

static inline node* parse_statement(Parser* parser){

    printf("checkpoint 2\n");
    int x = peek(parser, 0)->type;
    printf("%i\n", x);
    switch (peek(parser, 0)->type)
    {
    case TOK_EXIT:
        return parse_exit_node(parser);
    
    case TOK_LET:
        return parse_let_node(parser);
    
    case TOK_FN:
        return parse_function_node(parser);
    case TOK_IDENTIFIER:
        // x = 5;
        if (!peek(parser, 1)) break;
        if (peek(parser, 1)->type == TOK_EQUAL)
        {
            printf("parsing assignment\n");
            // assignment
            return parse_assignment_node(parser);
        }
        

    default:
        return(parse_expression(parser, PREC_NONE, true));
    }
}
//////////////////////////////////// NODE CREATORS  ////////////////////////////////////

node* create_number_node(int value)
{
    node* number_node = arena_alloc(compiler->expressions_arena, sizeof(node));
    number_node -> type = NODE_EXPRESSION;
    number_node -> expr = arena_alloc(compiler->expressions_arena, sizeof(expression));
    number_node -> expr -> type = EXPR_INT;
    number_node -> expr -> integer.value = value;
    number_node -> expr -> result_type = DATA_TYPE_INT;
    return number_node;
}


node* create_variable_node_dec(string var_name, size_t length, variable_storage_type storage_type, normal_register reg_location, Data_type data_type)
{
    node* var_node = arena_alloc(compiler->expressions_arena, sizeof(node));
    if(!var_node) return NULL;
    var_node->type = NODE_EXPRESSION;
    var_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression));
    if(!var_node->expr) return NULL;
    var_node->expr->variable.length = length;
    var_node->expr->variable.name = var_name;
    var_node->expr->variable.hash = hash_function(var_name, length);
    var_node->expr->type = EXPR_IDENTIFIER;
    var_node->expr->variable.data_type = data_type;
    var_node->expr->variable.node_in_table = add_var_to_current_scope(var_node->expr, storage_type, reg_location); // offset will be set later during code generation and size will be int for now (8 bytes)
    var_node->expr->result_type = data_type;
    return var_node;
}

node* create_unary_node(TokenType operator, node* operand)
{
    node* unary_node = arena_alloc(compiler->expressions_arena, sizeof(node));
    if(!unary_node) return NULL;
    unary_node->type = NODE_EXPRESSION;
    unary_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression));
    if(!unary_node->expr) return NULL;
    unary_node->expr->type = EXPR_UNARY;
    unary_node->expr->unary.operator = operator;
    unary_node->expr->unary.operand = operand->expr;
    unary_node->expr->result_type = operand->expr->result_type;
    return unary_node;
}

node* create_bin_node(node* left, TokenType operator, Parser* parser, bool constant_foldable)
{
    printf("Making binary node with left type %d and operator %d\n", left->expr->type, operator);
    printf("node left %d\n", left->expr->type);
    node* right_node = parse_expression(parser, presedences[operator], constant_foldable);

    if (!right_node) {
        panic(ERROR_MEMORY_ALLOCATION, "Binary expression allocation failed");
    }

    
    node* bin_node = arena_alloc(compiler->expressions_arena, sizeof(node));
    
    if (!bin_node) {
        panic(ERROR_MEMORY_ALLOCATION, "Binary expression allocation failed");
    }

    
    bin_node->type = NODE_EXPRESSION;
    bin_node-> expr = arena_alloc(compiler->expressions_arena, sizeof(expression));
    
    if (!bin_node->expr) {
        panic(ERROR_MEMORY_ALLOCATION, "Binary expression allocation failed");
    }

    if (left->expr->result_type == DATA_TYPE_DOUBLE || right_node->expr->result_type == DATA_TYPE_DOUBLE) {
        if ((left->expr->result_type == DATA_TYPE_DOUBLE || left->expr->result_type == DATA_TYPE_FLOAT) && (right_node->expr->result_type == DATA_TYPE_DOUBLE || right_node->expr->result_type == DATA_TYPE_FLOAT))
        {
            bin_node->expr->result_type = DATA_TYPE_DOUBLE;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression, one is double and the other is not");
        }
    }
    else if (left->expr->result_type == DATA_TYPE_FLOAT || right_node->expr->result_type == DATA_TYPE_FLOAT) {
        if (left->expr->result_type == DATA_TYPE_FLOAT && right_node->expr->result_type == DATA_TYPE_FLOAT)
        {
            bin_node->expr->result_type = DATA_TYPE_FLOAT;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression, one is float and the other is not");
        }
        
    }
    else if (left->expr->result_type == DATA_TYPE_LONG || right_node->expr->result_type == DATA_TYPE_LONG) {
        if ((left->expr->result_type == DATA_TYPE_LONG || left->expr->result_type == DATA_TYPE_INT) && (right_node->expr->result_type == DATA_TYPE_LONG || right_node->expr->result_type == DATA_TYPE_INT))
        {
            bin_node->expr->result_type = DATA_TYPE_LONG;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression");
        }
    }
    else if (left->expr->result_type == DATA_TYPE_INT || right_node->expr->result_type == DATA_TYPE_INT) {
        if (left->expr->result_type == DATA_TYPE_INT && right_node->expr->result_type == DATA_TYPE_INT)
        {
            bin_node->expr->result_type = DATA_TYPE_LONG;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression");
        }
    }
    else panic(ERROR_TYPE_MISMATCH, "Illegal types in binary expression");
    
    bin_node->expr->type = EXPR_BINARY;
    bin_node->expr->binary.left = left->expr;
    bin_node->expr->binary.right = right_node->expr;
    bin_node->expr->binary.constant_foldable = constant_foldable;

    printf("Creating binary node with operator %d\n", operator);
    switch (operator)
    {
    case TOK_ADD:
        bin_node->expr->binary.operator = TOK_ADD;
        break;
    
    case TOK_SUB:
        bin_node->expr->binary.operator = TOK_SUB;
        break;
    
    case TOK_MUL:
        bin_node->expr->binary.operator = TOK_MUL;
        break;
    
    case TOK_DIV:
        bin_node->expr->binary.operator = TOK_DIV;
        break;

    default:
        fprintf(stderr, "unidentified operator\n");
        panic(ERROR_SYNTAX, "Unknown binary operator");
    }

    return bin_node;
}

node* create_func_call_node(string func_name, size_t name_length, expression* arguments, size_t param_count) 
{
    function_node* func_dec_node = find_function_symbol_node(func_name, name_length);
    if(func_dec_node->param_count != param_count) panic(ERROR_ARGUMENT_COUNT, "Wrong number of arguments in function call");
    for (size_t i = 0; i < param_count; i++)
    {
        if(arguments[i].result_type != func_dec_node->parameters[i].variable.data_type) panic(ERROR_TYPE_MISMATCH, "Function argument type mismatch");
    }
    

    node* func_call_node = arena_alloc(compiler->expressions_arena, sizeof(node));
    if(!func_call_node) return NULL;
    func_call_node->type = NODE_EXPRESSION;
    func_call_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression));
    if(!func_call_node->expr) return NULL;
    func_call_node->expr->type = EXPR_FUNCTION_CALL;
    func_call_node->expr->func_call.name = func_name;
    func_call_node->expr->func_call.name_length = name_length;
    func_call_node->expr->func_call.arguments = arguments;
    func_call_node->expr->func_call.parameter_count = param_count;
    func_call_node->expr->result_type = func_dec_node->return_type;
    return func_call_node;

}


int evaluate_unary(expression* unary_exp)
{
    switch (unary_exp->unary.operator)
    {
    case TOK_SUB:
        evaluate_expression(unary_exp->unary.operand);
        write_to_buffer("neg rax\n", 8, output);
        break;
    
    default:
        return 0;
    }
}

int evaluate_bin(expression* binary_exp)
{
    evaluate_expression(binary_exp->binary.left); //left value at rsp
    write_to_buffer("\npush rax\n", 10, output);


    evaluate_expression(binary_exp->binary.right); // right value at rax
    switch (binary_exp->binary.operator)
    {
    case TOK_ADD:
        write_to_buffer("\npop rbx\nadd rax, rbx\n", 22, output);
        break;
    case TOK_SUB:
        write_to_buffer("\npop rbx\nxchg rax, rbx\nsub rax, rbx\n", 35, output);
        break;
    case TOK_MUL:
        write_to_buffer("\npop rbx\nmul rbx\n", 17, output);
        break;
    case TOK_DIV:
        write_to_buffer("\npop rbx\nxchg rax, rbx\ndiv rax, rbx\n", 35, output);
        break;

    default:
        return 0;
    }
    
}


int constant_folding(expression* expr)
{

}

void evaluate_expression(expression* expr)
{
    size_t num_len;
    printf("Evaluating expression of type %d\n", expr->type);

    switch (expr->type)
    {
    
    case EXPR_INT:{
    
        write_to_buffer("mov rax, ", 9, output);

        if (expr->integer.value < 0)
        {
            write_to_buffer("-", 1, output);
            write_to_buffer(_u64_to_str(-(expr->integer.value), &num_len), num_len, output);
        }
        else
        {
            string temp = _u64_to_str(expr->integer.value, &num_len);
            write_to_buffer(temp, num_len, output);
        }
        return;
    }

    case EXPR_IDENTIFIER:{
        size_t temp;

        symbol_node* var_node = expr->variable.node_in_table;
        printf("_______________________________%i\n", var_node->where_it_is_stored);
        if (var_node->where_it_is_stored == STORE_IN_STACK)
        {
            write_to_buffer("mov rax, [rbp - ", 16, output);
            nums_to_str(var_node->offset, &temp, output);
            write_to_buffer("]\n", 2, output);
        }
        else if (var_node->where_it_is_stored == STORE_AS_PARAM)
        {
            write_to_buffer("mov rax, [rbp + ", 16, output);
            nums_to_str(var_node->offset, &temp, output);
            write_to_buffer("]\n", 2, output);
        }
        else if (var_node->where_it_is_stored == STORE_IN_REGISTER)
        {
            write_to_buffer("mov rax, ", 9, output);
            string reg_name = reg[var_node->register_location];
            write_to_buffer(reg_name, strlen(reg_name), output);
            write_to_buffer("\n", 1, output);
        }

        printf("Variable name: %.*s\n", (int)expr->variable.length, expr->variable.name);
        printf("Variable offset string: %lu\n", temp);
        printf("done");
        return;
    }
    case EXPR_BINARY:
        evaluate_bin(expr);
        break;

    case EXPR_UNARY:
        evaluate_unary(expr);
        break;
    
    case EXPR_FUNCTION_CALL: {
        size_t param_count = expr->func_call.parameter_count;
        write_to_buffer("push r9\npush r8\npush rcx\npush rdx\npush rsi\npush rdi\n", 52, output);
        
        if (expr->func_call.parameter_count <= 6) {
            for (size_t i = 0; i < param_count; i++)
            {
                evaluate_expression(&(expr->func_call.arguments[i]));
                write_to_buffer("mov ", 4, output);
                write_to_buffer(reg[i], 3, output);
                write_to_buffer(", rax\n", 6, output);
                
            }
            
        }
        else {
            size_t i = 0;
            while (i <= 6)
            {
                evaluate_expression(&(expr->func_call.arguments[i]));
                write_to_buffer("mov ", 4, output);
                write_to_buffer(reg[i], 3, output);
                write_to_buffer(", rax\n", 6, output);
                i++;
            }
            for (size_t j = param_count - 1; j > 6; j--)
            {
                evaluate_expression(&(expr->func_call.arguments[i]));
                write_to_buffer("push rax\n", 9, output);
            }
            
            
        }
        write_to_buffer("call ", 5, output);
        write_to_buffer(expr->func_call.name, expr->func_call.name_length, output);
        write_to_buffer("\npop rdi\npop rsi\npop rdx\npop rcx\npop r8\npop r9\n", 47, output);
        break;
    }
    default:
        panic(ERROR_UNDEFINED, "Unexpected token, may still not be emplemented");
    }
}

node* create_variable_node(string var_name, size_t length, variable_storage_type storage_type, normal_register reg_location, Data_type data_type)
{
    node* var_node = arena_alloc(compiler->expressions_arena, sizeof(node));
    if(!var_node) panic(ERROR_MEMORY_ALLOCATION, "Variable node allocation failed");
    var_node->type = NODE_EXPRESSION;
    printf("Creating variable node for %.*s\n", (int)length, var_name);
    var_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression));
    if(!var_node->expr) panic(ERROR_MEMORY_ALLOCATION, "Variable node allocation failed");
    printf("checkpoint\n");
    var_node->expr->variable.length = length;
    var_node->expr->variable.name = var_name;
    var_node->expr->type = EXPR_IDENTIFIER;
    var_node->expr->variable.hash = hash_function(var_name, length);
    printf("checkpoint2\n");
    var_node->expr->variable.node_in_table = find_variable(var_node->expr->variable.hash, var_name, length);
    printf("checkpoint 4\n");
    return var_node;
}

node* parse_prefix(Parser* parser, bool constant_foldable)
{
    token* current_token = peek(parser, 0);
    if(!current_token) return NULL;
    printf("Parsing prefix token of type %d\n", current_token->type);
    switch (current_token->type)
    {
        case TOK_NUMBER:
        {
            node* num_node = create_number_node(current_token->int_value);
            advance(parser); // consume number token
            return num_node;
        }

        case TOK_IDENTIFIER:
            printf("Parsing identifier: %.*s\n", (int)current_token->str_value.length, current_token->str_value.starting_value);
            constant_foldable = false; // look into the token after the identifier to see if it's a function call or variable
            advance(parser); // consume identifier token
            token* next_tok = peek(parser, 0); // consume identifier token
            if (next_tok->type == TOK_EOF) panic(ERROR_SYNTAX, "Unexpected end of file after identifier");
            ////////////////////// FUNC CALL   //////////////////////////////
            if (next_tok->type == TOK_LPAREN)
            {
                advance(parser); // consume '('
                // Handle empty arguments: f() or f(void)
                if (peek(parser, 0)->type == TOK_RPAREN || 
                    (peek(parser, 0)->type == TOK_VOID && 
                    peek(parser, 1)->type == TOK_RPAREN)) {
                    
                    if (peek(parser, 0)->type == TOK_VOID) {
                        advance(parser); // consume 'void'
                    }
                    advance(parser); // consume ')'
                    
                    return create_func_call_node(current_token->str_value.starting_value, current_token->str_value.length, NULL, 0);
                }
                else { // normal case
                    size_t param_count = 0;

                    // first pass
                    size_t tmp = 0;
                    while (peek(parser, tmp)->type != TOK_RPAREN)
                    {
                        if(peek(parser, tmp)->type == TOK_COMMA) param_count++;
                        tmp++;
                    }
                    param_count++;

                    // second pass
                    expression* expressions = arena_alloc(compiler->expressions_arena, param_count * sizeof(expression));
                    for (size_t i = 0; i < param_count; i++)
                    {
                        expressions[i] = *(parse_expression(parser, PREC_NONE, false)->expr);
                        advance(parser); // consume comma and final R_BRACE
                    }
                    return create_func_call_node(current_token->str_value.starting_value, current_token->str_value.length, expressions, param_count);
                    

                    // second pass
                }
                
            }
            else {
                symbol_node* var = find_variable(hash_function(current_token->str_value.starting_value, current_token->str_value.length), current_token->str_value.starting_value, current_token->str_value.length);
                printf("\nFound variable %.*s at offset %lu\n", (int)current_token->str_value.length, current_token->str_value.starting_value, var->offset);
                if (!var)
                {
                    panic(ERROR_UNDEFINED_VARIABLE, "Variable used before declaration");
                }
                node* var_node = create_variable_node(var->var_name, var->var_name_size, var->where_it_is_stored, var->register_location, var->data_type);
                printf("Created variable node for %.*s\n node type %d\n", (int)current_token->str_value.length, current_token->str_value.starting_value, var_node->expr->type);
                return var_node;
            }
        case TOK_SUB: {  // Unary minus
            advance(parser);
            node* operand = parse_expression(parser, PREC_UNARY, true);
            node* sub_node = create_unary_node(TOK_SUB, operand);
            
            return sub_node;
        }

        case TOK_LPAREN: { // Braces must be used in C if you will declare a variable inside the case
            advance(parser);
            node* expr = parse_expression(parser, PREC_NONE, true);
            if (peek(parser, 0)->type != TOK_RPAREN)
            {
                panic(ERROR_SYNTAX, "Expected ')' ");
            }
            advance(parser);
            return expr;
        }

        default:
        panic(ERROR_UNDEFINED, "Unexpected token/s, may still not be emplemented");
    }

    
}



// 5 + 2 * 3 + x
node* parse_expression(Parser* parser, int prev_presedence, bool constant_foldable) {
    constant_foldable = true;
    printf("Parsing expression with previous precedence %d\n", prev_presedence);
    printf("Current token type: %d\n", peek(parser, 0)->type);
    node* left = parse_prefix(parser, constant_foldable); //after that the parser->current will be at the operator after the number/prefix token(s)
    TokenType curr_opperator = peek(parser, 0) -> type;
    int curr_presedence = presedences[curr_opperator];

    while (true)
    {
        if (curr_presedence <= prev_presedence)
        {
            printf("Breaking from expression parsing loop\n");
            break;
        }
        else
        {
            advance(parser); //now parser is on what is after the operator
            left = create_bin_node(left, curr_opperator, parser, constant_foldable); //now parser is on what was seen as a weak node (like the second plus on 5 + 2 * 3 + 1 )
            curr_opperator = peek(parser, 0) -> type;
            curr_presedence = presedences[curr_opperator];
        }
    }
    return left;
}
///////////////////////////////////////////////////      FUNCTION FUNCTIONS

///////////////////////////////////////////////////       PARSING NODES           ///////////////////////////////////////////////////////////////////////////

static inline node* parse_exit_node(Parser* parser){
    node* exit_node = (node*)arena_alloc(compiler->statements_arena, sizeof(node));
    if(!exit_node) panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed");

    exit_node->type = NODE_STATEMENT;
    advance(parser); // consume 'exit'

    exit_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    exit_node->stmnt->type = STMT_EXIT;
    printf("\n\n\nparsing exit expression\n\n\n");
    exit_node->stmnt->stmnt_exit.exit_code = parse_expression(parser, presedences[TOK_EXIT], true)->expr;

    if (peek(parser, 0)->type != TOK_SEMICOLON) panic(ERROR_SYNTAX, "Expected semicolumn ';'");
    advance(parser); // consume ;

    return exit_node;
}

static inline node* parse_let_node(Parser* parser){
    printf("parsing let node\n");
    node* let_node = (node*)arena_alloc(compiler->statements_arena, sizeof(node));
    if(!let_node) panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed");

    let_node->type = NODE_STATEMENT;
    advance(parser); //consume let

    let_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    if(!let_node->stmnt) panic(ERROR_MEMORY_ALLOCATION, "Exit node statement allocation failed");

    let_node->stmnt->type = STMT_LET;

    token* identifier_token = peek(parser, 0);

    let_node->stmnt->stmnt_let.name = identifier_token->str_value.starting_value;
    let_node->stmnt->stmnt_let.name_length = identifier_token->str_value.length;

    //we need this info: add_var_to_current_scope(expression* variable, variable_storage_type storage_type, normal_register reg_location)
    // and this info:   node* create_variable_node_dec(string var_name ✅, size_t length ✅ , variable_storage_type storage_type ✅ , normal_register reg_location ✅ , Data_type data_type)
    printf("hash: %lu\n", hash_function(peek(parser, 0)->str_value.starting_value, peek(parser, 0)->str_value.length));
    let_node->stmnt->stmnt_let.hash = hash_function(peek(parser, 0)->str_value.starting_value, peek(parser, 0)->str_value.length);
    advance(parser); //consume identifier

    token* data_type_token = advance(parser); //consume data type

    if (data_type_token->type != TOK_DATATYPE) panic(ERROR_ARGUMENT_COUNT, "undefined type");
    
    create_variable_node_dec(identifier_token->str_value.starting_value, identifier_token->str_value.length, STORE_IN_STACK, 0, data_type_token->data_type);


    token* t = advance(parser); // consume equal
    if (t->type != TOK_EQUAL)
    {
        printf(" the token was: %i", t->type);
        panic(ERROR_SYNTAX, "usage: let var = value;");
    }
    printf("\n\n\nparsing let expression\n\n\n");
    let_node->stmnt->stmnt_let.value = parse_expression(parser, presedences[TOK_EQUAL], true)->expr;
    advance(parser); // consume ;
    return let_node;
}

static inline node* parse_assignment_node(Parser* parser){
    node* assignment_node = (node*)arena_alloc(compiler->statements_arena, sizeof(node));
    if(!assignment_node) panic(ERROR_MEMORY_ALLOCATION, "Assignment node allocation failed");

    assignment_node->type = NODE_STATEMENT;
    token* identifier_token = peek(parser, 0);
    size_t hash = hash_function(identifier_token->str_value.starting_value, identifier_token->str_value.length);

    advance(parser); //consume identifier
    advance(parser); //consume equal

    assignment_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    assignment_node->stmnt->type = STMT_ASSIGNMENT;

    assignment_node->stmnt->stmnt_assign.hash = hash;
    printf("\n\n\nparsing assignment expression\n\n\n");
    assignment_node->stmnt->stmnt_assign.value = parse_expression(parser, presedences[TOK_EQUAL], true)->expr;
    printf("\n\n\nparsing assignment expression\n\n\n");
    assignment_node->stmnt->stmnt_assign.name = identifier_token->str_value.starting_value;
    assignment_node->stmnt->stmnt_assign.name_length = identifier_token->str_value.length;

    if (peek(parser, 0)->type != TOK_SEMICOLON) panic(ERROR_SYNTAX, "Expected semicolumn ';'");
    advance(parser); // consume ;
    return assignment_node;
}

static inline node* parse_code_block(Parser* parser, bool function_block, expression* params, size_t param_count) {
    if(advance(parser)->type != TOK_LBRACE) panic(ERROR_SYNTAX, "Must start code block using '{'");
    node* block_node = arena_alloc(compiler->statements_arena, sizeof(node));
    if(!block_node) panic(ERROR_MEMORY_ALLOCATION, "Code block node allocation failed");
    printf("M\n");
    block_node->type = NODE_STATEMENT;
    block_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    if(!block_node->stmnt) panic(ERROR_MEMORY_ALLOCATION, "Code block statement allocation failed");
    block_node->stmnt->type = STMT_BLOCK;

    // Save current position
    size_t start_position = parser->current;
    printf("S\n");
    // Fast forward to find matching RBRACE and count statements
    size_t brace_depth = 1;
    size_t stmt_count = 0;
    size_t i = 0;
    printf("A\n");
    while (brace_depth > 0) {
        token* tok = peek(parser, i);
        if (!tok) panic(ERROR_SYNTAX, "Unclosed brace");
        
        if (tok->type == TOK_LBRACE) {
            brace_depth++;
        } else if (tok->type == TOK_RBRACE) {
            brace_depth--;
        } else if (brace_depth == 1) {
            // Only count statements at our depth
            // Heuristic: count statement starters
            if (tok->type == TOK_LET || 
                tok->type == TOK_EXIT || 
                tok->type == TOK_RETURN ||
                tok->type == TOK_IF ||
                tok->type == TOK_WHILE ||
                tok->type == TOK_FOR ||
                tok->type == TOK_IDENTIFIER) {
                stmt_count++;
            }
        }
        i++;
    }
    printf("L\n");
    // Reset parser position
    parser->current = start_position;
    printf("P\n");
    
    block_node->stmnt->stmnt_block.statements = (statement**)arena_alloc(compiler->statements_arena, sizeof(statement*) * stmt_count);
    printf("O\n");
    size_t statement_count = 0;
    enter_new_scope();
    block_node->stmnt->stmnt_block.table = peek_symbol_stack();
    if(function_block) block_node->stmnt->stmnt_block.table->parent_scope = compiler->symbol_table_stack->storage[0];

    if (param_count <= 6)
    {
        for (int i = 0; i < param_count; i++)
        {
            add_var_to_current_scope(&(params[i]), STORE_IN_REGISTER, registers[i]);
        }
        
    }
    else {
        for (int i = 0; i < 6; i++)
        {
            add_var_to_current_scope(&(params[i]), STORE_IN_REGISTER, registers[i]);
        }
        for (size_t i = param_count - 1; i >= 6; i--)
        {
            add_var_to_current_scope(&(params[i]), STORE_AS_PARAM, 0);
        }
        
    }

    while (peek(parser, 0)->type != TOK_RBRACE)
    {
        node* statement = NULL;
        printf("\n\n\n\n------> Parsing statement in block, current token type: %d\n", peek(parser, 0)->type);
        switch (peek(parser, 0)->type)
        {
            case TOK_EXIT:
                statement = parse_exit_node(parser);
                break;

            case TOK_LET:
                statement = parse_let_node(parser);
                break;

            case TOK_IDENTIFIER: {
                // x = 5;
                if (!peek(parser, 1)) break;
                if (peek(parser, 1)->type == TOK_EQUAL)
                {
                    printf("parsing assignment\n");
                    // assignment
                    statement = parse_assignment_node(parser);
                }
                break;
            }
            default:
                statement = parse_expression(parser, PREC_NONE, true);
                break;
        }

        if(!statement) continue;
        block_node->stmnt->stmnt_block.statements[statement_count] = statement->stmnt;
        statement_count++;
    }

    block_node->stmnt->stmnt_block.statement_count = statement_count;
    advance(parser); // consume }
    exit_current_scope();
    return block_node;
}


// function parser
/*
fn add (param:int, param:long):int {
    //code
}
*/
static inline node* parse_function_node(Parser* parser)
{
    node* func_stmt = arena_alloc(compiler->statements_arena, sizeof(node));
    if(!func_stmt) panic(ERROR_MEMORY_ALLOCATION, "function decleration node allocation failed");

    func_stmt->type = NODE_STATEMENT;

    func_stmt->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    if(!func_stmt) panic(ERROR_MEMORY_ALLOCATION, "function decleration node allocation failed");
    func_stmt->stmnt->type = STMT_FUNCTION;
    advance(parser); //consume fn
    token* name_tok = advance(parser); // consume function name
    if (name_tok->type != TOK_IDENTIFIER) panic(ERROR_SYNTAX, "Syntax error, use case fn function_name (parameters) {\n----->code\n}                         ~~~~~~~~~~~~~");
    func_stmt->stmnt->stmnt_function_declaration.name = name_tok->str_value.starting_value;
    func_stmt->stmnt->stmnt_function_declaration.name_length = name_tok->str_value.length;
    func_stmt->stmnt->stmnt_function_declaration.index = hash_function(name_tok->str_value.starting_value, name_tok->str_value.length);
    
    func_stmt->stmnt->stmnt_function_declaration.function_node = arena_alloc(compiler->symbol_arena, sizeof(function_node));
    if(!func_stmt->stmnt->stmnt_function_declaration.function_node) panic(ERROR_MEMORY_ALLOCATION, "function decleration node allocation failed");
    func_stmt->stmnt->stmnt_function_declaration.function_node->name = name_tok->str_value.starting_value;
    func_stmt->stmnt->stmnt_function_declaration.function_node->name_length = name_tok->str_value.length;

    uint32_t param_count = 0;
    printf("roblox\n");
    // split to case void and case parameters
    if (peek(parser, 0)->type != TOK_LPAREN) panic(ERROR_SYNTAX, "fn ( parameters ) { code }\n   ~\n");
    advance(parser); // consume (
    if (peek(parser, 0)->type == TOK_VOID && peek(parser, 1)->type == TOK_RPAREN)
    {
        printf("ro\n");
        advance(parser); // consume void
        advance(parser); // consume )
        func_stmt->stmnt->stmnt_function_declaration.function_node->param_count = 0;
        func_stmt->stmnt->stmnt_function_declaration.function_node->parameters = NULL;
        // now at the return type
        printf("blox\n");
    }
    else
    {
        // find number of parameters and check syntax
        uint32_t tmp = 0;
        while (true)
        {
            // check if dev didn't close the paren
            if(peek(parser, tmp)->type == TOK_EOF) panic(ERROR_SYNTAX, "fn function_name (parameters')'\n                             ~\n");

            if (peek(parser, tmp)->type != TOK_IDENTIFIER) panic(ERROR_SYNTAX, "function parameter name not specified");
            tmp++;
            if (peek(parser, tmp)->type != TOK_DATATYPE) panic(ERROR_SYNTAX, "function parameter data type not specified");
            tmp++;
            param_count++;
            if (peek(parser, tmp)->type == TOK_COMMA) tmp++;
            else if (peek(parser, tmp)->type == TOK_RPAREN) break;
            else panic(ERROR_SYNTAX, "after parameter either ')' or ',' only");
        }

        // populate function node info
        func_stmt->stmnt->stmnt_function_declaration.function_node->param_count = param_count;
        func_stmt->stmnt->stmnt_function_declaration.function_node->parameters = arena_alloc(compiler->symbol_arena, sizeof(expression)*param_count);
        if(!func_stmt->stmnt->stmnt_function_declaration.function_node->parameters) panic(ERROR_MEMORY_ALLOCATION, "function parameters allocation failed");
        
        // loop through them and store their name, name len, data type
        for (size_t i = 0; i < param_count; i++)
        {
            token* name = advance(parser); // consume param name
            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.name = name->str_value.starting_value;
            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.length = name->str_value.length;
            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.hash = hash_function(name->str_value.starting_value, name->str_value.length);
            token* data_type = advance(parser); // consume param data type
            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.data_type = data_type->data_type;
            advance(parser); // consume , or )  it doesn't rly matter
        }
    }
    // now we are at the return type
    printf("Mbapee\n");
    if (peek(parser, 0)->type != TOK_DATATYPE) panic(ERROR_SYNTAX, "expected return value data type");
    func_stmt->stmnt->stmnt_function_declaration.function_node->return_type = advance(parser)->data_type;
    // consumed return type
    // now at {
    printf("ronaldo\n");
    func_stmt->stmnt->stmnt_function_declaration.function_node->code_block = parse_code_block(parser, true, func_stmt->stmnt->stmnt_function_declaration.function_node->parameters, func_stmt->stmnt->stmnt_function_declaration.function_node->param_count)->stmnt;
    printf("R9\n");
    func_stmt->stmnt->stmnt_function_declaration.function_node->next = NULL;
    append_function_to_func_map(func_stmt->stmnt->stmnt_function_declaration.function_node);
    return func_stmt;
}   


char num_buffer[32];

void write_to_buffer(string code, size_t code_length, FILE* output){
    if (code_length + compiler->currentsize >= compiler->capacity)
    {
        //flush buffer to file
        fwrite(compiler->buffer, 1, compiler->currentsize, output);
        compiler->currentsize = 0;
    }
    printf("----->%s\n", code);
    memcpy(compiler->buffer + compiler->currentsize, code, code_length);
    compiler->currentsize += code_length;
}



static inline string _u64_to_str(size_t number, size_t* num_len)
{
    // num buffer is 32 bytes long to hold max u64 value plus null terminator
    string end = &num_buffer[31];
    *num_len = 0;
    *end = '\0';
    end--;
    
    do {
        *end = '0' + (number % 10);
        number /= 10;
        end--;
        (*num_len)++;
    } while (number);
    return end + 1;
}


static inline void nums_to_str(size_t number, size_t* num_len, FILE* output)
{
    string num_as_str = _u64_to_str(number, num_len);
    write_to_buffer(num_as_str, *num_len, output);
}

static inline void enter_existing_scope(symbol_table* new_table, bool independent) {
    if (!compiler) panic(ERROR_UNDEFINED, "Compiler not initialized");
    if (compiler->symbol_table_stack->current_size + 1 >= compiler->symbol_table_stack->capacity) {
        compiler->symbol_table_stack->storage = realloc(compiler->symbol_table_stack->storage, compiler->symbol_table_stack->capacity * 2 * sizeof(symbol_table*));
        compiler->symbol_table_stack->capacity *= 2;
    }
    if (!compiler->symbol_table_stack->storage) panic(ERROR_MEMORY_ALLOCATION, "Entered too many scopes for your memory");

    if (!new_table) panic(ERROR_MEMORY_ALLOCATION, "New symbol table allocation failed");
    
    if (independent) new_table->parent_scope = compiler->symbol_table_stack->storage[0];
    else new_table->parent_scope = peek_symbol_stack();
    compiler->symbol_table_stack->storage[compiler->symbol_table_stack->current_size] = new_table;
    compiler->symbol_table_stack->current_size++;
}

void generate_function_code(statement* stmt, size_t* num_len) {
    printf("here\n");
    function_node* func_node = stmt->stmnt_function_declaration.function_node;
    printf("here 2\n");
    write_to_buffer(func_node->name, func_node->name_length, output);
    printf("here 3\n");
    write_to_buffer(":\n", 2, output);
    write_to_buffer("push rbp\nmov rbp, rsp\n", 22, output);
    
    enter_existing_scope(func_node->code_block->stmnt_block.table, true);

    write_to_buffer("sub rsp, ", 9, output);
    nums_to_str(func_node->code_block->stmnt_block.table->scope_offset, num_len, output);
    write_to_buffer("\n", 1, output);
    
    
    for (size_t i = 0; i < func_node->code_block->stmnt_block.statement_count; i++)
    {
        generate_statement_code(func_node->code_block->stmnt_block.statements[i], num_len);
    }
    write_to_buffer("ret\n", 4, output);
    exit_current_scope();
}

void generate_statement_code(statement* stmt, size_t* num_len) {
    printf("arrived\n");
    switch (stmt->type)
    {
    case STMT_EXIT:
        {
            // printf("exit code type: %d\n", stmt->stmnt_exit.exit_code->type);
            evaluate_expression(stmt->stmnt_exit.exit_code);
            write_to_buffer("\nmov rdi, rax\nmov rax, 60", 25, output);
            write_to_buffer("\nsyscall\n", 9, output);
            break;
        }
    
    case STMT_LET:
        {
            printf("broke here\n");
            evaluate_expression(stmt->stmnt_let.value);
            write_to_buffer("\n", 1, output);
            write_to_buffer("mov [rbp - ", 11, output);
            nums_to_str(find_variable(stmt->stmnt_let.hash, stmt->stmnt_let.name, stmt->stmnt_let.name_length)->offset, num_len, output);
            write_to_buffer("], rax\n", 7, output);
            break;
        }
    case STMT_ASSIGNMENT:
        {
            evaluate_expression(stmt->stmnt_assign.value); // now the expression is in rax
            symbol_node* var_node = find_variable(stmt->stmnt_assign.hash, stmt->stmnt_assign.name, stmt->stmnt_assign.name_length);
            if (!var_node) panic(ERROR_UNDEFINED, "Variable not declared before assignment");
            if (var_node->where_it_is_stored == STORE_IN_STACK)
            {
                write_to_buffer("\nmov [rbp - ", 12, output);
                nums_to_str(find_variable(stmt->stmnt_assign.hash, stmt->stmnt_assign.name, stmt->stmnt_assign.name_length)->offset, num_len, output);
                write_to_buffer("], rax\n", 7, output);
            }
            else if (var_node->where_it_is_stored == STORE_IN_REGISTER)
            {
                write_to_buffer("mov ", 4, output);
                write_to_buffer(reg[var_node->register_location], 3, output);
                write_to_buffer(", rax\n", 6, output);
            }
            else if (var_node->where_it_is_stored == STORE_IN_FLOAT_REGISTER)
            {
                panic(ERROR_UNDEFINED, "floats still not implemented");
            }
            else{
                panic(ERROR_UNDEFINED, "where is the var stored?");
            }
            break;
        }
    default:
        break;
    }  
}

void generate_assembly(const AST* AST){
    output = fopen("output.asm", "wb"); //clear file
    size_t num_len = 0;
    size_t i = 0;
    write_to_buffer("section .text\n\tglobal _start\n_start:\n", 37, output);
    // will move to function handling later
    write_to_buffer("push rbp\nmov rbp, rsp\nsub rsp, ", 31, output);
    nums_to_str(compiler->symbol_table_stack->storage[0]->scope_offset, &num_len, output);
    write_to_buffer("\n", 1, output);

    while (i < AST->node_count)
    {
        switch (AST->nodes[i]->type)    
        {
        
        case NODE_STATEMENT:
            generate_statement_code(AST->nodes[i]->stmnt, &num_len);
            break;
        case NODE_EXPRESSION:
            // This handles top-level expressions like "my_function();"
            evaluate_expression(AST->nodes[i]->expr);
            break;
        default:
            break;
        }
        i++;
    }
    i = 0;
    printf("----------------------------------------------------------\n");
    write_to_buffer("\n\n\n\n", 4, output);
    while (i < AST->node_count)
    {
        // Check if the node is actually a statement first
    if (AST->nodes[i]->type == NODE_STATEMENT) 
    {
        // USE ->stmnt, NOT ->expr
        switch (AST->nodes[i]->stmnt->type) 
        {
        case STMT_FUNCTION: // Use the correct enum (was STMT_FUNCTION in struct, make sure enums match)
            printf("Generating function code for statement node %lu\n", i);
            generate_function_code(AST->nodes[i]->stmnt, &num_len);
            break;
        default:
            break;
        }
    }
    i++;
    }


    //flush remaining buffer to file
    write_to_buffer("\n; Exit program\nmov rax, 60\nmov rdi, 0\nsyscall", 46, output);

    write_to_buffer("\nsection .rodata\nerr_div0:\tdb \"division by zero\", 10\nerr_div0_len:  equ $ - err_div0\n", 85, output);
    fwrite(compiler->buffer, 1, compiler->currentsize, output);
    fclose(output);
    //gcc phc.c -o phc && ./phc test.ph
    //nasm -f elf64 output.asm && ld output.o -o output && ./output
}