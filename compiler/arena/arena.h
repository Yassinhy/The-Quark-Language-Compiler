#ifndef ARENA_H
#define ARENA_H

#include "utilities/utils.h"
#include <stddef.h>

Compiler* init_compiler_arenas(size_t file_length);\
void arena_reset(Arena* arena);
void free_arena(Arena* arena);
Arena* initialize_arena(size_t capacity);
void free_global_arenas(Compiler* arenas);
void* arena_alloc(Arena* arena, size_t old_data_size, Compiler* compiler);

void push_to_while_stack(size_t counter, Compiler* compiler);
void pop_from_while_stack(Compiler* compiler);
size_t peek_while_stack(Compiler* compiler);

void push_to_if_stack(size_t counter, Compiler* compiler);
void pop_from_if_stack(Compiler* compiler);
size_t peek_if_stack(Compiler* compiler);


#endif
