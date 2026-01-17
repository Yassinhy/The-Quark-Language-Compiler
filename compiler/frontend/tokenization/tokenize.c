#include "frontend/tokenization/tokenize.h"
#include "utilities/utils.h"
#include "arena/arena.h"
#include "error_handler/error_handler.h"

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
    ['{'] = 14, ['}'] = 15, ['#'] = 16
};

static inline int identify_token(const char* value, size_t length, token* the_token){
    switch (length){
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
            
            break;
            
        case 3:
            if (value[0] == 'l' && value[1] == 'e' && value[2] == 't') {
                the_token->type = TOK_LET;
                return 1;
            }
            break;
            
        case 2:
            if (value[0] == 'f' && value[1] == 'n') {
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

void tokenize(const char* source, Compiler* compiler, size_t* token_count, size_t* file_length){
    printf("?\n");
    token* tokens = arena_alloc(compiler->token_arena, sizeof(token) * (*file_length + 1), compiler);


    *token_count = 0;
    size_t i = 0;
    size_t line_number = 1;
    printf("check\n");
    while (i < *file_length) {
        if (source[i] == '\0') break;

        while (i < *file_length && CHAR_TYPE[(unsigned char)source[i]] == 3) {
            if (source[i] == '\n') line_number++;
            i++;
        }
        if (i >= *file_length) break;
        
        // Process token at position i
        size_t token_start = i;
        printf("char: %c\n", source[i]);
        printf("%i\n", CHAR_TYPE[(unsigned char)source[i]]);
        switch (CHAR_TYPE[(unsigned char)source[i]])
        {
        case 11:{
            int value = source[i] - '0';
            i++;
            while (i < *file_length && (source[i] >= '0' && source[i] <= '9')) {
                value = value * 10 + (source[i] - '0');
                i++;
            }
            tokens[*token_count].line = line_number;
            tokens[*token_count].int_value = value;
            tokens[*token_count].type = TOK_NUMBER;
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
            while (i < *file_length && CHAR_TYPE[(unsigned char)source[i]] == 1) {
                i++;
            }
            tokens[*token_count].line = line_number;
            tokens[*token_count].str_value.starting_value = (char*)&source[token_start];
            tokens[*token_count].str_value.length = i - token_start;
            identify_token(tokens[*token_count].str_value.starting_value, tokens[*token_count].str_value.length, &tokens[*token_count]);

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

        case 16: {
            if (i + 1 < *file_length && source[i + 1] == '#') {
                // Comment detected - skip to end of line
                printf("Found backslash at pos %zu, next char is '%c' (ASCII %d)\n", 
                i, source[i+1], source[i+1]);
                i += 2;  // Skip both backslashes
                while (i < *file_length && source[i] != '\n') {
                    i++;
                }
            } else {
                panic(ERROR_SYNTAX, "backslash must be followed by another backslash for comments (\\\\)", compiler);
            }
            break;
        }

        case 12:{
        
            tokens[*token_count].line = line_number;         
            i++;
            switch (source[i])
            {
                case 'i':
                    if (source[i + 1] == 'n' && source[i + 2] == 't') {
                        tokens[*token_count].type = TOK_DATATYPE;
                        tokens[*token_count].data_type = DATA_TYPE_INT;
                        i += 3;
                    }
                    break;
                case 'b':
                    if (source[ i + 1] == 'o' && source[ i + 2] == 'o' && source[ i + 3] == 'l') {
                        tokens[*token_count].type = TOK_DATATYPE;
                        tokens[*token_count].data_type = DATA_TYPE_BOOL;
                        i += 4;
                    }
                    break;
                case 'c':
                    if (source[ i + 1] == 'h' && source[ i + 2] == 'a' && source[ i + 3] == 'r') {
                        tokens[*token_count].type = TOK_DATATYPE;
                        tokens[*token_count].data_type = DATA_TYPE_CHAR;
                        i += 4;
                    }
                    break;
                case 'l':
                    if (source[ i + 1] == 'o' && source[ i + 2] == 'n' && source[ i + 3] == 'g') {
                        tokens[*token_count].type = TOK_DATATYPE;
                        tokens[*token_count].data_type = DATA_TYPE_LONG;
                        i += 4;
                    }
                    break;
                case 'd':
                    if (source[ i + 1] == 'o' && source[ i + 2] == 'u' && source[ i + 3] == 'b' && source[ i + 4] == 'l' && source[ i + 5] == 'e') {
                        tokens[*token_count].type = TOK_DATATYPE;
                        tokens[*token_count].data_type = DATA_TYPE_DOUBLE;
                        i += 6;
                    }
                break;
            default:
                panic(ERROR_SYNTAX, "unidentified data type at line", compiler);
            }
            (*token_count)++;
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