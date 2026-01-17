#ifndef x86_64_H
#define x86_64_H

#include "utilities/utils.h"

void write_to_buffer(const char* code, size_t code_length, FILE* output, Compiler* compiler);
void nums_to_str(size_t number, size_t* num_len, FILE* output, Compiler* compiler);
void enter_existing_scope(symbol_table* new_table, bool independent, Compiler* compiler);
// void generate_function_code(statement* stmt, size_t* num_len, FILE* output, Compiler* compiler);
// void generate_statement_code(statement* stmt, size_t* num_len, FILE* output, Compiler* compiler);
void generate_assembly_x86_64(const AST* AST, Compiler* compiler, FILE* output);

char* _u64_to_str(size_t number, size_t* num_len);

#endif