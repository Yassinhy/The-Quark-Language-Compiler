#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

typedef char* string;
#define upper 4
FILE* output = NULL;
size_t glbl_offset = 0;
size_t semicolmn_count = 0;


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

// token structure
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

typedef struct expression
{
    ExpressionType type; //the type of the expression
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
            size_t index;
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
            struct expression*  calle;
            struct expression** arguments;
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
            expression** parameters; //parameters could be any expression 
            size_t param_count;
            struct statement* code_block;
            size_t hash;

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
size_t default_capacity = 1024 * 1024 * 4;

// Arena
typedef struct {
    size_t capacity;
    size_t current_size;
    uint8_t* data;
} Arena;


typedef struct hash_node{
    string var_name;
    size_t var_name_length;
    uint32_t offset;
    uint32_t size;
    struct hash_node* next;
} hash_node;

typedef struct
{
    hash_node** hash_map;
    uint32_t capacity;
    struct SymbolTable* parent_scope; 
    uint32_t current_offset;
} symbol_table;

#define DEFAULT_SCOPE_CAPACITY 64
symbol_table* peek_stack(void){
    return compiler->current_variable_hashtable->storage[compiler->current_variable_hashtable->current_size];
}
symbol_table* create_symbol_table() {
    symbol_table* new_table = arena_alloc(compiler->symbol_arena, sizeof(symbol_table));
    new_table->capacity = DEFAULT_SCOPE_CAPACITY;
    new_table->parent_scope = peek_stack();
    new_table->current_offset = 0;
    new_table->hash_map = create_hash_table(DEFAULT_SCOPE_CAPACITY);
    return new_table;
}

// stack made for var hash table
typedef struct
{
    uint32_t capacity;
    uint32_t current_size;
    symbol_table** storage;
}stack;


Arena* initialize_arena(size_t capacity);
void free_arena(Arena* arena);

typedef struct {
    Arena* token_arena;     // For tokens and lexer data
    Arena* statements_arena;       // For AST nodes and parser data  
    Arena* expressions_arena;    // For string literals and identifiers
    Arena* symbol_arena;    // For symbol table (if you keep it)
    Arena* sandbox_arena;

    // Stack
    stack* current_variable_hashtable;

    // Buffer
    char buffer[16 * 1024];
    size_t capacity;
    size_t currentsize;
} Compiler;

stack* initialize_stack(uint32_t size) {
    stack* stack = malloc(sizeof(stack));
    stack->capacity = size;
    stack->current_size = 0;
    stack->storage = arena_alloc(compiler->symbol_arena, size);
    stack->storage[stack->current_size++] = NULL;

    return stack;
}

Compiler* init_compiler_arenas(size_t file_length) {
    Compiler* arenas = malloc(sizeof(Compiler));
    arenas->token_arena = initialize_arena((file_length + 1) * token_size);
    arenas->statements_arena = initialize_arena(file_length * (expression_size + node_size));
    arenas->expressions_arena = initialize_arena(file_length * expression_size);
    arenas->symbol_arena = initialize_arena(default_capacity);
    arenas->sandbox_arena = initialize_arena(semicolmn_count * 8 + 512); // extra 512 bytes for safety
    arenas->current_variable_hashtable = initialize_stack(1024 * 1024 * 8);

    arenas->capacity = 16 * 1024;
    arenas->currentsize = 0;
    return arenas;
}

void free_global_arenas(Compiler* arenas) {
    if (arenas->token_arena) free_arena(arenas->token_arena);
    if (arenas->statements_arena) free_arena(arenas->statements_arena);
    if (arenas->expressions_arena) free_arena(arenas->expressions_arena);
    if(arenas->symbol_arena) free_arena(arenas->symbol_arena);
    if(arenas->sandbox_arena) free_arena(arenas->sandbox_arena);
    if (arenas->current_variable_hashtable) free(arenas->current_variable_hashtable);
    
    free(arenas);
}




typedef struct function_symbol_node {
    statement* function_declaration;
    uint32_t total_offset; // sub rsp, total_offset
    uint32_t total_var_count;
    uint32_t hash;

    hash_node** local_symbol_table;
    struct function_symbol_node* next;

} function_symbol_node;

hash_node** create_hash_table(size_t size);
function_symbol_node** create_function_hash_table(size_t size);



size_t amount_of_global_variables = 0;
function_symbol_node** function_symbol_table = NULL;

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

AST* parse(Parser* parser);
node* parse_prefix(Parser* parser, bool constant_foldable);

//node generation
node* create_number_node(int value);
node* make_bin_node(node* left, TokenType operator, Parser* parser, bool constant_foldable);

//File handling
static string readfile(const string filename);

// Tokenizer functions
static void tokenize(const string source, Compiler* compiler);
static inline int identify_token(const string value, size_t length, token* the_token);

// Code generation
void write_to_buffer(string code, size_t code_length, FILE* output);
void generate_assembly(const AST* AST);
static inline string _u64_to_str(size_t number, size_t* num_len);

//evaluation functions
int evaluate_bin(expression* binary_exp);
void evaluate_expression(expression* expr);

// Free functions
static inline void free_ast(AST* ast);
static inline void free_parser(Parser* parser);

static void panic(error_code error_code, string message);


// Arena functions
//Arena* initialize_arena(size_t capacity);
bool arena_realloc(Arena* arena, size_t data_size);
void* arena_alloc(Arena* arena, size_t data_size);
void arena_reset(Arena* arena);
//void free_arena(Arena* arena);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]){

    clock_t start = clock();


    if (argc != 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    //read file
    string source = readfile(argv[1]);

    //initialize compiler arenas
    compiler = init_compiler_arenas(file_length);

    //tokenize
    tokenize(source, compiler);  // This sets token_count
    glbl_tokens = (token*)compiler->token_arena->data;
    if (token_count == 0) {
        panic(ERROR_INTERNAL, "ERROR: No tokens created! Check your tokenizer.");
    }

    //initialize symbol tables
    function_symbol_table = create_function_hash_table(32);
    size_t table_size = (amount_of_global_variables > 0) ? amount_of_global_variables * 2 : 32;
    amount_of_global_variables = table_size;

    //create parser
    Parser* parser = make_parser(compiler);
    glbl_parser = parser;


    //parse tokens into AST
    AST* ast = malloc(sizeof(AST));
    glbl_ast = ast;
    ast->nodes = malloc(sizeof(node*) * token_count);  // Allocate pointer array
    ast->node_count = 0;
    ast->parser = parser;
    size_t i = 0;
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
    
    
    free_ast(ast);
    free(source);
    
    
    clock_t end = clock();
    system("nasm -f elf64 output.asm && ld output.o -o output");
    double time_spent = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Code executed in %.6f seconds\n", time_spent);
    
    return 0;
}

//////////////////////////////////////////////////////    STACK FUNCTIONS    //////////////////////////////////////////////////////////////////////////

void push(stack* stack, hash_node** hash_table) {
    if(!stack) panic(ERROR_UNDEFINED, "scope not declared");
    if (stack->current_size > stack->capacity) panic(ERROR_MEMORY_ALLOCATION, "Too many nested scopes/ recursion, divide function into more functions,  files or optimise");
    
    stack->storage[stack->current_size] = hash_table;
    stack->current_size++;
}

void pop(stack* stack) {
    if(!stack) panic(ERROR_UNDEFINED, "scope not declared");
    if (stack->current_size <= 1) panic(ERROR_INTERNAL, "Cannot pop the global (base) scope.");

    stack->current_size--;
}

hash_node* get_current_scope(uint32_t hash, uint32_t name_len, string name) {
    if (!compiler || !compiler->current_variable_hashtable) panic(ERROR_UNDEFINED, "Scope Stack not initialized.");

    for (size_t i = compiler->current_variable_hashtable->current_size - 1; i > 0; i--)
    {   
        hash_node* node = compiler->current_variable_hashtable->storage[i]->hash_map[hash];
        while(node != NULL) {
            if (node->var_name_length == name_len && !strcmp(node->var_name, name)) {
                return node;
            }

            node = node->next;
        }
    }
    panic(ERROR_UNDEFINED_VARIABLE, "undefined variable");
    return NULL;
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
        fprintf(stderr, " at line %lu: \n", glbl_parser->current_line);
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
    printf("%lu\n", arena->capacity);
    printf("I was called\n");
    panic(0, "arena realloc was called");
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

//////////////////////////////////////////////////   SYMBOL TABLE         ////////////////////////////////////////////////////////////////////

function_symbol_node** create_function_hash_table(size_t size)
{
    function_symbol_node** table = arena_alloc(compiler->symbol_arena, sizeof(function_symbol_node*) * size);
    if(!table) panic(ERROR_MEMORY_ALLOCATION, "Function symbol table allocation failed");
    memset(table, 0, sizeof(function_symbol_node*) * size);
    return table;
}

function_symbol_node* make_function_node(statement* function_declaration, uint32_t total_offset, uint32_t total_var_count, hash_node** local_symbol_table)
{
    function_symbol_node* new_node = arena_alloc(compiler->symbol_arena, sizeof(function_symbol_node));
    if(!new_node) panic(ERROR_MEMORY_ALLOCATION, "Function node allocation failed");
    new_node->function_declaration = function_declaration;
    new_node->total_offset = total_offset;
    new_node->total_var_count = total_var_count;
    new_node->local_symbol_table = local_symbol_table;
    new_node->hash = hash_function(function_declaration->stmnt_function_declaration.name, function_declaration->stmnt_function_declaration.name_length);
    new_node->next = NULL;
    return new_node;
}

function_symbol_node* append_function_node(function_symbol_node** table, statement* function_declaration, uint32_t total_offset, uint32_t total_var_count, hash_node** local_symbol_table, size_t index)
{
    function_symbol_node* current = table[index];
    if(!current)
    {
        table[index] = make_function_node(function_declaration, total_offset, total_var_count, local_symbol_table);
        return table[index];
    }
    while(current->next)
    {
        current = current->next;
    }
    current->next = make_function_node(function_declaration, total_offset, total_var_count, local_symbol_table);
    return current->next;
}

hash_node** create_func_scope(size_t size)
{
    hash_node** table = arena_alloc(compiler->symbol_arena, sizeof(hash_node*) * size);
    if(!table) return NULL;
    memset(table, 0, sizeof(hash_node*) * size);
    return table;
}

hash_node* append_var_to_function_node (function_symbol_node** table, function_symbol_node* function_node, string var_name, size_t var_name_length, uint32_t offset, uint32_t size)
{
    if(!table[function_node->hash]) panic(ERROR_INTERNAL, "Undeclared Function");
//   if(!function_node->local_symbol_table) function_node->local_symbol_table = create_hash_table(function_node->total_var_count * 2);
    return append_node(function_node->local_symbol_table, var_name, var_name_length, offset, size);
}


hash_node** create_hash_table(size_t size)
{
    hash_node** table = arena_alloc(compiler->symbol_arena, sizeof(hash_node*) * size);
    if(!table) return NULL;
    memset(table, 0, sizeof(hash_node*) * size);
    return table;
}

static inline size_t hash_function(const string var_name, size_t var_name_length)
{
    return ((var_name[0] + 'a') * var_name_length) % (amount_of_global_variables);
}

hash_node* make_node(string var_name, size_t var_name_length, uint32_t offset, uint32_t size)
{
    hash_node* new_node = arena_alloc(compiler->symbol_arena, sizeof(hash_node));
    if(!new_node) return NULL;
    new_node->var_name = var_name;
    new_node->var_name_length = var_name_length;
    new_node->offset = offset;
    new_node->size = size;
    new_node->next = NULL;
    return new_node;
}

hash_node* append_node(hash_node** table, string var_name, size_t var_name_length, uint32_t offset, uint32_t size)
{
    size_t index = hash_function(var_name, var_name_length);
    hash_node* current = table[index];
    if(!current)
    {
        table[index] = make_node(var_name, var_name_length, offset, size);
        return table[index];
    }
    while(current->next)
    {
        current = current->next;
    }
    current->next = make_node(var_name, var_name_length, offset, size);
    return current->next;
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


static const uint8_t CHAR_TYPE[256] = {
    ['0'] = 11, ['1'] = 11, ['2'] =11, ['3'] = 11, ['4'] = 11, ['5'] = 11, ['6'] = 11, ['7'] = 11, ['8'] = 11, ['9'] = 11,
    ['_'] = 1,
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1, ['g'] = 1, ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1, ['m'] = 1, ['n'] = 1, ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1, ['s'] = 1, ['t'] = 1, ['u'] = 1, ['v'] = 1, ['w'] = 1, ['x'] = 1, ['y'] = 1, ['z'] = 1,
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1, ['G'] = 1, ['H'] = 1, ['I'] = 1, ['J'] = 1, ['K'] = 1, ['L'] = 1, ['M'] = 1, ['N'] = 1, ['O'] = 1, ['P'] = 1, ['Q'] = 1, ['R'] = 1, ['S'] = 1, ['T'] = 1, ['U'] = 1, ['V'] = 1, ['W'] = 1, ['X'] = 1, ['Y'] = 1, ['Z'] = 1,
    ['='] = 2, ['+'] = 4, ['-'] = 5, ['*'] = 6, ['/'] = 7, 
    [' '] = 3, ['\n'] = 3, ['\t'] = 3, ['\r'] = 3,
    ['('] = 8, [')'] = 9, [';'] = 10, [':'] = 12,

};


/////////////////////////////////////////////////// TOKENIZER /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void tokenize(const string source, Compiler* compiler){
    token* tokens = arena_alloc(compiler->token_arena, sizeof(token) * (file_length + 1));


    token_count = 0;
    size_t i = 0;
    size_t line_number = 1;
    while (i < file_length) {
        if (source[i] == '\0') break;

        while (i < file_length && CHAR_TYPE[source[i]] == 3) {
            if (source[i] == '\n') line_number++;
            i++;
        }
        if (i >= file_length) break;
        
        // Process token at position i
        size_t token_start = i;
        
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
            amount_of_global_variables += identify_token(tokens[token_count].str_value.starting_value, tokens[token_count].str_value.length, &tokens[token_count]);
            
            token_count++;
            break;
        }

        case 10:{
        
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_SEMICOLON; // SEMICOLUMN            
            token_count++;
            i++;
            semicolmn_count++;
            break;
        }

        case 12:{
        
            tokens[token_count].line = line_number;
            tokens[token_count].type = TOK_COLON; // COLON            
            token_count++;
            i++;
            switch (source[i])
            {
            case 'i':
                if (source[1] == 'n' && source[2] == 't') tokens[token_count].type = DATA_TYPE_INT;
                i += 3;
                break;
            case 'b':
                if (source[1] == 'o' && source[2] == 'o' && source[3] == 'l') tokens[token_count].type = DATA_TYPE_BOOL;
                i += 4;
                break;
            case 'c':
                if (source[1] == 'h' && source[2] == 'a' && source[3] == 'r') tokens[token_count].type = DATA_TYPE_CHAR;
                i += 4;
                break;
            case 'l':
                if (source[1] == 'o' && source[2] == 'n' && source[3] == 'g') tokens[token_count].type = DATA_TYPE_LONG;
                i += 4;
                break;
            case 'd':
                if (source[1] == 'o' && source[2] == 'u' && source[3] == 'b' && source[4] == 'l' && source[5] == 'e') tokens[token_count].type = DATA_TYPE_DOUBLE;
                i += 6;
                break;
            default:
                panic(ERROR_SYNTAX, ("unidentified data type at line %lu", line_number));
            }
            break;
        }

        default: {
            printf("Error: Unrecognized character '%c' (ASCII %d) at line %zu, position %zu\n", 
           source[i], source[i], line_number, i);
            panic(ERROR_UNDEFINED, "unidentified token");
        }
        if (i >= file_length) {
            break;
        }
    }}

    tokens[token_count].line = line_number;
    tokens[token_count].type = TOK_EOF; // end of file            
    token_count++;
}

static inline int identify_token(const string value, size_t length, token* the_token){
    switch (length){
        case 4:
            if (value[0] == 'e' && value[1] == 'x' && value[2] == 'i' && value[3] == 't') the_token->type = TOK_EXIT;
            break;
        case 3:
            if (value[0] == 'l' && value[1] == 'e' && value[2] == 't') the_token->type = TOK_LET;
            return 1;
        default:
            the_token->type = TOK_IDENTIFIER;
    }
    return 0;

}

static Parser* make_parser(Compiler* compiler){
    Parser* parser = malloc(sizeof(Parser));
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
    TokenType sec_token = peek(parser, 1)->type;

    switch (peek(parser, 0)->type)
    {
    case TOK_EXIT:
        return parse_exit_node(parser);
    
    case TOK_LET:
        return parse_let_node(parser);
    
    case TOK_IDENTIFIER:

        if (sec_token == TOK_EQUAL)
        {
            printf("parsing assignment\n");
            // assignment
            return parse_assignment_node(parser);
        }
        // else
        // {
        //     return parse_statement_expression(parser, PREC_NONE, true);
        // }

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
    return number_node;
}


node* create_variable_node(string var_name, size_t length)
{
    node* var_node = arena_alloc(compiler->expressions_arena, sizeof(node));
    if(!var_node) return NULL;
    var_node->type = NODE_EXPRESSION;
    var_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression));
    if(!var_node->expr) return NULL;

    var_node->expr->variable.length = length;
    var_node->expr->variable.name = var_name;
    var_node->expr->variable.index = hash_function(var_name, length);
    var_node->expr->type = EXPR_IDENTIFIER;
    append_node(peek_stack()->hash_map, var_name, length, 0, 8); // offset will be set later during code generation and size will be int for now (8 bytes)
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
    return unary_node;
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

    case EXPR_IDENTIFIER:
        write_to_buffer("mov rax, [rbp - ", 16, output);
        printf("Variable index: %lu\n", expr->variable.index);
        size_t var_index = expr->variable.index;
        hash_node** symbol_table = peek_stack()->hash_map;
        uint32_t tmp_offset = symbol_table[var_index]->offset;
        while (symbol_table[var_index])
        {
            if (symbol_table[var_index]->var_name_length == expr->variable.length &&
                strncmp(symbol_table[var_index]->var_name, expr->variable.name, expr->variable.length) == 0)
            {
                tmp_offset = symbol_table[var_index]->offset;
                break;
            }
            symbol_table[var_index] = symbol_table[var_index]->next;
        }
        

        string temp = _u64_to_str(tmp_offset, &num_len);
        printf("Variable name: %.*s\n", (int)expr->variable.length, expr->variable.name);
        write_to_buffer(temp, num_len, output);
        printf("Variable offset string: %s\n", temp);
        write_to_buffer("]\n", 2, output);
        printf("done");
        return;
    
    case EXPR_BINARY:
        evaluate_bin(expr);
        break;

    case EXPR_UNARY:
        evaluate_unary(expr);
        break;
    
    default:
        panic(ERROR_UNDEFINED, "Unexpected token, may still not be emplemented");
    }
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
            advance(parser);
            return num_node;
        }

        case TOK_IDENTIFIER:
            constant_foldable = false;
            advance(parser);
            node* var_node = create_variable_node(current_token->str_value.starting_value, current_token->str_value.length);
            printf("Created variable node for %.*s\n node type %d\n", (int)current_token->str_value.length, current_token->str_value.starting_value, var_node->expr->type);
            return var_node;
                
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

node* make_bin_node(node* left, TokenType operator, Parser* parser, bool constant_foldable)
{
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
        if (evaluate_bin(bin_node->expr) == 0)
        {
            panic(ERROR_DIVISION_BY_ZERO, "Division by zero detected during constant folding");
        }
        bin_node->expr->binary.operator = TOK_DIV;
        break;

    default:
        fprintf(stderr, "unidentified operator ");
        panic(ERROR_SYNTAX, "Unknown binary operator");
    }

    return bin_node;
}

// 5 + 2 * 3 + x
node* parse_expression(Parser* parser, int prev_presedence, bool constant_foldable) {
    constant_foldable = true;
    node* left = parse_prefix(parser, constant_foldable); //after that the parser->current will be at the operator after the number/prefix token(s)
    TokenType curr_opperator = peek(parser, 0) -> type;
    int curr_presedence = presedences[curr_opperator];

    while (true)
    {
        if (curr_presedence <= prev_presedence)
        {
            break;
        }
        else
        {
            advance(parser); //now parser is on what is after the operator
            left = make_bin_node(left, curr_opperator, parser, constant_foldable); //now parser is on what was seen as a weak node (like the second plus on 5 + 2 * 3 + 1 )
            curr_opperator = peek(parser, 0) -> type;
            curr_presedence = presedences[curr_opperator];
        }
    }
    return left;
}


///////////////////////////////////////////////////       PARSING NODES           ///////////////////////////////////////////////////////////////////////////

static inline node* parse_exit_node(Parser* parser){
    node* exit_node = (node*)arena_alloc(compiler->statements_arena, sizeof(node));
    if(!exit_node) panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed");

    exit_node->type = NODE_STATEMENT;
    advance(parser); // consume 'exit'

    exit_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    exit_node->stmnt->type = STMT_EXIT;

    exit_node->stmnt->stmnt_exit.exit_code = parse_expression(parser, presedences[TOK_EXIT], true)->expr;

    if (peek(parser, 0)->type != TOK_SEMICOLON) panic(ERROR_SYNTAX, "Expected semicolumn ';'");
    //advance(parser); // consume ;

    return exit_node;
}

static inline node* parse_let_node(Parser* parser){
    node* let_node = (node*)arena_alloc(compiler->statements_arena, sizeof(node));
    if(!let_node) panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed");

    let_node->type = NODE_STATEMENT;
    advance(parser); //consume let

    let_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    let_node->stmnt->type = STMT_LET;

    let_node->stmnt->stmnt_let.name = peek(parser, 0)->str_value.starting_value;
    let_node->stmnt->stmnt_let.name_length = peek(parser, 0)->str_value.length;

    glbl_offset += 8; //here 8 is the size of int
    hash_node* variable_hash_node = append_node(peek_stack()->hash_map, peek(parser, 0)->str_value.starting_value, peek(parser, 0)->str_value.length, glbl_offset, 8);
    let_node->stmnt->stmnt_let.hash = hash_function(peek(parser, 0)->str_value.starting_value, peek(parser, 0)->str_value.length);
    advance(parser); //consume identifier

    token* t = advance(parser);
    if (t->type != TOK_EQUAL)
    {
        panic(ERROR_SYNTAX, "usage: let var = value;");
    }
    let_node->stmnt->stmnt_let.value = parse_expression(parser, presedences[TOK_EQUAL], true)->expr;
    return let_node;
}

static inline node* parse_assignment_node(Parser* parser){
    node* assignment_node = (node*)arena_alloc(compiler->statements_arena, sizeof(node));
    if(!assignment_node) panic(ERROR_MEMORY_ALLOCATION, "Assignment node allocation failed");

    assignment_node->type = NODE_STATEMENT;
    size_t hash = hash_function(peek(parser, 0)->str_value.starting_value, peek(parser, 0)->str_value.length);
    advance(parser); //consume identifier
    advance(parser); //consume equal
    assignment_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    assignment_node->stmnt->type = STMT_ASSIGNMENT;
    assignment_node->stmnt->stmnt_assign.hash = hash;
    assignment_node->stmnt->stmnt_assign.value = parse_expression(parser, presedences[TOK_EQUAL], true)->expr;
    if (peek(parser, 0)->type != TOK_SEMICOLON) panic(ERROR_SYNTAX, "Expected semicolumn ';'");
    advance(parser); // consume ;
    return assignment_node;
}

node* parse_codeblock_node(Parser* parser) {
    if (advance(parser) != TOK_LBRACE)
    {
        panic(ERROR_SYNTAX, "expected '{' at the beginning of code block");
    }
    
    node* codeblock_node = arena_alloc(compiler->statements_arena, sizeof(node));
    if(!codeblock_node) panic(ERROR_MEMORY_ALLOCATION, "Codeblock node allocation failed");

    codeblock_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    if(!codeblock_node->stmnt) panic(ERROR_MEMORY_ALLOCATION, "Codeblock node allocation failed");

    codeblock_node->type = NODE_STATEMENT;
    codeblock_node->stmnt->type = STMT_BLOCK;

    uint64_t capacity = 8;
    uint64_t count = 0;
    statement** statements = arena_alloc(compiler->sandbox_arena, sizeof(statement*) * capacity);
    if(!statements) panic(ERROR_MEMORY_ALLOCATION, "Codeblock statements allocation failed");

    while (peek(parser, 0) != TOK_RBRACE)
    {
        // Parse statements
        if (capacity == count)
        {
            capacity *= 2;
            compiler->statements_arena->current_size -= count * sizeof(statement*);
            statement** new_statements = arena_alloc(compiler->sandbox_arena, sizeof(statement*) * capacity);
            if(!new_statements) panic(ERROR_MEMORY_ALLOCATION, "Codeblock statements re-allocation failed");
            statements = new_statements;
        }
        statements[count++] = parse_statement(parser)->stmnt;
    }
    if (capacity > count) {
        // Only trim if we have extra capacity
        compiler->statements_arena->current_size -= (capacity - count) * sizeof(statement*);
        // No need to reallocate - just adjust the arena pointer
    }
    codeblock_node->stmnt->stmnt_block.statements = statements;
    codeblock_node->stmnt->stmnt_block.statement_count = count;
    advance(parser); //consume '}'
    return codeblock_node;
}

static inline node* parse_function_node(Parser* parser)
{
    node* func_node = arena_alloc(compiler->statements_arena, sizeof(node));
    if(!func_node) panic(ERROR_MEMORY_ALLOCATION, "Function node allocation failed");

    func_node->type = NODE_STATEMENT;
    func_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement));
    if(!func_node->stmnt) panic(ERROR_MEMORY_ALLOCATION, "Function node allocation failed");

    
    func_node->stmnt->type = STMT_FUNCTION;


    token* func_token = advance(parser); //consume parameter name

    func_node->stmnt->stmnt_function_declaration.name = func_token->str_value.starting_value;
    func_node->stmnt->stmnt_function_declaration.name_length = func_token->str_value.length;
    func_node->stmnt->stmnt_function_declaration.hash = hash_function(func_token->str_value.starting_value, func_token->str_value.length);
    

    //create_hash_table() will be used later for local variables
    function_symbol_node* symbol_node = append_function_node(function_symbol_table, func_node->stmnt, func_node->stmnt, 0, NULL, func_node->stmnt->stmnt_function_declaration.hash);

    if (advance(parser) != TOK_LPAREN) panic(ERROR_SYNTAX, "Expected '(' after function name"); //consume function name
    uint32_t param_count = 0;

    if (peek(parser, 0) == TOK_VOID)
    {
        advance(parser); //consume void
        if (peek(parser, 0) != TOK_RPAREN)
        {
            panic(ERROR_SYNTAX, "Expected ')' after 'void' ");
        }
        // no parameters
    }
    else {
        uint32_t i = 0;
        while (peek(parser, i) != TOK_RPAREN) {
                if (peek(parser, i)->type != TOK_IDENTIFIER) {
                    panic(ERROR_SYNTAX, "Expected parameter name");
                }
                i++; //consume parameter name
                if (peek(parser, i)->type != TOK_DATATYPE) {
                    panic(ERROR_SYNTAX, "Expected parameter type");
                }
                i++; //consume parameter type
                if (peek(parser, i)->type != TOK_COMMA && peek(parser, i)->type != TOK_RPAREN) {
                    panic(ERROR_SYNTAX, "Expected parameter separator ',' or ')'");
                }
                i++; //consume parameter name
                param_count++;
        }
    }

    func_node->stmnt->stmnt_function_declaration.param_count = param_count;
    symbol_node->local_symbol_table = create_hash_table(param_count * 2); // * 2 for less collisions
    uint32_t current_rsp_offset = 0;
    func_node->stmnt->stmnt_function_declaration.parameters = arena_alloc(compiler->expressions_arena, sizeof(expression*) * param_count);
    if(!func_node->stmnt->stmnt_function_declaration.parameters) panic(ERROR_MEMORY_ALLOCATION, "Function parameters allocation failed");
    // Parse parameters
    advance(parser); //consume '('
    param_count = 0;
    // fn example(a:type, b:type)
    while (peek(parser, 0) != TOK_RPAREN)
    {
        token* name = advance(parser); //consume parameter name
        token* type = advance(parser); //consume parameter type
        expression* param_expr = arena_alloc(compiler->expressions_arena, sizeof(expression)); // expression of type identifier


        param_expr->type = EXPR_IDENTIFIER;
        param_expr->variable.name = name->str_value.starting_value;
        param_expr->variable.length = name->str_value.length;
        param_expr->variable.index = hash_function(name->str_value.starting_value, name->str_value.length);


        append_var_to_function_node(function_symbol_table, symbol_node, name->str_value.starting_value, type->str_value.length, NULL, Data_type_sizes[type->type]);
        func_node->stmnt->stmnt_function_declaration.parameters[param_count] = param_expr;
        if(!param_expr) panic(ERROR_MEMORY_ALLOCATION, "Function parameter allocation failed");
        param_expr->type = EXPR_IDENTIFIER;

        if (peek(parser, 0)->type == TOK_COMMA) advance(parser); //consume comma
        param_count++;
    }
    advance(parser); // Consume the final ')'

    // now parse the code body
    func_node->stmnt->stmnt_function_declaration.code_block = parse_codeblock_node(parser)->stmnt;
    return func_node;
}


///////////////////////////////////////////////////       CODE GENERATION         ///////////////////////////////////////////////////////////////////////////
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

void generate_assembly(const AST* AST){
    output = fopen("output.asm", "wb"); //clear file
    size_t num_len = 0;
    size_t i = 0;
    write_to_buffer("section .text\n\tglobal _start\n_start:\n", 37, output);
    // will move to function handling later
    write_to_buffer("push rbp\nmov rbp, rsp\nsub rsp, ", 31, output);
    nums_to_str(glbl_offset, &num_len, output);
    write_to_buffer("\n", 1, output);

    while (i < AST->node_count)
    {
        switch (AST->nodes[i]->type)    
        {
        case NODE_STATEMENT:
            switch (AST->nodes[i]->stmnt->type)
            {
            case STMT_FUNCTION:
                {
                    // Function handling code here
                    break;
                }
            case STMT_EXIT:
                {
                printf("exit code type: %d\n", AST->nodes[i]->stmnt->stmnt_exit.exit_code->type);
                evaluate_expression(AST->nodes[i]->stmnt->stmnt_exit.exit_code);
                write_to_buffer("\nmov rdi, rax\nmov rax, 60", 25, output);
                write_to_buffer("\nsyscall\n", 9, output);
                break;
                }
            
            case STMT_LET:
                {
                    evaluate_expression(AST->nodes[i]->stmnt->stmnt_let.value);
                    write_to_buffer("\nmov [rbp - ", 12, output); //here 8 is the size of int, will become dynamic later
                    printf("%u", get_current_scope(AST->nodes[i]->stmnt->stmnt_let.hash, AST->nodes[i]->stmnt->stmnt_let.name_length, AST->nodes[i]->stmnt->stmnt_let.name)->offset);
                    nums_to_str(get_current_scope(AST->nodes[i]->stmnt->stmnt_let.hash, AST->nodes[i]->stmnt->stmnt_let.name_length, AST->nodes[i]->stmnt->stmnt_let.name)->offset, &num_len, output);
                    write_to_buffer("], rax\n", 7, output);
                    break;
                }
            case STMT_ASSIGNMENT:
                {
                    evaluate_expression(AST->nodes[i]->stmnt->stmnt_assign.value); // now the expression is in rax
                    write_to_buffer("\nmov [rbp - ", 12, output);
                    
                    nums_to_str(get_current_scope(AST->nodes[i]->stmnt->stmnt_let.hash, AST->nodes[i]->stmnt->stmnt_let.name_length, AST->nodes[i]->stmnt->stmnt_let.name)->offset, &num_len, output);
                    write_to_buffer("], rax\n", 7, output);
                    break;
                }
            default:
                break;
            }  
        
        default:
            break;
        }
        i++;
    }

    //flush remaining buffer to file
    write_to_buffer("section .rodata\nerr_div0:\tdb \"division by zero\", 10\nerr_div0_len:  equ $ - err_div0", 83, output);
    fwrite(compiler->buffer, 1, compiler->currentsize, output);
    fclose(output);
    //gcc phc.c -o phc && ./phc test.ph
    //nasm -f elf64 output.asm && ld output.o -o output && ./output
}