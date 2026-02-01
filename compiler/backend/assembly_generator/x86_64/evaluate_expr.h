#ifndef EVALUATE_EXPR_H
#define EVALUATE_EXPR_H
#include "utilities/utils.h"

void evaluate_expression_x86_64(expression* expr, Compiler* compiler, FILE* output, int conditional, Data_type wanted_output_result);

int evaluate_bin(expression* binary_exp, Compiler* compiler, FILE* output, int conditional, Data_type wanted_output_result);
int evaluate_unary(expression* unary_exp, Compiler* compiler, FILE* output, Data_type wanted_output_result);

#endif