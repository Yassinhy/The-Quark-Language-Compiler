#ifndef EXPRESSIONS_H


#define EXPRESSIONS_H
#include "utilities/utils.h"


node* create_number_node(int value, Compiler* compiler);
node* create_variable_node_dec(char* var_name, size_t length, variable_storage_type storage_type, normal_register reg_location, Data_type data_type, Compiler* compiler);
node* create_unary_node(TokenType op, node* operand, Compiler* compiler);
node* create_bin_node(node* left, TokenType op, Parser* parser, bool constant_foldable, Compiler* compiler);
node* create_func_call_node(char* func_name, size_t name_length, expression* arguments, size_t param_count, Compiler* compiler);
node* create_variable_node(char* var_name, size_t length, Compiler* compiler);

void evaluate_expression_x86_64(expression* expr, Compiler* compiler, FILE* output);


#endif