#include "backend/assembly_generator/x86_64/evaluate_expr.h"
#include "backend/assembly_generator/x86_64/x86_64.h"
// #include "frontend/expression_creation/expressions.h"
#include "utilities/utils.h"
#include "error_handler/error_handler.h"
#include <stdbool.h>

void evaluate_expression_x86_64(expression* expr, Compiler* compiler, FILE* output, bool conditional)
{
    size_t num_len;
    printf("Evaluating expression of type %d\n", expr->type);

    switch (expr->type)
    {
    
    case EXPR_INT:{
    
        write_to_buffer("mov rax, ", 9, output, compiler);

        // if (expr->integer.value < 0)
        // {
        //     write_to_buffer("-", 1, output, compiler);
        //     write_to_buffer(_u64_to_str(-(expr->integer.value), &num_len), num_len, output, compiler);
        // }
        // else
        // {
             char* temp = _u64_to_str(expr->integer.value, &num_len);
             write_to_buffer(temp, num_len, output, compiler);
             write_to_buffer("\n", 1, output, compiler);
        // }
        return;
    }

    case EXPR_IDENTIFIER: 
    {
        size_t temp = 0;

        symbol_node* var_node = expr->variable.node_in_table;
        printf("_______________________________%i\n", var_node->where_it_is_stored);
        if (var_node->where_it_is_stored == STORE_IN_STACK)
        {
            write_to_buffer("mov rax, [rbp - ", 16, output, compiler);
            nums_to_str(var_node->offset, &temp, output, compiler);
            write_to_buffer("]\n", 2, output, compiler);
        }
        else if (var_node->where_it_is_stored == STORE_AS_PARAM)
        {
            write_to_buffer("mov rax, [rbp + ", 16, output, compiler);
            nums_to_str(var_node->offset, &temp, output, compiler);
            write_to_buffer("]\n", 2, output, compiler);
        }
        else if (var_node->where_it_is_stored == STORE_IN_REGISTER)
        {
            write_to_buffer("mov rax, ", 9, output, compiler);
            const char* reg_name = reg[var_node->register_location];
            write_to_buffer(reg_name, strlen(reg_name), output, compiler);
            write_to_buffer("\n", 1, output, compiler);
        }

        printf("Variable name: %.*s\n", (int)expr->variable.length, expr->variable.name);
        printf("Variable offset string: %llu\n", temp);
        printf("done");
        return;
    }
    case EXPR_BINARY:
        evaluate_bin(expr, compiler, output, conditional);
        break;

    case EXPR_UNARY:
        evaluate_unary(expr, compiler, output);
        break;
    
    case EXPR_FUNCTION_CALL:
    {
        size_t param_count = expr->func_call.parameter_count;
        write_to_buffer("push r9\npush r8\npush rcx\npush rdx\npush rsi\npush rdi\n", 52, output, compiler);
        
        if (expr->func_call.parameter_count <= 6) {
            for (size_t i = 0; i < param_count; i++)
            {
                evaluate_expression_x86_64(&(expr->func_call.arguments[i]), compiler, output, false);
                write_to_buffer("mov ", 4, output, compiler);
                write_to_buffer(reg[i], 3, output, compiler);
                write_to_buffer(", rax\n", 6, output, compiler);
                
            }
            
        }
        else {
            size_t i = 0;
            while (i <= 6)
            {
                evaluate_expression_x86_64(&(expr->func_call.arguments[i]), compiler, output, false);
                write_to_buffer("mov ", 4, output, compiler);
                write_to_buffer(reg[i], 3, output, compiler);
                write_to_buffer(", rax\n", 6, output, compiler);
                i++;
            }
            for (size_t j = param_count - 1; j > 6; j--)
            {
                evaluate_expression_x86_64(&(expr->func_call.arguments[i]), compiler, output, false);
                write_to_buffer("push rax\n", 9, output, compiler);
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


int evaluate_bin(expression* binary_exp, Compiler* compiler, FILE* output, bool conditional)
{
    evaluate_expression_x86_64(binary_exp->binary.left, compiler, output, false); //left value at rsp
    write_to_buffer("\npush rax\n", 10, output, compiler);
    evaluate_expression_x86_64(binary_exp->binary.right, compiler, output, false); // right value at rax
    switch (binary_exp->binary.op)
    {
    case TOK_ADD:
        write_to_buffer("\npop rbx\nadd rax, rbx\n", 22, output, compiler);
        break;
    case TOK_SUB:
        write_to_buffer("\npop rbx\nxchg rax, rbx\nsub rax, rbx\n", 35, output, compiler);
        break;
    case TOK_MUL:
        write_to_buffer("\npop rbx\nmul rbx\n", 17, output, compiler);
        break;
    case TOK_DIV:
        // cmp rbx, 0
        // je err_div0
        write_to_buffer("\npop rbx\nxchg rax, rbx\ndiv rax, rbx\n", 35, output, compiler);
        break;
    case TOK_EQ:
        write_to_buffer("\npop rbx\ncmp rax, rbx\n", 22, output, compiler);
        if (!conditional) // store in rax
        {
            write_to_buffer("sete al\nmovzx rax, al\n", 22, output, compiler);
        }
        else {   // jump if equal
            write_to_buffer("jne ", 4, output, compiler);
        }
        break;

    default:
        return 0;
    }
    return 0; // warning suppression
}


int evaluate_unary(expression* unary_exp, Compiler* compiler, FILE* output)
{
    switch (unary_exp->unary.op)
    {
    case TOK_SUB:
        evaluate_expression_x86_64(unary_exp->unary.operand, compiler, output, false);
        write_to_buffer("neg rax\n", 8, output, compiler);
        break;
    
    default:
        return 0;
    }
    return 0;
}

