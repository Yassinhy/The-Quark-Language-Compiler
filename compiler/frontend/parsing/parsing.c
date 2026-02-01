#include "utilities/utils.h"
#include "arena/arena.h"
#include "error_handler/error_handler.h"
#include "frontend/parsing/parsing.h"
#include "symbol_table/symbol_table.h"
#include "frontend/expression_creation/expressions.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

Parser *make_parser(Compiler *compiler)
{
    printf("parser 14 start\n");
    
    Parser *parser = arena_alloc(compiler->statements_arena, sizeof(Parser), compiler);
    
    printf("parser 14 end\n");

    if (!parser)
    {
        panic(ERROR_MEMORY_ALLOCATION, "Parser allocation failed", compiler);
    }

    parser->tokens = (token *)compiler->token_arena->data;
    parser->current = 0;
    parser->current_line = 1;
    return parser;
}

token *peek(Parser *parser, size_t offset)
{
    if (parser->current + offset >= parser->token_count)
    {
        fprintf(stderr, "peek surpassed tokens\n");
        return NULL;
    }
    return &parser->tokens[parser->current + offset];
}

token *advance(Parser *parser)
{
    if (parser->current >= parser->token_count)
    {
        return NULL;
    }
    return &parser->tokens[parser->current++];
}

bool match(Parser *parser, TokenType type)
{
    token *current_token = peek(parser, 0);
    return current_token && current_token->type == type;
}

node* parse_statement(Compiler *compiler, Parser *parser)
{
    printf("\n\n statement getting parsed is: %i \n\n", peek(parser, 0)->type);
    switch (peek(parser, 0)->type)
    {
    case TOK_EXIT:
        return parse_exit_node(compiler, parser);
        break;
    case TOK_RETURN:
        return parse_return_node(compiler, parser);
        break;
    case TOK_LET:
        return parse_let_node(compiler, parser);
        break;
    case TOK_FN:
        return skip_function_decleration(compiler, parser);
        break;
    case TOK_IF:
        printf("here\n");
        return parse_if_node(compiler, parser);
        break;
    case TOK_WHILE:
        return parse_while_node(compiler, parser);
        break;
    case TOK_BREAK:
        printf("found break node\n");
        return parse_break_node(compiler, parser);
        break;
    case TOK_IDENTIFIER:
        // x = 5;
        if (!peek(parser, 1))
            break;
        if (peek(parser, 1)->type == TOK_EQUAL)
        {
            // assignment
            return parse_assignment_node(compiler, parser);
        }
        if (peek(parser, 1)->type == TOK_LPAREN)
        {
            // call
            return (parse_expression(parser, PREC_NONE, true, compiler));
        }
        break;
    default:
        return (parse_expression(parser, PREC_NONE, true, compiler));
    }
    return NULL;
}

node *parse_exit_node(Compiler *compiler, Parser *parser)
{
    node *exit_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!exit_node)
        panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed", compiler);

    exit_node->type = NODE_STATEMENT;
    advance(parser); // consume 'exit'

    exit_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    exit_node->stmnt->type = STMT_EXIT;
    exit_node->stmnt->stmnt_exit.exit_code = parse_expression(parser, presedences[TOK_EXIT], true, compiler)->expr;

    if (peek(parser, 0)->type != TOK_SEMICOLON)
        panic(ERROR_SYNTAX, "Expected semicolumn ';'", compiler);
    advance(parser); // consume ;

    return exit_node;
}

node* parse_break_node(Compiler* compiler, Parser* parser) {
    node *break_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!break_node)
        panic(ERROR_MEMORY_ALLOCATION, "Return node allocation failed", compiler);

    break_node->type = NODE_STATEMENT;
    advance(parser); // consume 'break'
    
    break_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    break_node->stmnt->type = STMT_BREAK;

    if (advance(parser)->type != TOK_SEMICOLON) panic(ERROR_SYNTAX, "expected semicolomn after keyword 'break'", compiler);
    return break_node;
}

node* parse_return_node(Compiler *compiler, Parser *parser)
{
    node *return_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!return_node)
        panic(ERROR_MEMORY_ALLOCATION, "Return node allocation failed", compiler);

    return_node->type = NODE_STATEMENT;
    advance(parser); // consume 'return'

    return_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    return_node->stmnt->type = STMT_RETURN;

    if (peek(parser, 0)->type == TOK_SEMICOLON)
    {
        return_node->stmnt->stmnt_return.value = NULL;
    }
    else
    {
        expression *tmp = parse_expression(parser, PREC_NONE, true, compiler)->expr;
        return_node->stmnt->stmnt_return.value = tmp;
        return_node->stmnt->stmnt_return.return_data_type = tmp->result_type;
    }
    advance(parser); // consume ;
    return return_node;
}

node *parse_let_node(Compiler *compiler, Parser *parser)
{
    node *let_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!let_node)
        panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed", compiler);

    let_node->type = NODE_STATEMENT;
    advance(parser); // consume let

    printf("CONSUMED LET\n");

    let_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    if (!let_node->stmnt) panic(ERROR_MEMORY_ALLOCATION, "Exit node statement allocation failed", compiler);

    let_node->stmnt->type = STMT_LET;

    token *identifier_token = peek(parser, 0);

    let_node->stmnt->stmnt_let.name = identifier_token->str_value.starting_value;
    let_node->stmnt->stmnt_let.name_length = identifier_token->str_value.length;

    printf("ADDED SOME INFO\n");

    // we need this info: add_var_to_current_scope(expression* variable, variable_storage_type storage_type, normal_register reg_location)
    //  and this info:   node* create_variable_node_dec(string var_name ✅, size_t length ✅ , variable_storage_type storage_type ✅ , normal_register reg_location ✅ , Data_type data_type)
    let_node->stmnt->stmnt_let.hash = hash_function(peek(parser, 0)->str_value.starting_value, peek(parser, 0)->str_value.length);
    advance(parser); // consume identifier

    printf("CONSUMED IDENTIDIER\n");

    token *data_type_token = advance(parser); // consume data type

    printf("CONSUMED 1\n");

    printf("DATA TYPE TOKEN TYPE: %i\n", data_type_token->type);

    if (data_type_token->type != TOK_DATATYPE)
        panic(ERROR_ARGUMENT_COUNT, "undefined type", compiler);

    printf("CONSUMED 2\n");

    create_variable_node_dec(identifier_token->str_value.starting_value, identifier_token->str_value.length, STORE_IN_STACK, 0, data_type_token->data_type, compiler);

    printf("CONSUMED 3\n");

    token *t = advance(parser); // consume equal

    printf("CONSUMED 4\n");

    if (t->type != TOK_EQUAL)
    {
        panic(ERROR_SYNTAX, "usage: let var = value;", compiler);
    }

    printf("EVALUATING\n");

    let_node->stmnt->stmnt_let.value = parse_expression(parser, presedences[TOK_EQUAL], true, compiler)->expr;
    
    printf("DONE WITH THE EVALUATION\n");

    advance(parser); // consume ;
    return let_node;
}

node *parse_assignment_node(Compiler *compiler, Parser *parser)
{
    node *assignment_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!assignment_node)
        panic(ERROR_MEMORY_ALLOCATION, "Assignment node allocation failed", compiler);

    assignment_node->type = NODE_STATEMENT;
    token *identifier_token = peek(parser, 0);
    size_t hash = hash_function(identifier_token->str_value.starting_value, identifier_token->str_value.length);

    advance(parser); // consume identifier
    advance(parser); // consume equal

    assignment_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    assignment_node->stmnt->type = STMT_ASSIGNMENT;

    assignment_node->stmnt->stmnt_assign.hash = hash;
    assignment_node->stmnt->stmnt_assign.value = parse_expression(parser, presedences[TOK_EQUAL], true, compiler)->expr;
    assignment_node->stmnt->stmnt_assign.name = identifier_token->str_value.starting_value;
    assignment_node->stmnt->stmnt_assign.name_length = identifier_token->str_value.length;

    if (peek(parser, 0)->type != TOK_SEMICOLON)
        panic(ERROR_SYNTAX, "Expected semicolumn ';'", compiler);
    advance(parser); // consume ;
    return assignment_node;
}

// if (condition)
// {
//     /* code */
// }
// else if (condition)
// {
//     /* code */
// }
// else {
//  /* code */
// }

node *parse_while_node(Compiler *compiler, Parser *parser)
{
    node *while_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!while_node)
        panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed", compiler);
    while_node->type = NODE_STATEMENT;

    advance(parser); // consume while

    while_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    if (!while_node->stmnt)
        panic(ERROR_MEMORY_ALLOCATION, "Exit node statement allocation failed", compiler);
    while_node->stmnt->type = STMT_WHILE;

    if (advance(parser)->type != TOK_LPAREN)
        panic(ERROR_SYNTAX, "Expected '(' after 'if'", compiler); // consume (
    while_node->stmnt->stmnt_while.condition = parse_expression(parser, PREC_NONE, true, compiler)->expr;
    if (peek(parser, 0)->type != TOK_RPAREN)
    {
        printf("tok_rparen: %i\n", peek(parser, 0)->type);
        panic(ERROR_SYNTAX, "Didn't close parenthesis at the if/ else if statement", compiler);
    }
    advance(parser); // consume )
    // we are now at {, pass as code block
    while_node->stmnt->stmnt_while.body = parse_code_block(compiler, parser, false, NULL, 0, 0)->stmnt; // now finished }
    while_node->stmnt->stmnt_while.counter = compiler->counters->while_statements;
    compiler->counters->while_statements++;
    return while_node;
}

node *parse_if_node(Compiler *compiler, Parser *parser)
{
    node *if_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!if_node) panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed", compiler);

    if_node->type = NODE_STATEMENT;
    advance(parser); // consume if
    if_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    if (!if_node->stmnt) panic(ERROR_MEMORY_ALLOCATION, "Exit node statement allocation failed", compiler);
    if_node->stmnt->type = STMT_IF;

    if (advance(parser)->type != TOK_LPAREN) panic(ERROR_SYNTAX, "Expected '(' after 'if'", compiler); // consume (

    if_node->stmnt->stmnt_if.condition = parse_expression(parser, PREC_NONE, true, compiler)->expr;
    if (peek(parser, 0)->type != TOK_RPAREN) panic(ERROR_SYNTAX, "Didn't close parenthesis at the if/ else if statement", compiler);
    advance(parser);                                                                              // consume )


    size_t* has_return = malloc(sizeof(size_t));
    *has_return = 0;

    if (peek(parser, 0)->type == TOK_LBRACE){
    if_node->stmnt->stmnt_if.then = parse_code_block2(compiler, parser, NULL, 0, 0, has_return)->stmnt; // now finished } // if it had a return value, then has_return will be 1
    }
    else {
        if_node->stmnt->stmnt_if.then = parse_statement(compiler, parser)->stmnt;
    }

    // if (the if statement has a 100% return) {
    //     has_return = scopes = 1
    // }
    // else {
    //     has_return = 0
    // }


    if_node->stmnt->stmnt_if.or_else = NULL;
    if (peek(parser, 0)->type == TOK_ELSE) {
        if_node->stmnt->stmnt_if.or_else = parse_else_node(compiler, parser, has_return)->stmnt; // if the else or elsif block(s) had a return statement, it would be +1
        
    }

    // if (the else statement has a 100% return) {
    //     has_return = scopes = 2
    // }
    // else {
    //     has_return = 1 or 0
    // }

    if_node->stmnt->stmnt_if.return_percent = *has_return;
    free(has_return);
    
    return if_node;
}

node *parse_elseif_node(Compiler *compiler, Parser *parser, size_t* has_return)
{
    node *if_node = (node *)arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!if_node) panic(ERROR_MEMORY_ALLOCATION, "Exit node allocation failed", compiler);

    if_node->type = NODE_STATEMENT;
    advance(parser); // consume if
    if_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    if (!if_node->stmnt) panic(ERROR_MEMORY_ALLOCATION, "Exit node statement allocation failed", compiler);
    if_node->stmnt->type = STMT_IF;

    if (advance(parser)->type != TOK_LPAREN) panic(ERROR_SYNTAX, "Expected '(' after 'if'", compiler); // consume (

    if_node->stmnt->stmnt_if.condition = parse_expression(parser, PREC_NONE, true, compiler)->expr;
    if (peek(parser, 0)->type != TOK_RPAREN) panic(ERROR_SYNTAX, "Didn't close parenthesis at the if/ else if statement", compiler);
    advance(parser);                                                                              // consume )


    if (peek(parser, 0)->type == TOK_LBRACE){
    if_node->stmnt->stmnt_if.then = parse_code_block2(compiler, parser, NULL, 0, 0, has_return)->stmnt; // now finished } // if it had a return value, then has_return will be 1
    }
    else {
        if_node->stmnt->stmnt_if.then = parse_statement(compiler, parser)->stmnt;
    }

    // if (the if statement has a 100% return) {
    //     has_return = scopes = 1
    // }
    // else {
    //     has_return = 0
    // }


    if_node->stmnt->stmnt_if.or_else = NULL;
    if (peek(parser, 0)->type == TOK_ELSE) {
        if_node->stmnt->stmnt_if.or_else = parse_else_node(compiler, parser, has_return)->stmnt; // if the else or elsif block(s) had a return statement, it would be +1
        
    }

    // if (the else statement has a 100% return) {
    //     has_return = scopes = 2
    // }
    // else {
    //     has_return = 1 or 0
    // }

    if_node->stmnt->stmnt_if.return_percent = *has_return;
    
    return if_node;
}



node *parse_else_node(Compiler *compiler, Parser *parser, size_t* has_return)
{
    printf("parsing else node\n");
    if (advance(parser)->type != TOK_ELSE) panic(ERROR_SYNTAX, "expected else statement", compiler);

    if (peek(parser, 0)->type == TOK_IF) // else if
    {
        node *else_if = parse_elseif_node(compiler, parser, has_return);
        else_if->stmnt->type = STMT_IF;
        printf("done parsing else node v1\n");
        return else_if;
    }
    else if (peek(parser, 0)->type == TOK_LBRACE) // else {}
    {
        if (peek(parser, 0)->type == TOK_LBRACE){
            printf("done parsing else node v2\n");
            return parse_code_block2(compiler, parser, NULL, 0, 0, has_return); // now finished } // if it had a return value, then has_return will be 1
        }
        else {
            printf("done parsing else node v3\n");
            return parse_statement(compiler, parser);
        }
    }
    else
    {
        panic(ERROR_SYNTAX, "usage: else if {\n<code>\n}\nOR\nelse {\n<code>\n}\n", compiler);
        return NULL;
    }
    return NULL;
}

node* skip_function_decleration(Compiler* compiler, Parser* parser) {
    advance(parser); // consume fn
    advance(parser); // consume function name
    advance(parser); // consume (
    while (peek(parser, 0)->type != TOK_RPAREN) advance(parser); // consume parameters and return type
    advance(parser); // consume )
    advance(parser); // consume :datatype
    advance(parser); // consume {
    size_t stack = 0;
    while (true) {
        print_token(peek(parser, 0)->type);
        switch (peek(parser, 0)->type) {
            case TOK_LBRACE:
                stack++;
                break;
            case TOK_RBRACE:
                if (stack == 0) {
                    advance(parser); // consume }
                    return NULL;
                }
                stack--;
                break;
            case TOK_EOF:
                panic(ERROR_SYNTAX, "Curly Brace NOT closed  at function decleration", compiler);
            default:
                break;
        
        }
        printf("Stack: %lu\n", stack);
        advance(parser);
    }
    print_token(peek(parser, 0)->type);
    return NULL;
}


node* parse_function_node(Compiler* compiler, Parser* parser)
{
    node *func_stmt = arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!func_stmt)
        panic(ERROR_MEMORY_ALLOCATION, "function decleration node allocation failed", compiler);

    func_stmt->type = NODE_STATEMENT;

    func_stmt->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    if (!func_stmt)
        panic(ERROR_MEMORY_ALLOCATION, "function decleration node allocation failed", compiler);
    func_stmt->stmnt->type = STMT_FUNCTION;
    advance(parser);                   // consume fn
    token *name_tok = advance(parser); // consume function name
    if (name_tok->type != TOK_IDENTIFIER) panic(ERROR_SYNTAX, "Syntax error, use case fn function_name (parameters) {\n----->code\n}                         ~~~~~~~~~~~~~", compiler);

    /////////////////////////////////////

    func_stmt->stmnt->stmnt_function_declaration.name = name_tok->str_value.starting_value;
    func_stmt->stmnt->stmnt_function_declaration.name_length = name_tok->str_value.length;

    func_stmt->stmnt->stmnt_function_declaration.index = hash_function(name_tok->str_value.starting_value, name_tok->str_value.length);

    func_stmt->stmnt->stmnt_function_declaration.function_node = arena_alloc(compiler->symbol_arena, sizeof(function_node), compiler);
    
    if (!func_stmt->stmnt->stmnt_function_declaration.function_node) panic(ERROR_MEMORY_ALLOCATION, "function decleration node allocation failed", compiler);
        
    func_stmt->stmnt->stmnt_function_declaration.function_node->name = name_tok->str_value.starting_value;
    func_stmt->stmnt->stmnt_function_declaration.function_node->name_length = name_tok->str_value.length;


    //////////////////////////////////////

    uint32_t param_count = 0;
    // split to case void and case parameters
    if (peek(parser, 0)->type != TOK_LPAREN)
        panic(ERROR_SYNTAX, "fn ( parameters ) { code }\n   ~\n", compiler);
    advance(parser); // consume (
    if (peek(parser, 0)->type == TOK_VOID && peek(parser, 1)->type == TOK_RPAREN)
    {
        advance(parser); // consume void
        advance(parser); // consume )
        func_stmt->stmnt->stmnt_function_declaration.function_node->param_count = 0;
        func_stmt->stmnt->stmnt_function_declaration.function_node->parameters = NULL;
    }
    else
    {
        // find number of parameters and check syntax
        uint32_t tmp = 0;
        while (true)
        {
            // check if dev didn't close the paren
            if (peek(parser, tmp)->type == TOK_EOF)
                panic(ERROR_SYNTAX, "fn function_name (parameters')'\n                             ~\n", compiler);

            if (peek(parser, tmp)->type != TOK_IDENTIFIER)
                panic(ERROR_SYNTAX, "function parameter name not specified", compiler);
            tmp++;
            if (peek(parser, tmp)->type != TOK_DATATYPE)
                panic(ERROR_SYNTAX, "function parameter data type not specified", compiler);
            tmp++;
            param_count++;
            if (peek(parser, tmp)->type == TOK_COMMA)
                tmp++;
            else if (peek(parser, tmp)->type == TOK_RPAREN)
                break;
            else
                panic(ERROR_SYNTAX, "after parameter either ')' or ',' only", compiler);
        }

        // populate function node info
        func_stmt->stmnt->stmnt_function_declaration.function_node->param_count = param_count;
        func_stmt->stmnt->stmnt_function_declaration.function_node->parameters = arena_alloc(compiler->symbol_arena, sizeof(expression) * param_count, compiler);
        
        if (!func_stmt->stmnt->stmnt_function_declaration.function_node->parameters)
            panic(ERROR_MEMORY_ALLOCATION, "function parameters allocation failed", compiler);

        // loop through them and store their name, name len, data type
        for (size_t i = 0; i < param_count; i++)
        {
            token *name = advance(parser); // consume param name
            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.name = name->str_value.starting_value;
            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.length = name->str_value.length;
            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.hash = hash_function(name->str_value.starting_value, name->str_value.length);
            token *data_type = advance(parser); // consume param data type
            
            printf("DEBUG param: name=%.*s, data_type token value=%d\n", 
            (int)name->str_value.length, name->str_value.starting_value, 
            data_type->data_type);

            func_stmt->stmnt->stmnt_function_declaration.function_node->parameters[i].variable.data_type = data_type->data_type;
            advance(parser); // consume , or )  it doesn't rly matter
        }
    }
    // now we are at the return type
    if (peek(parser, 0)->type != TOK_DATATYPE)
        panic(ERROR_SYNTAX, "expected return value data type", compiler);
    func_stmt->stmnt->stmnt_function_declaration.function_node->return_type = advance(parser)->data_type;
    // consumed return type
    // now at {
    printf("before codeblock\n");
    
    append_function_to_func_map(func_stmt->stmnt->stmnt_function_declaration.function_node, func_stmt->stmnt->stmnt_function_declaration.index, compiler);
    
    func_stmt->stmnt->stmnt_function_declaration.function_node->code_block = parse_code_block(compiler, parser, true, func_stmt->stmnt->stmnt_function_declaration.function_node->parameters, func_stmt->stmnt->stmnt_function_declaration.function_node->param_count, func_stmt->stmnt->stmnt_function_declaration.function_node->return_type)->stmnt;
    func_stmt->stmnt->stmnt_function_declaration.function_node->next = NULL;
    printf("after code block\n");
    
    return func_stmt;
}

node *parse_code_block(Compiler *compiler, Parser *parser, bool function_block, expression *params, size_t param_count, Data_type function_return_type)
{
    // for function blocks, not a function: 0 a function but no return type: 1 a function with a return type: 2
    if (advance(parser)->type != TOK_LBRACE)
        panic(ERROR_SYNTAX, "Must start code block using '{'", compiler);
    node *block_node = arena_alloc(compiler->statements_arena, sizeof(node), compiler);
    if (!block_node)
        panic(ERROR_MEMORY_ALLOCATION, "Code block node allocation failed", compiler);
    block_node->type = NODE_STATEMENT;
    block_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);
    if (!block_node->stmnt)
        panic(ERROR_MEMORY_ALLOCATION, "Code block statement allocation failed", compiler);
    block_node->stmnt->type = STMT_BLOCK;

    // Save current position
    size_t start_position = parser->current;
    // Fast forward to find matching RBRACE and count statements
    size_t brace_depth = 1;
    size_t stmt_count = 0;
    size_t i = 0;
    while (brace_depth > 0)
    {
        token *tok = peek(parser, i);
        if (!tok)
            panic(ERROR_SYNTAX, "Unclosed brace", compiler);

        if (tok->type == TOK_LBRACE)
        {
            brace_depth++;
        }
        else if (tok->type == TOK_RBRACE)
        {
            brace_depth--;
        }
        else if (brace_depth == 1)
        {
            // Only count statements at our depth
            // Heuristic: count statement starters
            if (tok->type == TOK_LET ||
                tok->type == TOK_EXIT ||
                tok->type == TOK_RETURN ||
                tok->type == TOK_IF ||
                tok->type == TOK_ELSE ||
                tok->type == TOK_WHILE ||
                tok->type == TOK_FOR ||
                tok->type == TOK_BREAK||
                tok->type == TOK_IDENTIFIER)
            {
                stmt_count++;
            }
        }
        i++;
    }
    // Reset parser position
    parser->current = start_position;

    block_node->stmnt->stmnt_block.statements = (statement **)arena_alloc(compiler->statements_arena, sizeof(statement *) * stmt_count, compiler);
    size_t statement_count = 0;
    enter_new_scope(compiler, function_return_type);
    block_node->stmnt->stmnt_block.table = peek_symbol_stack(compiler);
    if (function_block)
        block_node->stmnt->stmnt_block.table->parent_scope = compiler->symbol_table_stack->storage[0];
    else
    {
        block_node->stmnt->stmnt_block.table->parent_scope = peek_symbol_stack(compiler);
    }

    // if the thing has a return value
    int has_ret = 0;

    if (param_count <= 6)
    {
        for (size_t i = 0; i < param_count; i++)
        {
            add_var_to_current_scope(compiler, &(params[i]), STORE_IN_REGISTER, registers[i]);
        }
    }
    else
    {
        for (int i = 0; i < 6; i++)
        {
            add_var_to_current_scope(compiler, &(params[i]), STORE_IN_REGISTER, registers[i]);
        }
        for (size_t i = param_count - 1; i >= 6; i--)
        {
            add_var_to_current_scope(compiler, &(params[i]), STORE_AS_PARAM, 0);
        }
    }
    while (peek(parser, 0)->type != TOK_RBRACE)
    {
        node *statement = NULL;
        switch (peek(parser, 0)->type)
        {
        case TOK_EXIT:
            statement = parse_exit_node(compiler, parser);
            break;

        case TOK_LET:
            statement = parse_let_node(compiler, parser);
            break;

        case TOK_IF:
            statement = parse_if_node(compiler, parser);
            if (statement->stmnt->stmnt_if.return_percent == 2) has_ret = 2;
            else if ((statement->stmnt->stmnt_if.return_percent == 1) && (has_ret == 0)) has_ret = 1;
            break;

        case TOK_WHILE:
            statement = parse_while_node(compiler, parser);
            break;

        case TOK_IDENTIFIER:
        {
            // x = 5;
            if (!peek(parser, 1))
                break;
            if (peek(parser, 1)->type == TOK_EQUAL)
            {
                // assignment
                statement = parse_assignment_node(compiler, parser);
            }
            else {
                statement = parse_expression(parser, PREC_NONE, true, compiler);
            }
            break;
        }

        case TOK_BREAK:
            printf("found break node\n");
            statement = parse_break_node(compiler,  parser);
            break;

        case TOK_RETURN:
        {
            statement = parse_return_node(compiler, parser);
            has_ret = 2;

            if (statement && statement->stmnt)
            {
                
                Data_type expected_return_type = peek_symbol_stack(compiler)->scope_data_type;
                if (statement->stmnt->stmnt_return.return_data_type != expected_return_type)
                {
                    printf("scope data type: %d, return type: %d\n", expected_return_type, statement->stmnt->stmnt_return.return_data_type);
                    panic(ERROR_TYPE_MISMATCH, "Incompatible return type of output with current scope", compiler);
                }
            }
            break;
        }
        
        default:
            statement = parse_expression(parser, PREC_NONE, true, compiler);
            break;
        }
        //if (has_ret == 2) skip to end of block; dead code elmination
        if (!statement)
            continue;
        block_node->stmnt->stmnt_block.statements[statement_count] = statement->stmnt;
        statement_count++;
    }

    if (function_block) // fn add(x :int, y :int) :int {}
    {
        if (has_ret == 0)
            panic(ERROR_UNDEFINED, "Function has no return, although declared to have a return type", compiler);
        else if (has_ret == 1) {
            warning("WARNING: not all paths of the function contains a path of returning a value, although declared to have a return type", compiler);
        }
    }

    block_node->stmnt->stmnt_block.statement_count = statement_count;
    advance(parser); // consume }
    exit_current_scope(compiler);
    return block_node;
}

node *parse_code_block2(Compiler *compiler, Parser *parser, expression *params, size_t param_count, Data_type function_return_type, size_t* path_exahustion)
{
    printf("started with parsing code block 2\n");
    // for function blocks, not a function: 0 a function but no return type: 1 a function with a return type: 2
    if (advance(parser)->type != TOK_LBRACE) panic(ERROR_SYNTAX, "Must start code block using '{'", compiler);

    printf("checkpoint 0\n");
    node *block_node = arena_alloc(compiler->statements_arena, sizeof(node), compiler);

    printf("checkpoint 1\n");
    if (!block_node)    panic(ERROR_MEMORY_ALLOCATION, "Code block node allocation failed", compiler);

    printf("checkpoint 2\n");
    block_node->type = NODE_STATEMENT;
    block_node->stmnt = arena_alloc(compiler->statements_arena, sizeof(statement), compiler);

    printf("checkpoint 3\n");
    if (!block_node->stmnt)    panic(ERROR_MEMORY_ALLOCATION, "Code block statement allocation failed", compiler);

    printf("checkpoint 4\n");
    block_node->stmnt->type = STMT_BLOCK;

    printf("checkpoint 1\n");
    // Save current position
    size_t start_position = parser->current;
    // Fast forward to find matching RBRACE and count statements
    size_t brace_depth = 1;
    size_t stmt_count = 0;
    size_t i = 0;
    
    printf("checkpoint 2\n");

    while (brace_depth > 0)
    {
        token *tok = peek(parser, i);
        if (!tok)
            panic(ERROR_SYNTAX, "Unclosed brace", compiler);

        if (tok->type == TOK_LBRACE)
        {
            brace_depth++;
        }
        else if (tok->type == TOK_RBRACE)
        {
            brace_depth--;
        }
        else if (brace_depth == 1)
        {
            // Only count statements at our depth
            // Heuristic: count statement starters
            if (tok->type == TOK_LET ||
                tok->type == TOK_EXIT ||
                tok->type == TOK_RETURN ||
                tok->type == TOK_IF ||
                tok->type == TOK_ELSE ||
                tok->type == TOK_WHILE ||
                tok->type == TOK_FOR ||
                tok->type == TOK_BREAK||
                tok->type == TOK_IDENTIFIER)
            {
                stmt_count++;
            }
        }
        i++;
    }

    printf("checkpoint 3\n");

    // Reset parser position
    parser->current = start_position;

    printf("checkpoint A\n");

    block_node->stmnt->stmnt_block.statements = (statement **)arena_alloc(compiler->statements_arena, sizeof(statement *) * stmt_count, compiler);
    size_t statement_count = 0;
    enter_new_scope(compiler, function_return_type);

    printf("checkpoint B\n");
    block_node->stmnt->stmnt_block.table = peek_symbol_stack(compiler);

    printf("checkpoint C\n");
    block_node->stmnt->stmnt_block.table->parent_scope = peek_symbol_stack(compiler);
    
    printf("checkpoint D\n");
    // if the thing has a return value
    int has_ret = 0;

    Data_type return_type = DATA_TYPE_UNDEFINED;

    printf("checkpoint E\n");

    if (param_count <= 6)
    {
        printf("checkpoint F\n");
        for (size_t i = 0; i < param_count; i++)
        {
            add_var_to_current_scope(compiler, &(params[i]), STORE_IN_REGISTER, registers[i]);
        }
    }
    else
    {
        printf("checkpoint F2\n");
        for (int i = 0; i < 6; i++)
        {
            add_var_to_current_scope(compiler, &(params[i]), STORE_IN_REGISTER, registers[i]);
        }
        for (size_t i = param_count - 1; i >= 6; i--)
        {
            add_var_to_current_scope(compiler, &(params[i]), STORE_AS_PARAM, 0);
        }
    }
    printf("checkpoint G\n");
    while (peek(parser, 0)->type != TOK_RBRACE)
    {
        node *statement = NULL;
        printf("checkpoint: %i\n", peek(parser, 0)->type);
        switch (peek(parser, 0)->type)
        {
        case TOK_EXIT:
            statement = parse_exit_node(compiler, parser);
            break;

        case TOK_LET:
            statement = parse_let_node(compiler, parser);
            break;

        case TOK_IF:
            statement = parse_if_node(compiler, parser);
            has_ret = (statement->stmnt->stmnt_if.return_percent == 2);
            break;

        case TOK_WHILE:
            statement = parse_while_node(compiler, parser);
            break;

        case TOK_IDENTIFIER:
        {
            printf("Arrived to token identifier\n");
            // x = 5;
            if (!peek(parser, 1))
                panic(ERROR_UNDEFINED, "nothing after identifier\n", compiler);
            if (peek(parser, 1)->type == TOK_EQUAL)
            {
                // assignment
                printf("Arrived to token assignment\n");
                statement = parse_assignment_node(compiler, parser);
            }
            else {
                statement = parse_expression(parser, PREC_NONE, true, compiler);
            }
            printf("Arrived to break\n");
            break;
        }

        case TOK_BREAK:
            printf("found break node\n");
            statement = parse_break_node(compiler,  parser);
            break;

        case TOK_RETURN:
        {
            statement = parse_return_node(compiler, parser);
            has_ret = 1;

            if (statement && statement->stmnt)
            {
                return_type = statement->stmnt->stmnt_return.return_data_type;
                
                Data_type expected_return_type = peek_symbol_stack(compiler)->scope_data_type;
                if (return_type != expected_return_type)
                {
                    printf("scope data type: %d, return type: %d\n", expected_return_type, return_type);
                    panic(ERROR_TYPE_MISMATCH, "Incompatible return type of output with current scope", compiler);
                }
            }
            break;
        }
        
        default:
            printf("returneed to default\n");
            statement = parse_expression(parser, PREC_NONE, true, compiler);
            break;
        }

        if (!statement)
            continue;
        block_node->stmnt->stmnt_block.statements[statement_count] = statement->stmnt;
        statement_count++;
    }
    printf("checkpoint H\n");
    *path_exahustion = *path_exahustion + has_ret;

    block_node->stmnt->stmnt_block.statement_count = statement_count;
    advance(parser); // consume }
    exit_current_scope(compiler);
    printf("done with parsing code block 2\n");
    return block_node;
}




// 5 + 2 * 3 + x
node *parse_expression(Parser *parser, int prev_presedence, bool constant_foldable, Compiler *compiler)
{
    constant_foldable = true;
    node *left = parse_prefix(parser, constant_foldable, compiler); // after that the parser->current will be at the operator after the number/prefix token(s)
    TokenType curr_opperator = peek(parser, 0)->type;
    int curr_presedence = presedences[curr_opperator];

    while (true)
    {
        if (curr_presedence <= prev_presedence)
        {
            break;
        }
        else
        {
            advance(parser);                                                                   // now parser is on what is after the operator
            left = create_bin_node(left, curr_opperator, parser, constant_foldable, compiler); // now parser is on what was seen as a weak node (like the second plus on 5 + 2 * 3 + 1 )
            curr_opperator = peek(parser, 0)->type;
            curr_presedence = presedences[curr_opperator];
        }
    }
    return left;
}

node *parse_prefix(Parser *parser, bool constant_foldable, Compiler *compiler)
{
    token *current_token = peek(parser, 0);
    printf("token type: %i\n", current_token->type);
    if (!current_token)
        return NULL;
    switch (current_token->type)
    {
    case TOK_NUMBER:
    {
        printf("parsing number\n");
        node *num_node = create_number_node(current_token->int_value, compiler);
        advance(parser); // consume number token
        printf("done with parsing number\n");
        return num_node;
    }

    case TOK_IDENTIFIER:
    {
        printf("found identifier\n");
        constant_foldable = false;         // look into the token after the identifier to see if it's a function call or variable
        advance(parser);                   // consume identifier token
        token *next_tok = peek(parser, 0); // consume identifier token
        if (next_tok->type == TOK_EOF) panic(ERROR_SYNTAX, "Unexpected end of file after identifier", compiler);

        printf("found identifier\n");
        
        ////////////////////// FUNC CALL   //////////////////////////////
        if (next_tok->type == TOK_LPAREN)
        {
            printf("found function call\n");
            advance(parser); // consume '('
            // Handle empty arguments: f() or f(void)
            if (peek(parser, 0)->type == TOK_RPAREN ||
                (peek(parser, 0)->type == TOK_VOID &&
                 peek(parser, 1)->type == TOK_RPAREN))
            {

                if (peek(parser, 0)->type == TOK_VOID)
                {
                    advance(parser); // consume 'void'
                }
                advance(parser); // consume ')'

                return create_func_call_node(current_token->str_value.starting_value, current_token->str_value.length, NULL, 0, compiler);
            }
            else
            { // normal case
                size_t param_count = 0;

                // first pass
                size_t tmp = 0;
                while (peek(parser, tmp)->type != TOK_RPAREN)
                {
                    if (peek(parser, tmp)->type == TOK_COMMA)
                        param_count++;
                    tmp++;
                }
                param_count++;

                // second pass
                expression *expressions = arena_alloc(compiler->expressions_arena, param_count * sizeof(expression), compiler);
                for (size_t i = 0; i < param_count; i++)
                {
                    expressions[i] = *(parse_expression(parser, PREC_NONE, false, compiler)->expr);
                    advance(parser); // consume comma and final R_BRACE
                }
                return create_func_call_node(current_token->str_value.starting_value, current_token->str_value.length, expressions, param_count, compiler);

                // second pass
            }
        }
        else
        {
            symbol_node *var = find_variable(compiler, hash_function(current_token->str_value.starting_value, current_token->str_value.length), current_token->str_value.starting_value, current_token->str_value.length);
            if (!var)
            {
                panic(ERROR_UNDEFINED_VARIABLE, "Variable used before declaration", compiler);
            }
            node *var_node = create_variable_node(var->var_name, var->var_name_size, compiler);
            return var_node;
        }
        panic(ERROR_SYNTAX, "Invalid identifier usage", compiler);
        break;
    }

    case TOK_SUB:
    { // Unary minus
        advance(parser);
        node *operand = parse_expression(parser, PREC_UNARY, true, compiler);
        node *sub_node = create_unary_node(TOK_SUB, operand, compiler);

        return sub_node;
    }

    case TOK_LPAREN:
    { // Braces must be used in C if you will declare a variable inside the case
        advance(parser);
        node *expr = parse_expression(parser, PREC_NONE, true, compiler);
        if (peek(parser, 0)->type != TOK_RPAREN)
        {
            panic(ERROR_SYNTAX, "Expected ')' ", compiler);
        }
        advance(parser);
        return expr;
    }

    default:
        panic(ERROR_UNDEFINED, "Unexpected token/s, may still not be emplemented", compiler);
    }
    // will never reach this case
    return NULL;
}
