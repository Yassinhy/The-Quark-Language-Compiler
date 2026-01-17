#ifndef ARENA_H
#define ARENA_H

#include "utilities/utils.h"

Compiler* init_compiler_arenas(size_t file_length);\
void arena_reset(Arena* arena);
void free_arena(Arena* arena);
Arena* initialize_arena(size_t capacity);
void free_global_arenas(Compiler* arenas);
void* arena_alloc(Arena* arena, size_t old_data_size, Compiler* compiler);

#endif
