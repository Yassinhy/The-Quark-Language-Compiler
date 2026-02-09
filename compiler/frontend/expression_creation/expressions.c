#include "frontend/expression_creation/expressions.h"
#include "utilities/utils.h"
#include "arena/arena.h"
#include "symbol_table/symbol_table.h"
#include "error_handler/error_handler.h"
#include "frontend/parsing/parsing.h"
#include <stdio.h>

node* create_number_node(int value, Compiler* compiler)
{
    node* number_node = arena_alloc(compiler->expressions_arena, sizeof(node), compiler);
    number_node -> type = NODE_EXPRESSION;
    number_node -> expr = arena_alloc(compiler->expressions_arena, sizeof(expression), compiler);
    number_node -> expr -> type = EXPR_INT;
    number_node -> expr -> integer.value = value;
    // if(value < int max) int
    // else long
    number_node -> expr -> result_type = DATA_TYPE_INT;
    return number_node;
}

node* create_variable_node_dec(char* var_name, size_t length, variable_storage_type storage_type, normal_register reg_location, Data_type data_type, Compiler* compiler)
{
    node* var_node = arena_alloc(compiler->expressions_arena, sizeof(node), compiler);
    if(!var_node) return NULL;
    var_node->type = NODE_EXPRESSION;
    var_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression), compiler);
    if(!var_node->expr) return NULL;
    var_node->expr->variable.length = length;
    var_node->expr->variable.name = var_name;
    var_node->expr->variable.hash = hash_function(var_name, length);
    var_node->expr->type = EXPR_IDENTIFIER;
    var_node->expr->variable.data_type = data_type;
    var_node->expr->variable.node_in_table = add_var_to_current_scope(compiler, var_node->expr, storage_type, reg_location); // offset will be set later during code generation and size will be int for now (8 bytes)
    var_node->expr->result_type = data_type;
    return var_node;
}

node* create_unary_node(TokenType op, node* operand, Compiler* compiler)
{
    node* unary_node = arena_alloc(compiler->expressions_arena, sizeof(node), compiler);
    if(!unary_node) return NULL;
    unary_node->type = NODE_EXPRESSION;
    unary_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression), compiler);
    if(!unary_node->expr) return NULL;
    unary_node->expr->type = EXPR_UNARY;
    unary_node->expr->unary.op = op;
    unary_node->expr->unary.operand = operand->expr;
    unary_node->expr->result_type = operand->expr->result_type;
    return unary_node;
}


node* create_bin_node(node* left, TokenType op, Parser* parser, bool constant_foldable, Compiler* compiler)
{
    node* right_node = parse_expression(parser, presedences[op], constant_foldable, compiler);
    if (!right_node) {
        panic(ERROR_MEMORY_ALLOCATION, "Binary expression allocation failed", compiler);
    }

    
    node* bin_node = arena_alloc(compiler->expressions_arena, sizeof(node), compiler);
    
    if (!bin_node) {
        panic(ERROR_MEMORY_ALLOCATION, "Binary expression allocation failed", compiler);
    }

    
    bin_node->type = NODE_EXPRESSION;
    bin_node-> expr = arena_alloc(compiler->expressions_arena, sizeof(expression), compiler);
    
    if (!bin_node->expr) {
        panic(ERROR_MEMORY_ALLOCATION, "Binary expression allocation failed", compiler);
    }

    if (left->expr->result_type == DATA_TYPE_DOUBLE || right_node->expr->result_type == DATA_TYPE_DOUBLE) {
        if ((left->expr->result_type == DATA_TYPE_DOUBLE || left->expr->result_type == DATA_TYPE_FLOAT) && (right_node->expr->result_type == DATA_TYPE_DOUBLE || right_node->expr->result_type == DATA_TYPE_FLOAT))
        {
            bin_node->expr->result_type = DATA_TYPE_DOUBLE;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression, one is double and the other is not", compiler);
        }
    }
    else if (left->expr->result_type == DATA_TYPE_FLOAT || right_node->expr->result_type == DATA_TYPE_FLOAT) {
        if (left->expr->result_type == DATA_TYPE_FLOAT && right_node->expr->result_type == DATA_TYPE_FLOAT)
        {
            bin_node->expr->result_type = DATA_TYPE_FLOAT;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression, one is float and the other is not", compiler);
        }
        
    }
    else if (left->expr->result_type == DATA_TYPE_LONG || right_node->expr->result_type == DATA_TYPE_LONG) {
        if ((left->expr->result_type == DATA_TYPE_LONG || left->expr->result_type == DATA_TYPE_INT) && (right_node->expr->result_type == DATA_TYPE_LONG || right_node->expr->result_type == DATA_TYPE_INT))
        {
            bin_node->expr->result_type = DATA_TYPE_LONG;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression", compiler);
        }
    }
    else if (left->expr->result_type == DATA_TYPE_INT || right_node->expr->result_type == DATA_TYPE_INT) {
        if (left->expr->result_type == DATA_TYPE_INT && right_node->expr->result_type == DATA_TYPE_INT)
        {
            bin_node->expr->result_type = DATA_TYPE_INT;
        }
        else
        {
            panic(ERROR_TYPE_MISMATCH, "Type mismatch in binary expression", compiler);
        }
    }
    else {
        panic(ERROR_TYPE_MISMATCH, "Illegal types in binary expression", compiler);
    }

    bin_node->expr->type = EXPR_BINARY;
    bin_node->expr->binary.left = left->expr;
    bin_node->expr->binary.right = right_node->expr;
    bin_node->expr->binary.constant_foldable = constant_foldable;

    switch (op)
    {
    case TOK_ADD:
        bin_node->expr->binary.op = TOK_ADD;
        break;
    
    case TOK_SUB:
        bin_node->expr->binary.op = TOK_SUB;
        break;
    
    case TOK_MUL:
        bin_node->expr->binary.op = TOK_MUL;
        break;
    
    case TOK_DIV:
        bin_node->expr->binary.op = TOK_DIV;
        break;

    case TOK_PERCENT:
        bin_node->expr->binary.op = TOK_PERCENT;
        break;

    case TOK_EQ:
        bin_node->expr->binary.op = TOK_EQ;
        break;
    
    case TOK_NE:
        bin_node->expr->binary.op = TOK_NE;
        break;

    case TOK_GT:
        bin_node->expr->binary.op = TOK_GT;
        break;

    case TOK_GE:
        bin_node->expr->binary.op = TOK_GE;
        break;

    case TOK_LT:
        bin_node->expr->binary.op = TOK_LT;
        break;
        
    case TOK_LE:
        bin_node->expr->binary.op = TOK_LE;
        break;

    default:
        fprintf(stderr, "unidentified operator\n");
        panic(ERROR_SYNTAX, "Unknown binary operator", compiler);
    }
    return bin_node;
}

node* create_func_call_node(char* func_name, size_t name_length, expression* arguments, size_t param_count, Compiler* compiler) 
{

    function_node* func_dec_node = find_function_symbol_node(func_name, name_length, hash_function(func_name, name_length), compiler);
    
    if(!func_dec_node) {
        panic(ERROR_UNDEFINED, "Function not found", compiler);
    }


    if(func_dec_node->param_count != param_count) panic(ERROR_ARGUMENT_COUNT, "Wrong number of arguments in function call", compiler);
    

    for (size_t i = 0; i < param_count; i++)
    {
        if(arguments[i].result_type != func_dec_node->parameters[i].variable.data_type) panic(ERROR_TYPE_MISMATCH, "Function argument type mismatch", compiler);
    }
    

    node* func_call_node = arena_alloc(compiler->expressions_arena, sizeof(node), compiler);
    if(!func_call_node) return NULL;
    func_call_node->type = NODE_EXPRESSION;
    func_call_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression), compiler);
    if(!func_call_node->expr) return NULL;
    func_call_node->expr->type = EXPR_FUNCTION_CALL;
    func_call_node->expr->func_call.name = func_name;
    func_call_node->expr->func_call.name_length = name_length;
    func_call_node->expr->func_call.arguments = arguments;
    func_call_node->expr->func_call.parameter_count = param_count;
    func_call_node->expr->result_type = func_dec_node->return_type;
    return func_call_node;

}

node* create_variable_node(char* var_name, size_t length, Compiler* compiler)
{
    node* var_node = arena_alloc(compiler->expressions_arena, sizeof(node), compiler);
    if(!var_node) panic(ERROR_MEMORY_ALLOCATION, "Variable node allocation failed", compiler);
    var_node->type = NODE_EXPRESSION;
    var_node->expr = arena_alloc(compiler->expressions_arena, sizeof(expression), compiler);
    if(!var_node->expr) panic(ERROR_MEMORY_ALLOCATION, "Variable node allocation failed", compiler);
    var_node->expr->variable.length = length;
    var_node->expr->variable.name = var_name;
    var_node->expr->type = EXPR_IDENTIFIER;
    var_node->expr->variable.hash = hash_function(var_name, length);
    
    // Look up the variable in the symbol table
    symbol_node* found_symbol = find_variable(compiler, var_node->expr->variable.hash, var_name, length);
    
    if (!found_symbol) {
        panic(ERROR_UNDEFINED_VARIABLE, "Variable not found", compiler);
    }




    var_node->expr->variable.node_in_table = found_symbol;
    
    var_node->expr->variable.data_type = found_symbol->data_type;
    var_node->expr->result_type = found_symbol->data_type;
    
    

    return var_node;
}


