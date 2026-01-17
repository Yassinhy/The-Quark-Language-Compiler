#include "utilities/utils.h"
#include "error_handler/error_handler.h"
#include "arena/arena.h"


void* arena_alloc(Arena* arena, size_t old_data_size, Compiler* compiler) {
    size_t data_size = (old_data_size + 7) & ~7; // Align to 8 bytes
    if (arena->current_size + data_size > arena->capacity) {
        panic(ERROR_MEMORY_ALLOCATION, "Not enough memory for more allocation", compiler);
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

Compiler* init_compiler_arenas(size_t file_length) {
    Compiler* arenas = malloc(sizeof(Compiler));
    arenas->token_arena = initialize_arena((file_length + 1) * sizeof(token));
    arenas->statements_arena = initialize_arena(file_length * (sizeof(expression) + sizeof(node)));
    arenas->expressions_arena = initialize_arena(file_length * sizeof(expression));
    arenas->symbol_arena = initialize_arena(1024 * 1024 * 4);

    // make the stack struct
    arenas->symbol_table_stack = malloc(sizeof(symbol_table_stack));
    if (!arenas->symbol_table_stack) panic(ERROR_MEMORY_ALLOCATION, "Symbol table stack allocation failed", arenas);
    
    // make the initial stack (16 scopes and could grow dynamicaly)
    arenas->symbol_table_stack->storage = malloc(16 * sizeof(symbol_table*));
    if (!arenas->symbol_table_stack->storage) panic(ERROR_MEMORY_ALLOCATION, "Symbol table stack allocation failed", arenas);
    
    // initialize capacity
    arenas->symbol_table_stack->capacity = 16;
    
    // initialize global stack
    arenas->symbol_table_stack->storage[0] = arena_alloc(arenas->symbol_arena, sizeof(symbol_table), arenas);
    arenas->symbol_table_stack->storage[0]->parent_scope = NULL;
    arenas->symbol_table_stack->storage[0]->scope_offset = 0;
    arenas->symbol_table_stack->storage[0]->symbol_map = arena_alloc(arenas->symbol_arena, BUCKETS_GLOBAL_SYMBOLTABLE * sizeof(symbol_node*), arenas);
    memset(arenas->symbol_table_stack->storage[0]->symbol_map, 0, BUCKETS_GLOBAL_SYMBOLTABLE * sizeof(symbol_node*));

    arenas->symbol_table_stack->current_size = 1;

    arenas->function_map = arena_alloc(arenas->symbol_arena, BUCKETS_FUNCTION_TABLE * sizeof(function_node**), arenas);
    memset(arenas->function_map, 0, BUCKETS_FUNCTION_TABLE * sizeof(function_node**));

    // initialize parser and AST
    arenas->parser = NULL;
    arenas->ast = NULL;

    arenas->capacity = 16 * 1024;
    arenas->currentsize = 0;
    arenas->if_statements = 0;
    return arenas;
}

