#ifndef PARSING_H
#define PARSING_H

#include "utilities/utils.h"
#include <stddef.h>

// Parsing functions
Parser* make_parser(Compiler* compiler);
token* peek(Parser* parser, size_t offset);
token* advance(Parser* parser);
bool match(Parser* parser, TokenType type);


node* parse_statement(Compiler* compiler, Parser* parser);

node* parse_expression(Parser* parser, int prev_presedence, bool constant_foldable, Compiler* compiler);
node* parse_prefix(Parser* parser, bool constant_foldable, Compiler* compiler);


node* parse_exit_node(Compiler* compiler, Parser* parser);
node *parse_return_node(Compiler *compiler, Parser* parser);
node* parse_break_node(Compiler* compiler, Parser* parser);

node* parse_let_node(Compiler* compiler, Parser* parser);
node* parse_assignment_node(Compiler* compiler, Parser* parser);

node* parse_if_node(Compiler* compiler, Parser* parser);
node *parse_else_node(Compiler *compiler, Parser *parser, size_t* has_return);

node* parse_while_node (Compiler* compiler, Parser* parser);

node* parse_function_node(Compiler* compiler, Parser* parser);
node* skip_function_decleration(Compiler* compiler, Parser* parser);
node* parse_code_block(Compiler* compiler, Parser* parser, bool function_block, expression* params, size_t param_count , Data_type function_return_type);
node* parse_code_block2(Compiler* compiler, Parser* parser, expression* params, size_t param_count , Data_type function_return_type, size_t* has_return);
#endif