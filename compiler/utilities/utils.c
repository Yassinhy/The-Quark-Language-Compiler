#include "utils.h"

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

const char* reg[] = {
    "rdi",
    "rsi",
    "rdx",
    "rcx",
    "r8 ",
    "r9 "
};

const normal_register registers[] = {
    [0] = RDI,
    [1] = RSI,
    [2] = RDX,
    [3] = RCX,
    [4] = R8,
    [5] = R9
};


char* readfile(const char* filename, size_t* file_length) {
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
    char* content = malloc(size + 1);
    if (content == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }
    
    // Read entire file at once
    *file_length = fread(content, 1, size, file);
    content[*file_length] = '\0';
    fclose(file);
    return content;
}