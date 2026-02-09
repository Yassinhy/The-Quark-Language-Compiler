#include "utilities/utils.h"

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
void print_token(TokenType type) {
    switch (type) {
        // Control tokens
        case TOK_NONE:
            printf("TOK_NONE\n");
            break;
        case TOK_EOF:
            printf("TOK_EOF\n");
            break;

        // Assignment (lowest precedence)
        case TOK_EQUAL:
            printf("TOK_EQUAL\n");
            break;

        // Logical operators
        case TOK_OR:
            printf("TOK_OR\n");
            break;
        case TOK_AND:
            printf("TOK_AND\n");
            break;

        // Comparison
        case TOK_EQ:
            printf("TOK_EQ\n");
            break;
        case TOK_NE:
            printf("TOK_NE\n");
            break;
        case TOK_LT:
            printf("TOK_LT\n");
            break;
        case TOK_GT:
            printf("TOK_GT\n");
            break;
        case TOK_LE:
            printf("TOK_LE\n");
            break;
        case TOK_GE:
            printf("TOK_GE\n");
            break;

        // Arithmetic
        case TOK_ADD:
            printf("TOK_ADD\n");
            break;
        case TOK_SUB:
            printf("TOK_SUB\n");
            break;
        case TOK_MUL:
            printf("TOK_MUL\n");
            break;
        case TOK_DIV:
            printf("TOK_DIV\n");
            break;
        case TOK_PERCENT:
            printf("TOK_PERCENT\n");
            break;

        // Unary operators (high precedence)
        case TOK_NOT:
            printf("TOK_NOT\n");
            break;
        case TOK_BITNOT:
            printf("TOK_BITNOT\n");
            break;
        case TOK_NEGATE:
            printf("TOK_NEGATE\n");
            break;

        // Primary expressions (highest)
        case TOK_NUMBER:
            printf("TOK_NUMBER\n");
            break;
        case TOK_STRING:
            printf("TOK_STRING\n");
            break;
        case TOK_IDENTIFIER:
            printf("TOK_IDENTIFIER\n");
            break;
        case TOK_TRUE:
            printf("TOK_TRUE\n");
            break;
        case TOK_FALSE:
            printf("TOK_FALSE\n");
            break;
        case TOK_NULL:
            printf("TOK_NULL\n");
            break;

        // Grouping
        case TOK_LPAREN:
            printf("TOK_LPAREN\n");
            break;
        case TOK_RPAREN:
            printf("TOK_RPAREN\n");
            break;
        case TOK_LBRACE:
            printf("TOK_LBRACE\n");
            break;
        case TOK_RBRACE:
            printf("TOK_RBRACE\n");
            break;
        case TOK_LBRACKET:
            printf("TOK_LBRACKET\n");
            break;
        case TOK_RBRACKET:
            printf("TOK_RBRACKET\n");
            break;

        // Punctuation
        case TOK_SEMICOLON:
            printf("TOK_SEMICOLON\n");
            break;
        case TOK_COMMA:
            printf("TOK_COMMA\n");
            break;
        case TOK_DOT:
            printf("TOK_DOT\n");
            break;
        case TOK_COLON:
            printf("TOK_COLON\n");
            break;
        case TOK_QUESTION:
            printf("TOK_QUESTION\n");
            break;

        // Keywords
        case TOK_EXIT:
            printf("TOK_EXIT\n");
            break;
        case TOK_LET:
            printf("TOK_LET\n");
            break;
        case TOK_IF:
            printf("TOK_IF\n");
            break;
        case TOK_ELSE:
            printf("TOK_ELSE\n");
            break;
        case TOK_WHILE:
            printf("TOK_WHILE\n");
            break;
        case TOK_FOR:
            printf("TOK_FOR\n");
            break;
        case TOK_FN:
            printf("TOK_FN\n");
            break;
        case TOK_RETURN:
            printf("TOK_RETURN\n");
            break;
        case TOK_BREAK:
            printf("TOK_BREAK\n");
            break;
        case TOK_CONTINUE:
            printf("TOK_CONTINUE\n");
            break;

        // Bitwise operators (medium precedence)
        case TOK_BIT_OR:
            printf("TOK_BIT_OR\n");
            break;
        case TOK_BIT_AND:
            printf("TOK_BIT_AND\n");
            break;
        case TOK_BIT_XOR:
            printf("TOK_BIT_XOR\n");
            break;
        case TOK_SHIFT_LEFT:
            printf("TOK_SHIFT_LEFT\n");
            break;
        case TOK_SHIFT_RIGHT:
            printf("TOK_SHIFT_RIGHT\n");
            break;

        // Data types
        case TOK_DATATYPE:
            printf("TOK_DATATYPE\n");
            break;
        case TOK_VOID:
            printf("TOK_VOID\n");
            break;

        default:
            printf("UNKNOWN_TOKEN\n");
            break;
    }
}