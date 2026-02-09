#include "utilities/utils.h"
#include "arena/arena.h"
#include "error_handler/error_handler.h"
#include "frontend/parsing/parsing.h"
#include "frontend/tokenization/tokenize.h"
#include "frontend/parsing/parsing.h"
#include "backend/assembly_generator/x86_64/x86_64.h"
#include <stdio.h>


int main(int argc, char** argv) { 
    // test
    clock_t start = clock();
    // parameter checker for ./phc <filename>
    if (argc != 4) {
        printf("Usage: %s <file.ph> <architecture> <output program name>\n", argv[0]);
        return 1;
    }

    //read file
    size_t* file_length = malloc(sizeof(size_t));
    char* source = readfile(argv[1], file_length);

    //initialize compiler arenas
    Compiler* compiler = init_compiler_arenas(*file_length);
    // tokenize
    size_t* token_count = malloc(sizeof(size_t));
    size_t* function_count = malloc(sizeof(size_t));

    tokenize(source, compiler, token_count, file_length, function_count);
    if (*token_count == 0) {
        panic(ERROR_INTERNAL, "ERROR: No tokens created! Check your tokenizer.", compiler);
    }

    //create parser
    Parser* parser = make_parser(compiler);
    compiler->parser = parser;
    parser->tokens = (token*)compiler->token_arena->data;
    parser->token_count = *token_count;

    // Abstract Syntax Tree
    AST* ast = arena_alloc(compiler->statements_arena, sizeof(AST), compiler);

    compiler->ast = ast;
    ast->nodes = arena_alloc(compiler->statements_arena, sizeof(node*) * *token_count, compiler);  // Allocate pointer array
    
    ast->function_nodes = arena_alloc(compiler->statements_arena, (sizeof(node*) * (*function_count + 20)), compiler);  // Allocate pointer array
    ast->node_count = 0;
    ast->function_node_count = 0;
    ast->parser = parser;

    // First pass
    while (parser->current < (*token_count - 1)) {
        if (peek(parser, 0)->type == TOK_FN) {
            ast->function_nodes[ast->function_node_count++] = parse_function_node(compiler, parser);
        }
        else {
            advance(parser);
        }
    }

    // reset parser
    compiler->parser->current = 0;

    // parse code
    while (parser->current < *token_count - 1) {
        node* parsed_node = parse_statement(compiler, parser);
        if (parsed_node != NULL) {
            ast->nodes[ast->node_count++] = parsed_node;
        }

        if (peek(parser, 0)->type == TOK_SEMICOLON)
        {
            advance(parser);
        }
    }

    if (ast->nodes == NULL) {
        panic(ERROR_INTERNAL, "ERROR: AST creation failed!", compiler);
    }

    // create output file
    FILE* output = fopen("output.asm", "wb");

    // generate code
    if (strncmp("x86_64", argv[2], 6) == 0) {
        generate_assembly_x86_64 ((const AST*)ast, compiler, output);
        /*else if (strcasecmp("arm64", argv[2]) == 0) {
        generate_assembly_arm_64 ((const AST*)ast, compiler);
        }*/
    } else
    {
        panic(ERROR_UNDEFINED, "undefined system architecture, currently supporting x86_64 and arm64 only", compiler);
    }

    // terminate program and free memory
    free(source);
    free(token_count);
    free(file_length);
    free_global_arenas(compiler);
    
    clock_t end = clock();

    char command[512];
    // build the command string
    snprintf(command, sizeof(command), 
            "nasm -f elf64 %s.asm && ld %s.o -o %s", 
            argv[3], argv[3], argv[3]);

    int result = system(command);

    if (result != 0) {
        fprintf(stderr, "Compilation or linking failed.\n");
    }
    double time_spent = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Code compiled in %.6f seconds\n", time_spent);
    
    return 0;
}
