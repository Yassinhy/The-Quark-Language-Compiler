#include "utilities/utils.h"
#include "arena/arena.h"
#include "error_handler/error_handler.h"

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
        fprintf(stderr, " at line %zu: \n", compiler->parser->current_line);
    }
    fprintf(stderr, "\n%s\n", message);
    fprintf(stderr, "compilation process stopped\n");

    free_global_arenas(compiler);
    if (compiler->ast)
    {
        fprintf(stderr, "freeing the AST\n");
        free(compiler->ast->nodes);
        free(compiler->ast->parser);
        free(compiler->ast);
        exit(error_code);
    }

    else if (compiler->parser)
    {
        fprintf(stderr, "freeing the parser\n");
        free(compiler->parser);
        exit(error_code);
    }

    else
    {
        fprintf(stderr, "nothing to free\n");
        exit(error_code);
    }
    

}
