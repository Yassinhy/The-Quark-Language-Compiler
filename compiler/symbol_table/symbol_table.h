#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "utilities/utils.h"
#include "arena/arena.h"

size_t hash_function(const char* var_name, size_t var_name_length);
symbol_node* add_var_to_current_scope(Compiler* compiler, expression* variable, variable_storage_type storage_type, normal_register reg_location);
symbol_table* peek_symbol_stack(Compiler* compiler);
symbol_node* find_variable(Compiler* compiler, uint32_t hash, char* var_name, uint32_t var_name_length);

void enter_new_scope(Compiler* compiler, Data_type scope_data_type);
void exit_current_scope(Compiler* compiler);

void append_function_to_func_map(function_node* function_node_input, size_t hash, Compiler* compiler);
function_node* find_function_symbol_node (char* function_name, size_t function_name_length, size_t hash, Compiler* compiler);

#endif