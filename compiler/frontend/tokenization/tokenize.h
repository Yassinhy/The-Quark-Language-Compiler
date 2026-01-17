#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "utilities/utils.h"


extern const uint8_t CHAR_TYPE[256];

void tokenize(const char* source, Compiler* compiler, size_t* token_count, size_t* file_length);

#endif

