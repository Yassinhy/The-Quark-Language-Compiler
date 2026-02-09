#include "utilities/utils.h"
#include "arena/arena.h"
#include "symbol_table/symbol_table.h"
#include "error_handler/error_handler.h"
#include <stddef.h>


size_t hash_function(const char* var_name, size_t var_name_length)
{
    return ((var_name[0] + 'a') * var_name_length + var_name[var_name_length - 1] * var_name_length / 2);
}

symbol_table* peek_symbol_stack(Compiler* compiler) {
    if (compiler->symbol_table_stack->current_size == 0) {
        panic(ERROR_UNDEFINED, "Trying to reach outside global scope", compiler);
    }
    symbol_table* current_table = compiler->symbol_table_stack->storage[compiler->symbol_table_stack->current_size - 1];
    if (!current_table) {
        panic(ERROR_UNDEFINED, "symbol table does not exist", compiler);
    }
    return current_table;
}

void enter_new_scope(Compiler* compiler, Data_type scope_data_type) {
    if (!compiler) panic(ERROR_UNDEFINED, "Compiler not initialized", compiler);
    if (compiler->symbol_table_stack->current_size + 1 >= compiler->symbol_table_stack->capacity) {
        compiler->symbol_table_stack->storage = realloc(compiler->symbol_table_stack->storage, compiler->symbol_table_stack->capacity * 2 * sizeof(symbol_table*));
        compiler->symbol_table_stack->capacity *= 2;
    }
    if (!compiler->symbol_table_stack->storage) panic(ERROR_MEMORY_ALLOCATION, "Entered too many scopes for your memory", compiler);
    symbol_table* new_table = arena_alloc(compiler->symbol_arena, sizeof(symbol_table), compiler);
    if (!new_table) panic(ERROR_MEMORY_ALLOCATION, "New symbol table allocation failed", compiler);
    
    new_table->parent_scope = peek_symbol_stack(compiler);
    new_table->scope_data_type = scope_data_type;
    new_table->scope_offset = 0;

    new_table->symbol_map = arena_alloc(compiler->symbol_arena, 16 * sizeof(symbol_node*), compiler);
    if (!new_table->symbol_map) panic(ERROR_MEMORY_ALLOCATION, "new scope unable to be declared, not enough memory", compiler);
    memset(new_table->symbol_map, 0, 16 * sizeof(symbol_node*));
    
    compiler->symbol_table_stack->storage[compiler->symbol_table_stack->current_size] = new_table;
    compiler->symbol_table_stack->current_size++;
}


symbol_node* add_var_to_current_scope(Compiler* compiler, expression* variable, variable_storage_type storage_type, normal_register reg_location) {
    symbol_node** new_symbol;
    size_t hash = hash_function(variable->variable.name, variable->variable.length);
    if (peek_symbol_stack(compiler)->parent_scope == NULL)
    {
        new_symbol = &peek_symbol_stack(compiler)->symbol_map[hash % BUCKETS_GLOBAL_SYMBOLTABLE];
    }
    else
    {
        new_symbol = &peek_symbol_stack(compiler)->symbol_map[hash % BUCKETS_IN_EACH_SYMBOL_MAP];
    }
    
    while (*new_symbol != NULL) {
        if ((*new_symbol)->var_name_size == variable->variable.length && 
            !strncmp((*new_symbol)->var_name, variable->variable.name, variable->variable.length)) {
            panic(ERROR_INTERNAL, "Variable already declared in this scope", compiler);
        }
        new_symbol = &(*new_symbol)->next;
    }

    (*new_symbol) = arena_alloc(compiler->symbol_arena, sizeof(symbol_node), compiler);
    (*new_symbol)->next = NULL;
    (*new_symbol)->var_name = variable->variable.name;
    (*new_symbol)->var_name_size = variable->variable.length;
    (*new_symbol)->data_type = variable->variable.data_type;
    (*new_symbol)->where_it_is_stored = storage_type;
    switch (storage_type)
    {
    case STORE_IN_STACK:
        peek_symbol_stack(compiler)->scope_offset += Data_type_sizes[variable->variable.data_type];
        (*new_symbol)->offset = peek_symbol_stack(compiler)->scope_offset;
        break;
    case STORE_IN_REGISTER:
        (*new_symbol)->register_location = reg_location;
        break;
    case STORE_AS_PARAM:
        peek_symbol_stack(compiler)->param_offset += Data_type_sizes[variable->variable.data_type];
        (*new_symbol)->param_offset = 8 + peek_symbol_stack(compiler)->scope_offset; // 8 for the return pointer
        break;
    case STORE_IN_FLOAT_REGISTER:
        //TO DO     
        panic(ERROR_INTERNAL, "Float register storage not implemented yet", compiler);
        break;
    default:
        break;
    }
    return (*new_symbol);
}


symbol_node* find_variable(Compiler* compiler, uint32_t hash, char* var_name, uint32_t var_name_length) {
    if(!compiler || !compiler->symbol_table_stack) panic(ERROR_UNDEFINED, "Scope Stack not initialized.", compiler);
    if(!compiler->symbol_table_stack->storage) panic(ERROR_UNDEFINED, "Scope Stack storage not initialized.", compiler);
    int64_t i = compiler->symbol_table_stack->current_size - 1;
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



void append_function_to_func_map(function_node* function_node_input, size_t hash, Compiler* compiler) {
    
    function_node** bucket = &(compiler->function_map[hash % BUCKETS_FUNCTION_TABLE]);
    while (*bucket != NULL) {
        if ((*bucket)->name_length == function_node_input->name_length && 
            !strncmp((*bucket)->name, function_node_input->name, function_node_input->name_length)) {
            panic(ERROR_INTERNAL, "Function already declared before", compiler);
        }
        bucket = &(*bucket)->next;
    }
    *bucket = function_node_input;
    function_node_input->next = NULL;
}


function_node* find_function_symbol_node (char* function_name, size_t function_name_length, size_t hash, Compiler* compiler) {
    
    function_node** bucket = &(compiler->function_map[hash % BUCKETS_FUNCTION_TABLE]);
    while (*bucket != NULL) {
        if ((*bucket)->name_length == function_name_length && 
            !strncmp((*bucket)->name, function_name, function_name_length)) {
            return *bucket;
        }
        bucket = &(*bucket)->next;
    }
    panic(ERROR_UNDEFINED_FUNCTION, "Function not defined", compiler);
    return NULL; 
}


void exit_current_scope(Compiler* compiler) {
    if (compiler->symbol_table_stack->current_size > 1) {
        compiler->symbol_table_stack->current_size--;
    } else {
        panic(ERROR_INTERNAL, "Tried to exit global scope", compiler);
    }
}


