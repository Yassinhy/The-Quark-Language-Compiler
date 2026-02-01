#include "backend/assembly_generator/x86_64/evaluate_expr.h"
#include "backend/assembly_generator/x86_64/x86_64.h"
// #include "frontend/expression_creation/expressions.h"
#include "utilities/utils.h"
#include "error_handler/error_handler.h"
#include <stdio.h>

const char* regs32[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "r8d", "r9d"};
const char* regs64[] = {"rax", "rbx", "rdi", "rsi","rdx", "rcx", "r8 ", "r9 "};

const char* get_reg( int reg_index, Data_type type) {
    if (type == DATA_TYPE_INT) {
        return regs32[reg_index];
    } else {
        return regs64[reg_index];
    }
}

void evaluate_expression_x86_64(expression* expr, Compiler* compiler, FILE* output, int conditional, Data_type wanted_output_result)
{
    size_t num_len;
    char buffer[128];

    printf("Evaluating expression of type %d\n", expr->type);

    switch (expr->type)
    {
    
    case EXPR_INT:{

        int len = snprintf(buffer, sizeof(buffer), "mov %s, ", get_reg(0, wanted_output_result));
        write_to_buffer(buffer, len, output, compiler);

        char* temp2 = _u64_to_str(expr->integer.value, &num_len);
        write_to_buffer(temp2, num_len, output, compiler);
        write_to_buffer("\n", 1, output, compiler);
        return;
    }

    case EXPR_IDENTIFIER: 
    {
        size_t temp = 0;
        symbol_node* var_node = expr->variable.node_in_table;

        if (var_node->where_it_is_stored == STORE_IN_STACK)
        {
            if (wanted_output_result == DATA_TYPE_LONG && var_node->data_type == DATA_TYPE_INT) {
                int len = snprintf(buffer, sizeof(buffer), "movsx rax, dword[rbp - ");
                write_to_buffer(buffer, len, output, compiler);
            }
            else {
                int len = snprintf(buffer, sizeof(buffer), "mov %s, [rbp - ", get_reg(0, wanted_output_result));
                write_to_buffer(buffer, len, output, compiler);
            }

            nums_to_str(var_node->offset, &temp, output, compiler);
            write_to_buffer("]\n", 2, output, compiler);
        }
        else if (var_node->where_it_is_stored == STORE_AS_PARAM)
        {

            if (wanted_output_result == DATA_TYPE_LONG && var_node->data_type == DATA_TYPE_INT) {
                int len = snprintf(buffer, sizeof(buffer), "movsxd rax, dword[rbp + ");
                write_to_buffer(buffer, len, output, compiler);
            }
            else {
                int len = snprintf(buffer, sizeof(buffer), "mov %s, [rbp + ", get_reg(0, wanted_output_result));
                write_to_buffer(buffer, len, output, compiler);
            }

            nums_to_str(var_node->offset, &temp, output, compiler);
            write_to_buffer("]\n", 2, output, compiler);
        }
        else if (var_node->where_it_is_stored == STORE_IN_REGISTER)
        {

            if (wanted_output_result == DATA_TYPE_LONG && var_node->data_type == DATA_TYPE_INT) {
                int len = snprintf(buffer, sizeof(buffer), "movsxd rax, ");
                write_to_buffer(buffer, len, output, compiler);
            }
            else if (wanted_output_result == DATA_TYPE_INT && var_node->data_type == DATA_TYPE_INT) {
                int len = snprintf(buffer, sizeof(buffer), "mov eax, ");
                write_to_buffer(buffer, len, output, compiler);
            }
            else {
                int len = snprintf(buffer, sizeof(buffer), "mov rax, ");
                write_to_buffer(buffer, len, output, compiler);
            }

            const char* reg_name = get_reg(2 + var_node->register_location, wanted_output_result);
            write_to_buffer(reg_name, strlen(reg_name), output, compiler);
            write_to_buffer("\n", 1, output, compiler);
        }

        return;
    }

    case EXPR_BINARY:
        evaluate_bin(expr, compiler, output, conditional, wanted_output_result);
        break;

    case EXPR_UNARY:
        evaluate_unary(expr, compiler, output, wanted_output_result);
        break;
    
    case EXPR_FUNCTION_CALL:
    {
        size_t param_count = expr->func_call.parameter_count;
        write_to_buffer("push r9\npush r8\npush rcx\npush rdx\npush rsi\npush rdi\n", 52, output, compiler);
        
        if (expr->func_call.parameter_count <= 6) {
            for (size_t i = 0; i < param_count; i++)
            {
                Data_type argument_data_type = expr->func_call.arguments[i].result_type;

                evaluate_expression_x86_64(&(expr->func_call.arguments[i]), compiler, output, false, argument_data_type);
                
                int len0 = snprintf(buffer, sizeof(buffer), "mov %s, %s\n", get_reg(i + 2, argument_data_type), get_reg(0, argument_data_type));
                write_to_buffer(buffer, len0, output, compiler);
                
            }
            
        }
        else {
            size_t i = 0;
            while (i <= 6)
            {
                Data_type argument_data_type = expr->func_call.arguments[i].result_type;

                evaluate_expression_x86_64(&(expr->func_call.arguments[i]), compiler, output, false, argument_data_type);

                int len0 = snprintf(buffer, sizeof(buffer), "mov %s, %s\n", get_reg(i + 2, argument_data_type), get_reg(0, argument_data_type));
                write_to_buffer(buffer, len0, output, compiler);

                i++;
            }
            for (size_t j = param_count - 1; j > 6; j--)
            {
                Data_type argument_data_type = expr->func_call.arguments[j].result_type;

                evaluate_expression_x86_64(&(expr->func_call.arguments[j]), compiler, output, false, argument_data_type);
                int len0 = snprintf(buffer, sizeof(buffer), "push rax\n");
                write_to_buffer(buffer, len0, output, compiler);
            }
            
            
        }
        write_to_buffer("call ", 5, output, compiler);
        write_to_buffer(expr->func_call.name, expr->func_call.name_length, output, compiler);
        write_to_buffer("\npop rdi\npop rsi\npop rdx\npop rcx\npop r8\npop r9\n", 47, output, compiler);
        break;
    }
    default:
        panic(ERROR_UNDEFINED, "Unexpected token, may still not be emplemented", compiler);
    }
}


int evaluate_bin(expression* binary_exp, Compiler* compiler, FILE* output, int conditional, Data_type wanted_output_result)
{
    char buffer[128];
    printf("\n\n\nbinary_exp->binary.right: %i\n\n\n", binary_exp->binary.right->variable.data_type);
    evaluate_expression_x86_64(binary_exp->binary.right, compiler, output, false, wanted_output_result); //right value

    int len0 = snprintf(buffer, sizeof(buffer), "push rax\n");
    write_to_buffer(buffer, len0, output, compiler);

    evaluate_expression_x86_64(binary_exp->binary.left, compiler, output, false, wanted_output_result); // left value

    switch (binary_exp->binary.op)
    {
    case TOK_ADD: {
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\nadd %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    }
    case TOK_SUB:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\nsub %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    }
    case TOK_MUL:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\nimul %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    }
    case TOK_DIV:{
        int len = snprintf(buffer, sizeof(buffer), "pop rbx\n");
        write_to_buffer(buffer, len, output, compiler);

        // Sign extend RAX into RDX (Required for idiv)
        if (wanted_output_result == DATA_TYPE_INT) {
            write_to_buffer("cdq\n", 4, output, compiler); // EAX -> EDX:EAX
            write_to_buffer("idiv ebx\n", 9, output, compiler);
        } else {
            write_to_buffer("cqo\n", 4, output, compiler); // RAX -> RDX:RAX
            write_to_buffer("idiv rbx\n", 9, output, compiler);
        }
        break;
    }

    case TOK_PERCENT:{
        int len = snprintf(buffer, sizeof(buffer), "pop rbx\n");
        write_to_buffer(buffer, len, output, compiler);

        // 2. Sign extend RAX into RDX (Required for idiv)
        if (wanted_output_result == DATA_TYPE_INT) {
            write_to_buffer("cdq\n", 4, output, compiler);
            write_to_buffer("idiv ebx\n", 9, output, compiler);
        } else {
            write_to_buffer("cqo\n", 4, output, compiler);
            write_to_buffer("idiv rbx\n", 9, output, compiler);
        }
        write_to_buffer("mov rax, rdx\n", 13, output, compiler);
        break;
    }

    case TOK_EQ:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\ncmp %s, %s\n", get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, sizeof(buffer), "sete al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
            write_to_buffer(buffer, len0, output, compiler);
        }
        else if (conditional == 1) {   // for the if statements
            write_to_buffer("jne ", 4, output, compiler);
        }
        else if (conditional == 2) // for the loops
        {
            write_to_buffer("je ", 3, output, compiler);
        }
        break;
    }
        
    case TOK_NE:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, sizeof(buffer), "sete al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
            write_to_buffer(buffer, len0, output, compiler);
        }
        else if (conditional == 1) {   // for the if statements
            write_to_buffer("je ", 3, output, compiler);
        }
        else if (conditional == 2) // for the loops
        {
            write_to_buffer("jne ", 4, output, compiler);
        }
        break;
    }

    case TOK_GT:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, sizeof(buffer), "sete al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
            write_to_buffer(buffer, len0, output, compiler);
        }
        else if (conditional == 1) {   // for the if statements
            write_to_buffer("jle ", 4, output, compiler);
        }
        else if (conditional == 2) // for the loops
        {
            write_to_buffer("jg ", 3, output, compiler);
        }
        break;
    }
//
    case TOK_LT:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, sizeof(buffer), "sete al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
            write_to_buffer(buffer, len0, output, compiler);
        }
        else if (conditional == 1) {   // for the if statements
            write_to_buffer("jge ", 4, output, compiler);
        }
        else if (conditional == 2) // for the loops
        {
            write_to_buffer("jl ", 3, output, compiler);
        }
        break;
    }

    case TOK_GE:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, sizeof(buffer), "sete al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
            write_to_buffer(buffer, len0, output, compiler);
        }
        else if (conditional == 1) {   // for the if statements
            write_to_buffer("jl ", 3, output, compiler);
        }
        else if (conditional == 2) // for the loops
        {
            write_to_buffer("jge ", 4, output, compiler);
        }
        break;
    }

    case TOK_LE:{
        int len0 = snprintf(buffer, sizeof(buffer), "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, sizeof(buffer), "sete al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
            write_to_buffer(buffer, len0, output, compiler);
        }
        else if (conditional == 1) {   // for the if statements
            write_to_buffer("jg ", 3, output, compiler);
        }
        else if (conditional == 2) // for the loops
        {
            write_to_buffer("jle ", 4, output, compiler);
        }
        break;
    }

    default:
        return 0;
    }
    return 0; // warning suppression
}


int evaluate_unary(expression* unary_exp, Compiler* compiler, FILE* output, Data_type wanted_output_result)
{
    char buffer[128];
    switch (unary_exp->unary.op)
    {
    case TOK_SUB:
        evaluate_expression_x86_64(unary_exp->unary.operand, compiler, output, false, wanted_output_result);
        int len0 = snprintf(buffer, sizeof(buffer), "neg %s\n", get_reg(0,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    
    default:
        return 0;
    }
    return 0;
}

