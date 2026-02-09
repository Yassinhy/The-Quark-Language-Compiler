#include "utilities/utils.h"
#include "error_handler/error_handler.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arena/arena.h"


static size_t alloc_count = 0;

void* arena_alloc(Arena* arena, size_t old_data_size, Compiler* compiler) {
    size_t data_size = (old_data_size + 7) & ~7; // Align to 8 bytes
    alloc_count++;
    if (arena->current_size + data_size > arena->capacity) {
        size_t new_capacity = arena->capacity;
        
        // Calculate new capacity (doubling until it fits the new data)
        while (arena->current_size + data_size > new_capacity) {
            new_capacity *= 2;
        }
        void* new_data = realloc(arena->data, new_capacity);
        if (!new_data) {
            panic(ERROR_MEMORY_ALLOCATION, "Not enough memory to grow arena", compiler);
        }

        arena->data = (uint8_t*)new_data;
        arena->capacity = new_capacity;
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

    if(arenas->counters) {
        free(arenas->counters->end_whiles_stack);
        free(arenas->counters->end_ifs_stack);
        free(arenas->counters);
    }

    free(arenas);
}

Compiler* init_compiler_arenas(size_t file_length) {
    Compiler* arenas = malloc(sizeof(Compiler));
    arenas->token_arena = initialize_arena((file_length + 1) * 10 * sizeof(token));
    arenas->statements_arena = initialize_arena(file_length * 10 * (sizeof(statement) + sizeof(node)));
    arenas->expressions_arena = initialize_arena(file_length * 10 * sizeof(expression));
    arenas->symbol_arena = initialize_arena(1024 * 1024 * 8);

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

    // set all counters
    arenas->counters = malloc(sizeof(counters));
    arenas->counters->if_statements = 0;
    arenas->counters->while_statements = 0;

    // set all counter stacks
    arenas->counters->end_whiles_stack = malloc(32 * sizeof(size_t)); // start with 32 nested scopes
    arenas->counters->end_whiles_capacity = 32;
    arenas->counters->end_whiles_current = 0;

    arenas->counters->end_ifs_stack = malloc(32 * sizeof(size_t)); // start with 32 nested scopes
    arenas->counters->end_ifs_capacity = 32;
    arenas->counters->end_ifs_current = 0;
    
    
    return arenas;
}

void push_to_while_stack(size_t counter, Compiler* compiler) {
    if (compiler->counters->end_whiles_current + 1 > compiler->counters->end_whiles_capacity) {
        size_t* tmp_stack = realloc(compiler->counters->end_whiles_stack, compiler->counters->end_whiles_capacity * 2 * sizeof(size_t));
        if (!tmp_stack) panic(ERROR_RUNTIME, "nested too many scopes, not enough memory for more", compiler);
        else {
            compiler->counters->end_whiles_stack = tmp_stack;
            compiler->counters->end_whiles_capacity *= 2;
        }
    }
    compiler->counters->end_whiles_stack[compiler->counters->end_whiles_current] = counter;
    compiler->counters->end_whiles_current += 1;
}

void pop_from_while_stack(Compiler* compiler) {
    if (compiler->counters->end_whiles_current == 0) {
        panic(ERROR_UNDEFINED, "Trying to pop end_while counter from no scope", compiler);
    }
    compiler->counters->end_whiles_current -= 1;
}

size_t peek_while_stack(Compiler* compiler) {
    if (compiler->counters->end_whiles_current == 0) {
        panic(ERROR_UNDEFINED, "Trying to peek end_while counter from no scope pushed", compiler);
    }
    return compiler->counters->end_whiles_stack[compiler->counters->end_whiles_current - 1];
}

void push_to_if_stack(size_t counter, Compiler* compiler) {
    if (compiler->counters->end_ifs_current + 1 > compiler->counters->end_ifs_capacity) {
        size_t* tmp_stack = realloc(compiler->counters->end_ifs_stack, compiler->counters->end_ifs_capacity * 2 * sizeof(size_t));
        if (!tmp_stack) panic(ERROR_RUNTIME, "nested too many scopes, not enough memory for more", compiler);
        else {
            compiler->counters->end_ifs_stack = tmp_stack;
            compiler->counters->end_ifs_capacity *= 2;
        }
    }
    compiler->counters->end_ifs_stack[compiler->counters->end_ifs_current] = counter;
    compiler->counters->end_ifs_current += 1;
}

void pop_from_if_stack(Compiler* compiler) {
    if (compiler->counters->end_ifs_current == 0) {
        panic(ERROR_UNDEFINED, "Trying to pop end_if counter from no scope", compiler);
    }
    compiler->counters->end_ifs_current -= 1;
}

size_t peek_if_stack(Compiler* compiler) {
    if (compiler->counters->end_ifs_current == 0) {
        panic(ERROR_UNDEFINED, "Trying to peek end_if counter from no scope pushed", compiler);
    }
    return compiler->counters->end_ifs_stack[compiler->counters->end_ifs_current - 1];
}
