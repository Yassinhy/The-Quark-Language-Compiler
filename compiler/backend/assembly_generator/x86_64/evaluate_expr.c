#include "backend/assembly_generator/x86_64/evaluate_expr.h"
#include "backend/assembly_generator/x86_64/x86_64.h"
// #include "frontend/expression_creation/expressions.h"
#include "symbol_table/symbol_table.h"
#include "utilities/utils.h"
#include "error_handler/error_handler.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 256

const char* regs32[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
const char* regs64[] = {"rax", "rbx", "rcx", "rdx","rsi", "rdi", "r8 ", "r9 ", "r10", "r11", "r12", "r13", "r14", "r15"};

const char* get_reg( int reg_index, data_type* type) {
    if (Data_type_sizes_from_data_types[type->general_data_type] == 4) {
        return regs32[reg_index];
    } else {
        return regs64[reg_index];
    }
}

const char* size_prefix[] = {[1] = "byte", [2] = "word", [4] = "dword", [8] = "qword"};

typedef enum{
    CONVERT_ZERO_EXTEND,
    CONVERT_SIGN_EXTEND,
    CONVERT_TRUNCATE,
    CONVERT_NONE,
} Conversion_type;

Conversion_type find_conversion_type(data_type* to, data_type* from){
    if (Data_type_sizes_from_data_types[from->general_data_type] == Data_type_sizes_from_data_types[to->general_data_type])
        return CONVERT_NONE;

    if (Data_type_sizes_from_data_types[from->general_data_type] < Data_type_sizes_from_data_types[to->general_data_type]) {
        if (Data_is_signed[from->general_data_type])
            return CONVERT_SIGN_EXTEND;
        else
            return CONVERT_ZERO_EXTEND;
    }

    return CONVERT_TRUNCATE;
}


typedef enum
{
    REG_RAX = 0,
    REG_RBX = 1,
    REG_R10 = 8,
    REG_R11 = 9,
    REG_R12 = 10,

} Reg_type;

static void load_address_from_storage( symbol_node* var_node, Compiler* compiler, FILE* output, char* buffer, int reg_index)
{
    switch (var_node->where_it_is_stored) {
        case STORE_IN_STACK: {
            int len = snprintf(buffer, BUFFER_SIZE, "lea %s, [rbp - ",  regs64[reg_index]);
            write_to_buffer(buffer, len, output, compiler);
            nums_to_str(var_node->offset, output, compiler);
            write_to_buffer("]\n", 2, output, compiler);
            break;
        }

        case STORE_AS_PARAM: {
            int len2 = snprintf(buffer, BUFFER_SIZE, "lea %s, [rbp + ",  regs64[reg_index]);
            write_to_buffer(buffer, len2, output, compiler);
            nums_to_str(var_node->param_offset, output, compiler);
            write_to_buffer("]\n", 2, output, compiler);
            break;
        }

        default:
            panic(ERROR_INTERNAL, "Trying to get address of a variable that is not stored in memory", compiler);
    }
}


static void load_variable_from_storage( symbol_node* var_node, data_type* wanted_output_result, Compiler* compiler, FILE* output, char* buffer, int reg_index)
{

    Conversion_type conversion = find_conversion_type(wanted_output_result, var_node->data_type);
    printf("WANTED OUTPUT RESULT IS: %i\n", wanted_output_result->general_data_type);
    switch (var_node->where_it_is_stored) {
        case STORE_IN_STACK:
            if (conversion == CONVERT_ZERO_EXTEND) {
                int len = snprintf(buffer, BUFFER_SIZE, "movzx %s, %s[rbp - ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                write_to_buffer(buffer, len, output, compiler);
                nums_to_str(var_node->offset, output, compiler);
                write_to_buffer("]\n", 2, output, compiler);
            }

            else if (conversion == CONVERT_SIGN_EXTEND) {
                if (Data_type_sizes_from_data_types[var_node->data_type->general_data_type] < 4){
                    int len = snprintf(buffer, BUFFER_SIZE, "movsx %s, %s[rbp - ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                    write_to_buffer(buffer, len, output, compiler);
                    nums_to_str(var_node->offset, output, compiler);
                    write_to_buffer("]\n", 2, output, compiler);
                }
                else {
                    int len = snprintf(buffer, BUFFER_SIZE, "movsxd %s, %s[rbp - ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                    write_to_buffer(buffer, len, output, compiler);
                    nums_to_str(var_node->offset, output, compiler);
                    write_to_buffer("]\n", 2, output, compiler);
                }
            }
            
            else if (conversion == CONVERT_TRUNCATE) {
                warning("Truncation of the expression has happened", compiler);
                int len = snprintf(buffer, BUFFER_SIZE, "mov %s, %s[rbp - ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                write_to_buffer(buffer, len, output, compiler);
                nums_to_str(var_node->offset, output, compiler);
                write_to_buffer("]\n", 2, output, compiler);
            }

            else {
                int len;
                if (var_node->data_type->data_type_family == FAMILY_ARRAY) {
                    // array decay: caller wants the address, not a value
                    len = snprintf(buffer, BUFFER_SIZE, "lea %s, [rbp - ", regs64[reg_index]);
                }
                else {
                    len = snprintf(buffer, BUFFER_SIZE, "mov %s, %s[rbp - ",
                        get_reg(reg_index, wanted_output_result),
                        size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                }
                write_to_buffer(buffer, len, output, compiler);
                nums_to_str(var_node->offset, output, compiler);
                write_to_buffer("]\n", 2, output, compiler);
            }
            break;
            
        case STORE_AS_PARAM:
            if (conversion == CONVERT_ZERO_EXTEND) {
                int len = snprintf(buffer, BUFFER_SIZE, "movzx %s, %s[rbp + ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                write_to_buffer(buffer, len, output, compiler);
                nums_to_str(var_node->param_offset, output, compiler);
                write_to_buffer("]\n", 2, output, compiler);
            }

            else if (conversion == CONVERT_SIGN_EXTEND) {
                if (Data_type_sizes_from_data_types[wanted_output_result->general_data_type] < 4){
                    int len = snprintf(buffer, BUFFER_SIZE, "movsx %s, %s[rbp + ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                    write_to_buffer(buffer, len, output, compiler);
                    nums_to_str(var_node->param_offset, output, compiler);
                    write_to_buffer("]\n", 2, output, compiler);
                }
                else {
                    int len = snprintf(buffer, BUFFER_SIZE, "movsxd %s, %s[rbp + ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                    write_to_buffer(buffer, len, output, compiler);
                    nums_to_str(var_node->param_offset, output, compiler);
                    write_to_buffer("]\n", 2, output, compiler);
                }
            }
            
            else if (conversion == CONVERT_TRUNCATE) {
                warning("Truncation of the expression has happened", compiler);
                int len = snprintf(buffer, BUFFER_SIZE, "mov %s, %s[rbp + ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                write_to_buffer(buffer, len, output, compiler);
                nums_to_str(var_node->param_offset, output, compiler);
                write_to_buffer("]\n", 2, output, compiler);
            }

            else {
                int len = snprintf(buffer, BUFFER_SIZE, "mov %s, %s[rbp + ",  get_reg(reg_index, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
                write_to_buffer(buffer, len, output, compiler);
                nums_to_str(var_node->param_offset, output, compiler);
                write_to_buffer("]\n", 2, output, compiler);
            }
            break;
            
        case STORE_IN_REGISTER:
            if (conversion == CONVERT_ZERO_EXTEND) {
                int len = snprintf(buffer, BUFFER_SIZE, "movzx %s, %s\n",  get_reg(reg_index, wanted_output_result), get_reg(2 + var_node->register_location, wanted_output_result));
                write_to_buffer(buffer, len, output, compiler);
            }

            else if (conversion == CONVERT_SIGN_EXTEND) {
                if (Data_type_sizes_from_data_types[var_node->data_type->general_data_type] < 4){
                    int len = snprintf(buffer, BUFFER_SIZE, "movsx %s, %s\n",  get_reg(reg_index, wanted_output_result), get_reg(2 + var_node->register_location, wanted_output_result));
                    write_to_buffer(buffer, len, output, compiler);
                }
                else {
                    int len = snprintf(buffer, BUFFER_SIZE, "movsxd %s, %s\n",  get_reg(reg_index, wanted_output_result), get_reg(2 + var_node->register_location, wanted_output_result));
                    write_to_buffer(buffer, len, output, compiler);
                }
            }
            
            else if (conversion == CONVERT_TRUNCATE) {
                warning("Truncation of the expression has happened", compiler);
                int len = snprintf(buffer, BUFFER_SIZE, "mov %s, %s\n",  get_reg(reg_index, wanted_output_result), get_reg(2 + var_node->register_location, wanted_output_result));
                write_to_buffer(buffer, len, output, compiler);
            }

            else {
                int len = snprintf(buffer, BUFFER_SIZE, "mov %s, %s\n",  get_reg(reg_index, wanted_output_result), get_reg(2 + var_node->register_location, wanted_output_result));
                write_to_buffer(buffer, len, output, compiler);
            }
            break;
            
        default:
            panic(ERROR_UNDEFINED, "Unknown variable storage type", compiler);
    }
}

const bool is_signed_type[] = {
    [DATA_TYPE_CHAR] = true,
    [DATA_TYPE_INT] = true,
    [DATA_TYPE_LONG] = true,
    [DATA_TYPE_POINTER] = false,
    [DATA_TYPE_ARRAY] = false,
     // add other types as needed
};

const char* get_load_instruction(data_type* type, Compiler* compiler) {
    size_t size = get_data_type_size(type, compiler);
    bool is_signed = is_signed_type[type->general_data_type];
    switch (size) {
        case 1: 
            if (is_signed) {
                return "movsx al,";
            } else {
                return "movzx al,";
            }
        case 2: 
            if (is_signed) {
                return "movsx ax,";
            } else {
                return "movzx ax,";
            }
        case 4: return "mov eax,";
        case 8: return "mov rax,";
    }
}

void evaluate_expression_x86_64(expression* expr, Compiler* compiler, FILE* output, int conditional, data_type* wanted_output_result)
{
    size_t num_len;
    char buffer[BUFFER_SIZE];
    
    int len;
    switch (expr->type)
    {
    
    case EXPR_INT:
    {

        len = snprintf(buffer, BUFFER_SIZE, "mov %s, ", get_reg(0, wanted_output_result));
        write_to_buffer(buffer, len, output, compiler);

        char* temp2 = _u64_to_str(expr->integer.value, &num_len);
        write_to_buffer(temp2, num_len, output, compiler);
        write_to_buffer("\n", 1, output, compiler);
        return;
    }

    case EXPR_ADDRESS:
    {
        symbol_node* var_node = expr->address.operand->variable.node_in_table;
        if (compiler->return_context) {
            warning( "function returns address of local variable-- it will be invalid after the function returns", compiler);
        }
        switch (var_node->where_it_is_stored) {
            case STORE_IN_REGISTER:
            {
                len = snprintf(buffer, BUFFER_SIZE, "mov [rbp - %lu], %s\n", expr->address.stack_offset, get_reg(var_node->register_location + 2, var_node->data_type));
                write_to_buffer(buffer, len, output, compiler);
                var_node->where_it_is_stored = STORE_IN_STACK;
                var_node->offset = expr->address.stack_offset;
                len = snprintf(buffer, BUFFER_SIZE, "lea rax, [rbp - %lu]\n", var_node->offset);
                write_to_buffer(buffer, len, output, compiler);
                break;
            }

            case STORE_IN_STACK:
            {
                len = snprintf(buffer, BUFFER_SIZE, "lea rax, [rbp - %lu]\n", var_node->offset);
                write_to_buffer(buffer, len, output, compiler);
                break;
            }

            case STORE_AS_PARAM:
            {
                len = snprintf(buffer, BUFFER_SIZE, "lea rax, [rbp + %lu]\n", var_node->param_offset);
                write_to_buffer(buffer, len, output, compiler);
                break;
            }

            default:
                panic(ERROR_LOGICAL, "Trying to obtain the adrress of an invalidly stored variable", compiler);
        }
        break;
    }

    case EXPR_POINTER_DEREF:
    {
        evaluate_expression_x86_64(expr->dereference.operand, compiler, output, conditional, expr->dereference.operand->result_type);
        len = snprintf(buffer, BUFFER_SIZE, "mov %s, %s[rax]\n", get_reg(REG_RAX, wanted_output_result), size_prefix[Data_type_sizes_from_data_types[wanted_output_result->general_data_type]]);
        write_to_buffer(buffer, len, output, compiler);
        break;
    }

    case EXPR_IDENTIFIER: 
    {

        symbol_node* var_node = expr->variable.node_in_table;
        if (var_node == NULL) {
            fprintf(stderr, "ICE: variable '%s' has no symbol table entry\n",
                    expr->variable.name);   // adjust field names to match your struct
            exit(1);
        }
        write_to_buffer("; Starting to evaluate\n", 23, output, compiler);
        load_variable_from_storage(var_node, wanted_output_result, compiler, output, buffer, REG_RAX);
      
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
        data_type* long_data_type = malloc(sizeof(data_type));
        long_data_type->general_data_type = DATA_TYPE_LONG;
        size_t param_count = expr->func_call.parameter_count;
        write_to_buffer("push r9\npush r8\npush rcx\npush rdx\npush rsi\npush rdi\n", 52, output, compiler);
        
        if (expr->func_call.parameter_count <= 6) {
            
            for (size_t i = 0; i < param_count; i++)
            {
                data_type* argument_data_type = expr->func_call.arguments[i].result_type;

                evaluate_expression_x86_64(&(expr->func_call.arguments[i]), compiler, output, false, argument_data_type);
                len = snprintf(buffer, BUFFER_SIZE, "push rax\n");
                write_to_buffer(buffer, len, output, compiler);
                
            }

            for (int i = (int)param_count - 1; i >= 0; i--)
            {
                len = snprintf(buffer, BUFFER_SIZE, "pop %s\n", get_reg(i + 2, long_data_type));
                write_to_buffer(buffer, len, output, compiler);   
            }
            
        }
        else {
            for (int i = 0; i < 6; i++)
            {
                data_type* argument_data_type = expr->func_call.arguments[i].result_type;

                evaluate_expression_x86_64(&(expr->func_call.arguments[i]), compiler, output, false, argument_data_type);
                len = snprintf(buffer, BUFFER_SIZE, "push rax\n");
                write_to_buffer(buffer, len, output, compiler);
                
            }

            for (int i = 5; i >= 0; i--)
            {
                len = snprintf(buffer, BUFFER_SIZE, "pop %s\n", get_reg(i + 2, long_data_type));
                write_to_buffer(buffer, len, output, compiler);   
            }
/////////////////////////////////////////
            for (size_t j = param_count - 1; j >= 6; j--)
            {
                data_type* argument_data_type = expr->func_call.arguments[j].result_type;

                evaluate_expression_x86_64(&(expr->func_call.arguments[j]), compiler, output, false, argument_data_type);
                len = snprintf(buffer, BUFFER_SIZE, "push rax\n");
                write_to_buffer(buffer, len, output, compiler);
            }
            
            
        }
        write_to_buffer("call ", 5, output, compiler);
        write_to_buffer(expr->func_call.name, expr->func_call.name_length, output, compiler);

        if (strncmp(expr->func_call.name, "main", expr->func_call.name_length) == 0) {
            write_to_buffer("\n", 1, output, compiler);
        }
        else {
            write_to_buffer("_quark\n", 7, output, compiler);
        }

        write_to_buffer("pop rdi\npop rsi\npop rdx\npop rcx\npop r8\npop r9\n", 46, output, compiler);
       
        free(long_data_type);
        break;
    }

    case EXPR_ARR_INDEX:
    {
        if (expr->array_index.array->type == EXPR_IDENTIFIER) {
            symbol_node* var_node = expr->array_index.array->variable.node_in_table;
            int element_size = get_data_type_size(expr->result_type, compiler);

             if (var_node->data_type->data_type_family == FAMILY_ARRAY) {
                load_address_from_storage(var_node, compiler, output, buffer, REG_R10);
            } else {
                load_variable_from_storage(var_node, var_node->data_type, compiler, output, buffer, REG_R10);
            }

            len = snprintf(buffer, BUFFER_SIZE, "push r10\n"); // save base
            write_to_buffer(buffer, len, output, compiler);

            evaluate_expression_x86_64(expr->array_index.index, compiler, output, false, expr->array_index.index->result_type);       // index → rax, may trash r10
            len = snprintf(buffer, BUFFER_SIZE, "pop r10\n");  // restore base
            write_to_buffer(buffer, len, output, compiler);
            len = snprintf(buffer, BUFFER_SIZE, "imul rax, %d\n", element_size);
            write_to_buffer(buffer, len, output, compiler);
            len = snprintf(buffer, BUFFER_SIZE, "add rax, r10\n");
            write_to_buffer(buffer, len, output, compiler);
        }


        else {
            // now the base node is at rax
            evaluate_expression_x86_64(expr->array_index.array, compiler, output, false, expr->array_index.array->result_type);
            int element_size = get_data_type_size(expr->result_type, compiler);
            len = snprintf(buffer, BUFFER_SIZE, "push rax\n"); // save base
            write_to_buffer(buffer, len, output, compiler);
            evaluate_expression_x86_64(expr->array_index.index, compiler, output, false, expr->array_index.index->result_type);
            len = snprintf(buffer, BUFFER_SIZE, "pop r10\n");  // restore base
            write_to_buffer(buffer, len, output, compiler);
            len = snprintf(buffer, BUFFER_SIZE, "imul rax, %d\n", element_size);
            write_to_buffer(buffer, len, output, compiler);
            len = snprintf(buffer, BUFFER_SIZE, "add rax, r10\n");
            write_to_buffer(buffer, len, output, compiler);
        }
        if (expr->result_type->data_type_family != FAMILY_ARRAY) {
            len = snprintf(buffer, BUFFER_SIZE, "%s %s[rax]\n", get_load_instruction(expr->result_type, compiler), size_prefix[Data_type_sizes_from_data_types[expr->result_type->general_data_type]]);
            write_to_buffer(buffer, len, output, compiler);
        }
        break;
    }

    case EXPR_INIT_LIST:
        panic(ERROR_ARGUMENT_COUNT, "Initializer list cannot be evaluated", compiler);
    default:
        panic(ERROR_UNDEFINED, "Unexpected expression, may still not be emplemented", compiler);
    }
}


int evaluate_bin(expression* binary_exp, Compiler* compiler, FILE* output, int conditional, data_type* wanted_output_result)
{
    char buffer[BUFFER_SIZE];
    evaluate_expression_x86_64(binary_exp->binary.right, compiler, output, false, wanted_output_result); //right value

    int len0 = snprintf(buffer, BUFFER_SIZE, "push rax\n");
    write_to_buffer(buffer, len0, output, compiler);

    evaluate_expression_x86_64(binary_exp->binary.left, compiler, output, false, wanted_output_result); // left value

    switch (binary_exp->binary.op)
    {
    case TOK_ADD: {
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\nadd %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    }
    case TOK_SUB:{
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\nsub %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    }
    case TOK_MUL:{
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\nimul %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    }
    case TOK_DIV:{
        int len = snprintf(buffer, BUFFER_SIZE, "pop rbx\n");
        write_to_buffer(buffer, len, output, compiler);

        // Sign extend RAX into RDX (Required for idiv)
        if (wanted_output_result->general_data_type == DATA_TYPE_INT) {
            write_to_buffer("cdq\n", 4, output, compiler); // EAX -> EDX:EAX
            write_to_buffer("idiv ebx\n", 9, output, compiler);
        } else {
            write_to_buffer("cqo\n", 4, output, compiler); // RAX -> RDX:RAX
            write_to_buffer("idiv rbx\n", 9, output, compiler);
        }
        break;
    }

    case TOK_PERCENT:{
        int len = snprintf(buffer, BUFFER_SIZE, "pop rbx\n");
        write_to_buffer(buffer, len, output, compiler);

        // 2. Sign extend RAX into RDX (Required for idiv)
        if (wanted_output_result->general_data_type == DATA_TYPE_INT) {
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
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\ncmp %s, %s\n", get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, BUFFER_SIZE, "sete al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
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
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, BUFFER_SIZE, "setne al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
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
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, BUFFER_SIZE, "setg al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
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
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, BUFFER_SIZE, "setl al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
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
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, BUFFER_SIZE, "setge al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
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
        int len0 = snprintf(buffer, BUFFER_SIZE, "pop rbx\ncmp %s, %s\n",  get_reg(0,  wanted_output_result),  get_reg(1,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        if (conditional == 0) // store in rax
        {
            int len0 = snprintf(buffer, BUFFER_SIZE, "setle al\nmovzx %s, al\n", get_reg(0,  wanted_output_result));
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


int evaluate_unary(expression* unary_exp, Compiler* compiler, FILE* output, data_type* wanted_output_result)
{
    char buffer[BUFFER_SIZE];
    switch (unary_exp->unary.op)
    {
    case TOK_SUB:
        evaluate_expression_x86_64(unary_exp->unary.operand, compiler, output, false, wanted_output_result);
        int len0 = snprintf(buffer, BUFFER_SIZE, "neg %s\n", get_reg(0,  wanted_output_result));
        write_to_buffer(buffer, len0, output, compiler);
        break;
    
    default:
        return 0;
    }
    return 0;
}

