#include "backend/assembly_generator/x86_64/evaluate_expr.h"
#include "utilities/utils.h"
#include "error_handler/error_handler.h"
#include "backend/assembly_generator/x86_64/x86_64.h"
#include "symbol_table/symbol_table.h"
#include <stddef.h>
#include <stdio.h>
// #include "frontend/expression_creation/expressions.h"


const char* regs32_x86[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "r8d", "r9d"};
const char* regs64_x86[] = {"rax", "rbx", "rdi", "rsi","rdx", "rcx", "r8 ", "r9 "};

const char* get_reg_x86( int reg_index, Data_type type) {
    if (type == DATA_TYPE_INT) {
        return regs32_x86[reg_index];
    } else {
        return regs64_x86[reg_index];
    }
}

void write_to_buffer(const char* code, size_t code_length, FILE* output, Compiler* compiler){
    if (code_length + compiler->currentsize >= compiler->capacity)
    {
        //flush buffer to file
        fprintf(stderr, "FLUSHING: %zu bytes\n", compiler->currentsize);
        fwrite(compiler->buffer, 1, compiler->currentsize, output);
        fflush(output);  // ← Add this!
        compiler->currentsize = 0;
    }
    memcpy(compiler->buffer + compiler->currentsize, code, code_length);
    compiler->currentsize += code_length;
}


static char num_buffer[32];
static char buffer[128];


char* _u64_to_str(size_t number, size_t* num_len)
{
    // num buffer is 32 bytes long to hold max u64 value plus null terminator
    char* end = &num_buffer[31];
    *num_len = 0;
    *end = '\0';
    end--;
    
    do {
        *end = '0' + (number % 10);
        number /= 10;
        end--;
        (*num_len)++;
    } while (number);
    return end + 1;
}


void nums_to_str(size_t number, size_t* num_len, FILE* output, Compiler* compiler)
{
    char* num_as_str = _u64_to_str(number, num_len);
    write_to_buffer(num_as_str, *num_len, output, compiler);
}

static void generate_statement_code(statement* stmt, size_t* num_len, FILE* output, Compiler* compiler);

void enter_existing_scope(symbol_table* new_table, bool independent, Compiler* compiler) {
    if (!compiler) panic(ERROR_UNDEFINED, "Compiler not initialized", compiler);
    if (compiler->symbol_table_stack->current_size + 1 >= compiler->symbol_table_stack->capacity) {
        compiler->symbol_table_stack->storage = realloc(compiler->symbol_table_stack->storage, compiler->symbol_table_stack->capacity * 2 * sizeof(symbol_table*));
        compiler->symbol_table_stack->capacity *= 2;
    }
    if (!compiler->symbol_table_stack->storage) panic(ERROR_MEMORY_ALLOCATION, "Entered too many scopes for your memory", compiler);

    if (!new_table) panic(ERROR_MEMORY_ALLOCATION, "New symbol table allocation failed", compiler);
    
    if (independent) new_table->parent_scope = compiler->symbol_table_stack->storage[0];
    else new_table->parent_scope = peek_symbol_stack(compiler);
    compiler->symbol_table_stack->storage[compiler->symbol_table_stack->current_size] = new_table;
    compiler->symbol_table_stack->current_size++;
}

static void generate_block_code (statement* stmt, size_t* num_len, FILE* output, Compiler* compiler) {
    for (size_t i = 0; i < stmt->stmnt_block.statement_count; i++)
    {
        generate_statement_code(stmt->stmnt_block.statements[i], num_len, output, compiler);
    }
    
}

static inline void generate_function_code(statement* stmt, size_t* num_len, FILE* output, Compiler* compiler) {
    function_node* func_node = stmt->stmnt_function_declaration.function_node;
    write_to_buffer(func_node->name, func_node->name_length, output, compiler);
    write_to_buffer(":\n", 2, output, compiler);
    write_to_buffer("push rbp\nmov rbp, rsp\n", 22, output, compiler);

    enter_existing_scope(func_node->code_block->stmnt_block.table, true, compiler);

    write_to_buffer("sub rsp, ", 9, output, compiler);
    nums_to_str(func_node->code_block->stmnt_block.table->scope_offset, num_len, output, compiler);
    write_to_buffer("\n", 1, output, compiler);
    
    
    // for (size_t i = 0; i < func_node->code_block->stmnt_block.statement_count; i++)
    // {
    //     generate_statement_code(func_node->code_block->stmnt_block.statements[i], num_len, output, compiler);
    // }
    generate_block_code(func_node->code_block, num_len, output, compiler);

    exit_current_scope(compiler);
}

static void generate_statement_code(statement* stmt, size_t* num_len, FILE* output, Compiler* compiler) {
    switch (stmt->type)
    {
    case STMT_EXIT:
        {
            // always int
            evaluate_expression_x86_64(stmt->stmnt_exit.exit_code, compiler, output, 0, DATA_TYPE_INT);
            int len = snprintf(buffer, sizeof(buffer), "\nmov rdi, rax\nmov rax, 60\nsyscall\n");
            write_to_buffer(buffer, len, output, compiler);
            break;
        }

    case STMT_RETURN:
        evaluate_expression_x86_64(stmt->stmnt_return.value, compiler, output, false, stmt->stmnt_return.return_data_type); // now result is stored in rax
        write_to_buffer("leave\nret\n", 10, output, compiler);
        break;

    case STMT_LET:
        {
            symbol_node* var = find_variable(compiler, stmt->stmnt_let.hash, stmt->stmnt_let.name, stmt->stmnt_let.name_length);
            if (!var) {
                panic(ERROR_UNDEFINED_VARIABLE, "Variable not found", compiler);
            }

            evaluate_expression_x86_64(stmt->stmnt_let.value, compiler, output, 0, var->data_type);
            int len = snprintf(buffer, sizeof(buffer), "mov [rbp - %lu], %s\n", var->offset, get_reg_x86(0,  stmt->stmnt_let.value->result_type));
            write_to_buffer(buffer, len, output, compiler);
            break;
        }

    case STMT_ASSIGNMENT:
        {
            symbol_node* var = find_variable(compiler, stmt->stmnt_assign.hash, stmt->stmnt_assign.name, stmt->stmnt_assign.name_length);
            if (!var) {
                panic(ERROR_UNDEFINED_VARIABLE, "Variable not found", compiler);
            }

            evaluate_expression_x86_64(stmt->stmnt_assign.value, compiler, output, 0, var->data_type); // now the expression is in rax
            
            if (var->where_it_is_stored == STORE_IN_STACK)
            {
                int len = snprintf(buffer, sizeof(buffer), "mov [rbp - %lu], %s\n", var->offset, get_reg_x86(0,var->data_type));
                write_to_buffer(buffer, len, output, compiler);
            }
            else if (var->where_it_is_stored == STORE_IN_REGISTER)
            {
                if (var->data_type == DATA_TYPE_INT) {
                    int len = snprintf(buffer, sizeof(buffer), "mov %s, %s\n", get_reg_x86(2 + var->register_location,  var->data_type), get_reg_x86(0,  var->data_type));
                    write_to_buffer(buffer, len, output, compiler);
                }
            }
            else if (var->where_it_is_stored == STORE_IN_FLOAT_REGISTER)
            {
                panic(ERROR_UNDEFINED, "floats still not implemented", compiler);
            }
            else{
                panic(ERROR_UNDEFINED, "where is the var stored?", compiler);
            }
            break;
        }

    case STMT_IF: {

        size_t current_if_id = compiler->counters->if_statements++;
        push_to_if_stack(current_if_id, compiler);

        evaluate_expression_x86_64(stmt->stmnt_if.condition, compiler, output, 1, DATA_TYPE_INT);
        // till now what is printed:
        //cmp rax, rbx
        //jne 
        
        // where to jump if the condition is false
        if (stmt->stmnt_if.or_else) {
        write_to_buffer(".Lelse_", 7, output, compiler);
        } else {
            write_to_buffer(".end_if_", 8, output, compiler);
        }
        nums_to_str(current_if_id, num_len, output, compiler);
        write_to_buffer("\n", 1, output, compiler);

        // generate the code itself
        if (stmt->stmnt_if.then->type == STMT_BLOCK) generate_block_code(stmt->stmnt_if.then, num_len, output, compiler);
        else generate_statement_code(stmt->stmnt_if.then, num_len, output, compiler);

        // now that we have finished the then block, we go to endif, then we write the logic for else
        int len0 = snprintf(buffer, sizeof(buffer), "jmp .end_if_%lu\n", current_if_id);
        write_to_buffer(buffer, len0, output, compiler);

        if (stmt->stmnt_if.or_else)
        {
            // now the else block
            int len1 = snprintf(buffer, sizeof(buffer), ".Lelse_%lu:\n", current_if_id);
            write_to_buffer(buffer, len1, output, compiler);

            // .Lelse_3:\n
            if (stmt->stmnt_if.or_else->type == STMT_IF) { // else if case
                generate_statement_code(stmt->stmnt_if.or_else, num_len, output, compiler);
            } else { // plain else case
                if (stmt->stmnt_if.or_else->type == STMT_BLOCK) generate_block_code(stmt->stmnt_if.or_else, num_len, output, compiler);
                else generate_statement_code(stmt->stmnt_if.or_else, num_len, output, compiler);
                // compiler->if_statements++;
            }
            
        }

        // now the end_if
        write_to_buffer(".end_if_", 8, output, compiler);
        nums_to_str(peek_if_stack(compiler), num_len, output, compiler);
        write_to_buffer(":\n", 2, output, compiler);
        compiler->counters->if_statements++;
        pop_from_if_stack(compiler);
        break;
    }

    case STMT_BREAK:{
        size_t counter = peek_while_stack(compiler);
        int len1 = snprintf(buffer, sizeof(buffer), "jmp .end_while_loop_%lu\n", counter);
        write_to_buffer(buffer, len1, output, compiler);
        break;
    }
    
    case STMT_WHILE: {
        /*
        jmp .condition_{while loop counter}
        .while_loop_{while loop counter}:
        code block
        
        .condition_{while loop counter}:
        evaluate condition
        jne .loop_{while loop counter}

        .end_loop_{while loop counter}:
        */

        size_t counter = stmt->stmnt_while.counter;

        push_to_while_stack(counter, compiler);

        int len = snprintf(buffer, sizeof(buffer), "jmp .condition_while_%lu\n.while_loop_%lu:\n", counter, counter);
        write_to_buffer(buffer, len, output, compiler);

        generate_block_code(stmt->stmnt_while.body, num_len, output, compiler);

        pop_from_while_stack(compiler);

        int len0 = snprintf(buffer, sizeof(buffer), ".condition_while_%lu:\n", counter);
        write_to_buffer(buffer, len0, output, compiler);

        evaluate_expression_x86_64(stmt->stmnt_while.condition, compiler, output, 2, DATA_TYPE_INT); 

        int len1 = snprintf(buffer, sizeof(buffer), ".while_loop_%lu\n.end_while_loop_%lu:\n", counter, counter);
        write_to_buffer(buffer, len1, output, compiler);

        break;
    }
    default:
        break;
    }  
}

void generate_assembly_x86_64(const AST* AST, Compiler* compiler, FILE* output){
    output = fopen("output.asm", "wb"); //clear file
    size_t num_len = 0;
    size_t i = 0;
    write_to_buffer("section .text\n\tglobal _start\n_start:\n", 37, output, compiler);
    // will move to function handling later
    write_to_buffer("push rbp\nmov rbp, rsp\nsub rsp, ", 31, output, compiler);
    size_t glbl_scope_offset = compiler->symbol_table_stack->storage[0]->scope_offset;
    nums_to_str(((glbl_scope_offset + 7) & ~7), &num_len, output, compiler);

    write_to_buffer("\n", 1, output, compiler);
    while (i < AST->node_count)
    {
        switch (AST->nodes[i]->type)    
        {
        
        case NODE_STATEMENT:
            generate_statement_code(AST->nodes[i]->stmnt, &num_len, output, compiler);
            break;
        case NODE_EXPRESSION:
            // This handles top-level expressions like "my_function();"
            evaluate_expression_x86_64(AST->nodes[i]->expr, compiler, output, 0, DATA_TYPE_INT);
            break;
        default:
            break;
        }
        i++;
    }
    i = 0;
    write_to_buffer("\n; Exit program\nmov rax, 60\nmov rdi, 0\nsyscall", 46, output, compiler);
    write_to_buffer("\n\n\n\n", 4, output, compiler);
    while (i < AST->function_node_count)
    {
        generate_function_code(AST->function_nodes[i]->stmnt, &num_len, output, compiler);
        i++;
    }
    
    


    write_to_buffer("\nsection .rodata\nerr_div0:\tdb \"division by zero\", 10\nerr_div0_len:  equ $ - err_div0\n", 85, output, compiler);
    fwrite(compiler->buffer, 1, compiler->currentsize, output);
    fflush(output);
    fclose(output);
    //gcc phc.c -o phc && ./phc test.ph
    //nasm -f elf64 output.asm && ld output.o -o output && ./output
}