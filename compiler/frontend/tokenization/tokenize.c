#include "frontend/tokenization/tokenize.h"
#include "utilities/utils.h"
#include "arena/arena.h"
#include "error_handler/error_handler.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

const uint8_t CHAR_TYPE[256] = {
    ['0'] = 11, ['1'] = 11, ['2'] = 11, ['3'] = 11, ['4'] = 11, 
    ['5'] = 11, ['6'] = 11, ['7'] = 11, ['8'] = 11, ['9'] = 11,
    ['_'] = 1,
    ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1, 
    ['g'] = 1, ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1, 
    ['m'] = 1, ['n'] = 1, ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1, 
    ['s'] = 1, ['t'] = 1, ['u'] = 1, ['v'] = 1, ['w'] = 1, ['x'] = 1, 
    ['y'] = 1, ['z'] = 1,
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1, 
    ['G'] = 1, ['H'] = 1, ['I'] = 1, ['J'] = 1, ['K'] = 1, ['L'] = 1, 
    ['M'] = 1, ['N'] = 1, ['O'] = 1, ['P'] = 1, ['Q'] = 1, ['R'] = 1, 
    ['S'] = 1, ['T'] = 1, ['U'] = 1, ['V'] = 1, ['W'] = 1, ['X'] = 1, 
    ['Y'] = 1, ['Z'] = 1,
    ['='] = 2, ['+'] = 4, ['-'] = 5, ['*'] = 6, ['/'] = 7, 
    [' '] = 3, ['\n'] = 3, ['\t'] = 3, ['\r'] = 3,
    ['('] = 8, [')'] = 9, [';'] = 10, [':'] = 12, [','] = 13, 
    ['{'] = 14, ['}'] = 15, ['#'] = 16,
    ['<'] = 17, ['>'] = 18, ['!'] = 19, ['%'] = 20,
    ['&'] = 21, ['.'] = 22, ['['] = 23, [']'] = 24
};

static inline int identify_token(const char* value, size_t length, token* the_token, size_t* function_count){
    switch (length){

        case 6:
            if (value[0] == 'r' && value[1] == 'e' && value[2] == 't' && value[3] == 'u' && value[4] == 'r' && value[5] == 'n') {
                the_token->type = TOK_RETURN;
                return 0;
            }
            break;

        case 5:
            if (value[0] == 'w' && value[1] == 'h' && value[2] == 'i' && value[3] == 'l' && value[4] == 'e') {
                the_token->type = TOK_WHILE;
                return 0;
            }
            if (value[0] == 'b' && value[1] == 'r' && value[2] == 'e' && value[3] == 'a' && value[4] == 'k') {
                the_token->type = TOK_BREAK;
                return 0;
            }
            break;

        case 4:
            if (value[0] == 'e' && value[1] == 'x' && value[2] == 'i' && value[3] == 't') {
                the_token->type = TOK_EXIT;
                return 0;
            }
            if (value[0] == 'v' && value[1] == 'o' && value[2] == 'i' && value[3] == 'd') {
                the_token->type = TOK_VOID;
                return 0;
            }
            if (value[0] == 'e' && value[1] == 'l' &&  value[2] == 's' && value[3] == 'e')
            {
                the_token->type = TOK_ELSE;
                return 0;
            }
            if (value[0] == 'l' && value[1] == 'o' &&  value[2] == 'n' && value[3] == 'g')
            {
                the_token->type = TOK_LONG;
                return 0;
            }
            if (value[0] == 'v' && value[1] == 'o' &&  value[2] == 'i' && value[3] == 'd')
            {
                the_token->type = TOK_VOID;
                return 0;
            }
        
            
            break;
            
        case 3:
            if (value[0] == 'l' && value[1] == 'e' && value[2] == 't') {
                the_token->type = TOK_LET;
                return 1;
            }
            if (value[0] == 'i' && value[1] == 'n' && value[2] == 't') {
                the_token->type = TOK_INT;
                return 1;
            }
            break;
            
        case 2:
            if (value[0] == 'f' && value[1] == 'n') {
                *function_count += 1;
                the_token->type = TOK_FN;
                return 0;
            }
            if (value[0] == 'i' && value[1] == 'f')
            {
                the_token->type = TOK_IF;
                return 0;
            }
            
            break;
    }
    the_token->type = TOK_IDENTIFIER;
    return 0;
}

void tokenize(const char* source, Compiler* compiler, size_t* token_count, size_t* file_length, size_t* function_count){
    token* tokens = arena_alloc(compiler->token_arena, sizeof(token) * (*file_length + 1), compiler);

    *function_count = 0;
    *token_count = 0;
    size_t i = 0;
    size_t line_number = 1;
    while (i < *file_length) {
        if (source[i] == '\0') break;

        while (i < *file_length && CHAR_TYPE[(unsigned char)source[i]] == 3) {
            if (source[i] == '\n') line_number++;
            i++;
        }
        if (i >= *file_length) break;
        
        // Process token at position i
        size_t token_start = i;
        switch (CHAR_TYPE[(unsigned char)source[i]])
        {
        case 11: {
            int int_part = source[i] - '0';
            i++;
            while (i < *file_length && source[i] >= '0' && source[i] <= '9') {
                int_part = int_part * 10 + (source[i] - '0');
                i++;
            }

            if (i < *file_length && source[i] == '.') {
                i++; // consume '.'
                double value = (double)int_part;
                double factor = 0.1;
                while (i < *file_length && source[i] >= '0' && source[i] <= '9') {
                    value += (source[i] - '0') * factor;
                    factor *= 0.1;
                    i++;
                }
                tokens[*token_count].type = TOK_FLOAT;
                tokens[*token_count].float_value = value;
            } else {
                tokens[*token_count].type = TOK_NUMBER;
                tokens[*token_count].int_value = int_part;
            }

            tokens[*token_count].line = line_number;
            (*token_count)++;
            break;
        }
        case 2:{
            if (i + 1 < *file_length && source[i + 1] == '=')
                {
                    // It's '=='
                    tokens[*token_count].line = line_number;
                    tokens[*token_count].type = TOK_EQ;
                    (*token_count)++;
                    i += 2;  // Consume both '=' characters
                }
            else {
                    // It's just '='
                    tokens[*token_count].line = line_number;
                    tokens[*token_count].type = TOK_EQUAL;
                    (*token_count)++;
                    i++;  // Consume one '=' character
                }
                break;
        }

        case 5:{
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_SUB;
            (*token_count)++;
            i++;
            break;
        }

        case 4:{
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_ADD;
            (*token_count)++;
            i++;
            break;
        }

        case 6: {
        
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_MUL;
            (*token_count)++;
            i++;
            break;
        }
        case 7: {
            if (i + 1 < *file_length && source[i + 1] == '/') {
                i += 2;  // Skip both backslashes
                while (i < *file_length && source[i] != '\n') {
                    i++;
                }
                break;
            }
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_DIV;
            (*token_count)++;
            i++;
            break;
        }

        case 8: {
        
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_LPAREN; // LPAREN           
            (*token_count)++;
            i++;
            break;
        }
        
        case 9:{
        
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_RPAREN; // RPAREN            
            (*token_count)++;
            i++;
            break;
        }

        case 1 :{
            // Identifiers can START only with letters or underscore
            while (i < *file_length && (CHAR_TYPE[(unsigned char)source[i]] == 1 || (source[i] >= '0' && source[i] <= '9'))) {
                i++;
            }
            tokens[*token_count].line = line_number;
            tokens[*token_count].str_value.starting_value = (char*)&source[token_start];
            tokens[*token_count].str_value.length = i - token_start;
            identify_token(tokens[*token_count].str_value.starting_value, tokens[*token_count].str_value.length, &tokens[*token_count], function_count);

            (*token_count)++;
            break;
        }

        case 10:{
        
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_SEMICOLON; // SEMICOLUMN            
            (*token_count)++;
            i++;
            break;
        }
        case 13: {
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_COMMA; // COMMA            
            (*token_count)++;
            i++;
            break;
        }

        case 14: {
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_LBRACE; // {            
            (*token_count)++;
            i++;
            break;
        }

        case 15: {
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_RBRACE; // }           
            (*token_count)++;
            i++; 
            break;
        }

        case 17: {
            if (source[i + 1] == '=') { // <=
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_LE;           
                (*token_count)++;
                i+=2; 
                break;
            }
            else {   // <
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_LT;           
                (*token_count)++;
                i++; 
                break;
            }
        }

        case 18: {
            if (source[i + 1] == '=') { // >=
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_GE;           
                (*token_count)++;
                i+=2; 
                break;
            }
            else {   // >
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_GT;           
                (*token_count)++;
                i++; 
                break;
            }
        }

        case 19: {
            if (source[i + 1] == '=') { // !=
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_NE;           
                (*token_count)++;
                i+=2; 
                break;
            }
            else {   // ! only
                char buffer[100];
                snprintf(buffer, sizeof(buffer), "unidentified token at line %lu '!', did you mean '!='", line_number);
                panic(ERROR_UNDEFINED, buffer , compiler);
            }
            break;
        }

        case 20: {
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_PERCENT;           // %
            (*token_count)++;
            i++;
            break;
        }

        case 12:{
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_COLON;           // :
            (*token_count)++;
            i++;
            break;
        }
        case 21:{
            if (i + 1 < *file_length && source[i + 1] == '&') { // &&
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_AND;           
                (*token_count)++;
                i+=2; 
                break;
            }
            else {   // & only
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_BIT_AND;           // &
                (*token_count)++;
                i++;
                break;
            }
            break;
        }

        case 22: {
            if (i + 1 < *file_length && source[i + 1] == '*') { // .*
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_POINTER_DEREF;           
                (*token_count)++;
                i+=2; 
                break;
            }
            else {   // & only
                tokens[*token_count].line = line_number;
                tokens[*token_count].type = TOK_DOT;           // .
                (*token_count)++;
                i++;
                break;
            }
            break;
        }

        case 23:{
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_LBRACKET;           // [
            (*token_count)++;
            i++;
            break;
        }
        case 24:{
            tokens[*token_count].line = line_number;
            tokens[*token_count].type = TOK_RBRACKET;           // ]
            (*token_count)++;
            i++;
            break;
        }

        

        default: {
            printf("Error: Unrecognized character '%c' (ASCII %d) at line %zu, position %zu\n", 
           source[i], source[i], line_number, i);
            panic(ERROR_UNDEFINED, "unidentified token", compiler);
        }
    }}

    tokens[*token_count].line = line_number;
    tokens[*token_count].type = TOK_EOF; // end of file            
    (*token_count)++;
}