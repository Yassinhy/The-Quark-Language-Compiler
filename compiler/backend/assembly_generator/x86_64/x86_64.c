#include "backend/assembly_generator/x86_64/evaluate_expr.h"
#include "utilities/utils.h"
#include "error_handler/error_handler.h"
#include "backend/assembly_generator/x86_64/x86_64.h"
#include "symbol_table/symbol_table.h"
#include <stddef.h>
#include <stdio.h>
// #include "frontend/expression_creation/expressions.h"

void write_to_buffer(const char* code, size_t code_length, FILE* output, Compiler* compiler){
    if (code_length + compiler->currentsize >= compiler->capacity)
    {
        //flush buffer to file
        fwrite(compiler->buffer, 1, compiler->currentsize, output);
        compiler->currentsize = 0;
    }
    printf("----->%s\n", code);
    memcpy(compiler->buffer + compiler->currentsize, code, code_length);
    compiler->currentsize += code_length;
}


static char num_buffer[32];

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
    printf("here\n");
    function_node* func_node = stmt->stmnt_function_declaration.function_node;
    printf("here 2\n");
    write_to_buffer(func_node->name, func_node->name_length, output, compiler);
    printf("here 3\n");
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

    write_to_buffer("ret\n", 4, output, compiler);
    exit_current_scope(compiler);
}

static void generate_statement_code(statement* stmt, size_t* num_len, FILE* output, Compiler* compiler) {
    printf("arrived\n");
    switch (stmt->type)
    {
    case STMT_EXIT:
        {
            // printf("exit code type: %d\n", stmt->stmnt_exit.exit_code->type);
            evaluate_expression_x86_64(stmt->stmnt_exit.exit_code, compiler, output, false);
            write_to_buffer("\nmov rdi, rax\nmov rax, 60", 25, output, compiler);
            write_to_buffer("\nsyscall\n", 9, output, compiler);
            break;
        }
    
    case STMT_LET:
        {
            printf("broke here\n");
            evaluate_expression_x86_64(stmt->stmnt_let.value, compiler, output, false);
            write_to_buffer("\n", 1, output, compiler);
            write_to_buffer("mov [rbp - ", 11, output, compiler);
            nums_to_str(find_variable(compiler, stmt->stmnt_let.hash, stmt->stmnt_let.name, stmt->stmnt_let.name_length)->offset, num_len, output, compiler);
            write_to_buffer("], rax\n", 7, output, compiler);
            break;
        }
    case STMT_ASSIGNMENT:
        {
            evaluate_expression_x86_64(stmt->stmnt_assign.value, compiler, output, false); // now the expression is in rax
            symbol_node* var_node = find_variable(compiler, stmt->stmnt_assign.hash, stmt->stmnt_assign.name, stmt->stmnt_assign.name_length);
            if (!var_node) panic(ERROR_UNDEFINED, "Variable not declared before assignment", compiler);
            if (var_node->where_it_is_stored == STORE_IN_STACK)
            {
                write_to_buffer("\nmov [rbp - ", 12, output, compiler);
                nums_to_str(find_variable(compiler, stmt->stmnt_assign.hash, stmt->stmnt_assign.name, stmt->stmnt_assign.name_length)->offset, num_len, output, compiler);
                write_to_buffer("], rax\n", 7, output, compiler);
            }
            else if (var_node->where_it_is_stored == STORE_IN_REGISTER)
            {
                write_to_buffer("mov ", 4, output, compiler);
                write_to_buffer(reg[var_node->register_location], 3, output, compiler);
                write_to_buffer(", rax\n", 6, output, compiler);
            }
            else if (var_node->where_it_is_stored == STORE_IN_FLOAT_REGISTER)
            {
                panic(ERROR_UNDEFINED, "floats still not implemented", compiler);
            }
            else{
                panic(ERROR_UNDEFINED, "where is the var stored?", compiler);
            }
            break;
        }
    case STMT_IF:
        evaluate_expression_x86_64(stmt->stmnt_if.condition, compiler, output, true);
        // till now what is printed:
        //cmp rax, rbx
        //jne 
        
        if (stmt->stmnt_if.or_else)
        {
            write_to_buffer(".Lelse_", 5, output, compiler);
            nums_to_str(compiler->if_statements, num_len, output, compiler);
            write_to_buffer("\n", 1, output, compiler);
            // jne .Lelse_3 for example
    
            generate_block_code(stmt->stmnt_if.then, num_len, output, compiler);
            // now jump to endif
            write_to_buffer("jmp .end_if_", 12, output, compiler);
            nums_to_str(compiler->if_statements, num_len, output, compiler);
            write_to_buffer("\n", 1, output, compiler);
            // now the else block
            write_to_buffer(".Lelse_", 5, output, compiler);
            nums_to_str(compiler->if_statements, num_len, output, compiler);
            write_to_buffer(":\n", 2, output, compiler);
            // .Lelse_3:\n
            generate_block_code(stmt->stmnt_if.or_else, num_len, output, compiler);
        }
        
        else {
            write_to_buffer(".end_if_", 8, output, compiler);
            nums_to_str(compiler->if_statements, num_len, output, compiler);
            write_to_buffer("\n", 1, output, compiler);
            // jne .Lelse_3 for example
    
            generate_block_code(stmt->stmnt_if.then, num_len, output, compiler);
        }

        // now the end_if
        write_to_buffer(".end_if_", 8, output, compiler);
        nums_to_str(compiler->if_statements, num_len, output, compiler);
        write_to_buffer(":\n", 2, output, compiler);
        compiler->if_statements++;
        break;
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
    nums_to_str(compiler->symbol_table_stack->storage[0]->scope_offset, &num_len, output, compiler);

    write_to_buffer("\n", 1, output, compiler);
    while (i < AST->node_count)
    {
        printf("\n\n\n\n______________________________\n\n%i\n\n____________________\n\n\n", AST->nodes[i]->type);
        switch (AST->nodes[i]->type)    
        {
        
        case NODE_STATEMENT:
            generate_statement_code(AST->nodes[i]->stmnt, &num_len, output, compiler);
            break;
        case NODE_EXPRESSION:
            // This handles top-level expressions like "my_function();"
            evaluate_expression_x86_64(AST->nodes[i]->expr, compiler, output, false);
            break;
        default:
            break;
        }
        i++;
    }
    i = 0;
    printf("----------------------------------------------------------\n");
    write_to_buffer("\n; Exit program\nmov rax, 60\nmov rdi, 0\nsyscall", 46, output, compiler);
    write_to_buffer("\n\n\n\n", 4, output, compiler);
    while (i < AST->node_count)
    {
        // Check if the node is actually a statement first
    if (AST->nodes[i]->type == NODE_STATEMENT) 
    {
        // USE ->stmnt, NOT ->expr
        switch (AST->nodes[i]->stmnt->type) 
        {
        case STMT_FUNCTION: // Use the correct enum (was STMT_FUNCTION in struct, make sure enums match)
            generate_function_code(AST->nodes[i]->stmnt, &num_len, output, compiler);
            break;
        default:
            break;
        }
    }
    i++;
    }


    //flush remaining buffer to file
    write_to_buffer("\n; Exit program\nmov rax, 60\nmov rdi, 0\nsyscall", 46, output, compiler);

    write_to_buffer("\nsection .rodata\nerr_div0:\tdb \"division by zero\", 10\nerr_div0_len:  equ $ - err_div0\n", 85, output, compiler);
    fwrite(compiler->buffer, 1, compiler->currentsize, output);
    fclose(output);
    //gcc phc.c -o phc && ./phc test.ph
    //nasm -f elf64 output.asm && ld output.o -o output && ./output
}