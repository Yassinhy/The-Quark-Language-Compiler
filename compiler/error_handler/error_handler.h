#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "utilities/utils.h"

void panic(error_code error_code, char* message, Compiler* compiler);
void warning(char* message, Compiler* compiler);

#endif