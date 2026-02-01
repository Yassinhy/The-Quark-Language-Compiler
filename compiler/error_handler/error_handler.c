#include "utilities/utils.h"
#include "arena/arena.h"
#include "error_handler/error_handler.h"
#include "frontend/parsing/parsing.h"
void warning(char* message, Compiler* compiler) {
    if(compiler->parser){
        fprintf(stderr, " at line %zu: \n", peek(compiler->parser, 0)->line);
    }
    fprintf(stderr, "\n%s\n", message);
}
void panic(error_code error_code, char* message, Compiler* compiler)
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
    if(compiler->parser){
        fprintf(stderr, " at line %zu: \n", peek(compiler->parser, 0)->line);
    }
    fprintf(stderr, "\n%s\n", message);
    fprintf(stderr, "compilation process stopped\n");

    free_global_arenas(compiler);
    
    exit(error_code);
}
