#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "preprocessor.h"

static char* read_file(const char* path) {
    char* result = NULL;
    FILE* file = fopen(path, "r+");
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long end = ftell(file);
        if (end > 0) {
            fseek(file, 0, SEEK_SET);
            void* buffer = malloc(sizeof(char)*((size_t)end + 1));
            size_t read = fread(buffer, 1, (size_t)end, file);
            if (read == (size_t)end) {
                result = buffer;
                result[end] = '\0';
            } else {
                free(buffer);
            }
        }
        fclose(file);
    }
    return result;
}

struct tokenizer_state {
    char* buffer;
    size_t offset;
    int is_new_line;
    unsigned int line;
    unsigned int column;
};

enum token_type {
    token_unknown,
    token_space,
    token_newline,
    token_directive,
    token_stringify,
    token_concat,
    token_comment,
    token_continue,
    token_punc,
    token_identifier,
    token_string,
    token_char,
    token_integer,
    token_integer_hex,
    token_integer_octal,
    token_integer_unsigned,
    token_integer_long,
    token_integer_unsigned_long,
    token_integer_i64,
    token_real_float,
    token_real_double,
    token_real_double_long,
};

struct token {
    char* text;
    size_t length;
    unsigned int line;
    unsigned int column;
    enum token_type type;
};

static char next_char(struct tokenizer_state* state) {
    char c = *(state->buffer + state->offset);
    if (c == '\0') {
        return c;
    }
    ++state->offset;
    char n = *(state->buffer + state->offset);
    // TODO: handle '\r' before 
    if (n == '\n') {
        ++state->line;
        state->is_new_line = 1;
        state->column = 0;
    } else {
        ++state->column;
    }
    return n;
}

static int next_token(struct tokenizer_state* state, struct token* token) {
    char c = *(state->buffer + state->offset);
    if (c == '\0') {
        return -1;
    }

    token->text = state->buffer + state->offset;
    token->length = 0;
    token->line = state->line;
    token->column = state->column;
    token->type = token_unknown;

    char p = *(state->buffer + state->offset + 1);
    if (c == '#' && p == '#') {
        state->is_new_line = 0;
        token->type = token_concat;
        token->length = 2;
        next_char(state);
        next_char(state);
        return 0;
    } else if (c == '#') {
        if (state->is_new_line == 1) {
            state->is_new_line = 0;
            token->type = token_directive;
            token->length = 1;
            next_char(state);
        } else {
            token->type = token_stringify;
            token->length = 1;
            next_char(state);
        }
        return 0;
    } else if (c == '/' && p == '*') {
        state->is_new_line = 0;
        token->type = token_comment;
        token->length = 1;
        while (c != '*' || p != '/') {
            c = p;
            p = next_char(state);
            if (p == '\0') {
                return -1;
            }
            ++token->length;
        }
        next_char(state);
        return 0;
    } else if (c == '/' && p == '/') {
        state->is_new_line = 0;
        token->type = token_comment;
        token->length += 2;
        c = next_char(state);
        c = next_char(state);
        while (c != '\n' && c != '\0') {
            ++token->length;
            c = next_char(state);
        }
        return 0;
    } else if (c == '"') {
        int unescaped_newline = 0;
        state->is_new_line = 0;
        token->type = token_string;
        token->length = 1;
        do {
            c = next_char(state);
            ++token->length;
            if (c == '\\') {
                c = next_char(state);
                switch (c) {
                    case '\n':
                        /* continue */
                        break;
                    case 'n':
                        /* new line */
                        break;
                    case 'r':
                        /* return */
                        break;
                    case 't':
                        /* tab */
                        break;
                    case 'v':
                        /* vertical tab */
                        break;
                    default:
                        break;
                }
                c = next_char(state);
                token->length += 2;
            } 
        } while (c != '"' && c != '\0');
        if (c == '\0') {
            return -1;
        }
        next_char(state);
        return 0;
    } else if (c == '\'') {
        state->is_new_line = 0;
        token->type = token_char;
        token->length = 1;
        do {
            c = next_char(state);
            ++token->length;
            if (c == '\\') {
                c = next_char(state);
                switch (c) {
                /* handle escape characters */
                default:
                    break;
                }
                c = next_char(state);
                token->length += 2;
            } 
        } while (c != '\'' && c != '\0');
        if (c == '\0') {
            return -1;
        }
        next_char(state);
        return 0;
    } else if (c == '\\') {
        state->is_new_line = 0;
        token->type = token_continue;
        token->length = 1;
        c = next_char(state);
        if (c != '\n') {
            // escaped nothing?
        }
        return 0;
    } else if (ispunct(c)) {
        state->is_new_line = 0;
        token->type = token_punc;
        switch (c) {
            case '[':
            case ']':
            case '{':
            case '}':
            case '(':
            case ')':
            case ';':
            case '!':
            case ':':
            case ',':
            case '?':
            case '^':
                token->length = 1;
                next_char(state);            
                break;
            case '*':
            case '/':
            case '%':
            case '~':
                token->length = 1;
                c = next_char(state);
                if (c == '=') {
                    token->length = 2;
                    next_char(state);                    
                }
                break;
            case '+':
                token->length = 1;
                c = next_char(state);
                if (c == '=' || c == '+') {
                    token->length = 2;
                    next_char(state);                    
                }
                break;
            case '-':
                token->length = 1;
                c = next_char(state);
                if (c == '=' || c == '-' || c == '>') {
                    token->length = 2;
                    next_char(state);                    
                }
                break;
            case '|':
                token->length = 1;
                c = next_char(state);
                if (c == '=' || c == '|') {
                    token->length = 2;
                    next_char(state);
                }
                break;
            case '&':
                token->length = 1;
                c = next_char(state);
                if (c == '=' || c == '&') {
                    token->length = 2;
                    next_char(state);
                }
                break;
            case '<':
                token->length = 1;
                c = next_char(state);
                if (c == '=' || c == '<') {
                    token->length = 2;
                    next_char(state);
                }
                break;
            case '>':
                token->length = 1;
                c = next_char(state);
                if (c == '=' || c == '>') {
                    token->length = 2;
                    next_char(state);
                }
                break;
            case '=':
                token->length = 1;
                c = next_char(state);
                if (c == '=') {
                    token->length = 2;
                    next_char(state);
                }
                break;
            case '.':
                token->length = 1;
                c = next_char(state);
                if (c == '.') {
                    token->length = 2;
                    c = next_char(state);
                    if (c == '.') {
                        token->length = 3;
                        next_char(state);
                    }
                }
                break;
            default:
                token->length = 1;
                next_char(state);
                break;
        }
        return 0;
    } else if (isalpha(c) || c == '_') {
        state->is_new_line = 0;
        token->type = token_identifier;
        while ((isalnum(c) || c == '_') && c != '\0') {
            ++token->length;
            c = next_char(state);
        }
        return 0;
    } else if (isnumber(c)) {
        state->is_new_line = 0;
        token->type = token_integer;
        int point = 0;
        int exponent = 0;
        if (c == '0') {
            ++token->length;
            c = next_char(state);
            if (c == '.') {
                point = 1;
                token->type = token_real_double;
                ++token->length;
                c = next_char(state);
            } else if (c == 'x' || c == 'X') {
                token->type = token_integer_hex;
                ++token->length;
                c = next_char(state);
                while (ishexnumber(c) && c != '\0') {
                    ++token->length;
                    c = next_char(state);
                }
                return 0;
            } else if (isnumber(c)) {
                token->type = token_integer_octal;
                while (isnumber(c) && c != '\0') {
                    ++token->length;
                    c = next_char(state);
                }
                return 0;
            } else {
                token->type = token_integer;
                return 0;
            }
        } 
        while (isnumber(c) && c != '\0') {
            ++token->length;
            c = next_char(state);
            if (c == '.' && point == 0 && exponent == 0) {
                point = 1;
                token->type = token_real_double;
                ++token->length;
                c = next_char(state);
            } else if ((c == 'e' || c == 'E') && exponent == 0) {
                exponent = 1;
                token->type = token_real_double;
                ++token->length;
                c = next_char(state);
                if (c == '-' || c == '+') {
                    ++token->length;
                    c = next_char(state);
                }
            }
        }
        if (c == 'f' || c == 'F') {
            token->type = token_real_float;
            ++token->length;
            next_char(state);
        } else if (c == 'i') {
            token->type = token_integer_i64;
            ++token->length;
            c = next_char(state);
            if (c == '6') {
                ++token->length;
                c = next_char(state);
                if (c == '4') {
                    ++token->length;
                    next_char(state);
                }
            }
        } else if (c == 'u' || c == 'U') {
            token->type = token_integer_unsigned;
            ++token->length;
            c = next_char(state);
            if (c == 'l' || c == 'L') {
                token->type = token_integer_unsigned_long;
                ++token->length;
                next_char(state);                
            }
        } else if (c == 'l' || c == 'L') {
            ++token->length;
            c = next_char(state);
            if (point || exponent) {
                token->type = token_real_double_long;  
            } else {
                token->type = token_integer_long;
                if (c == 'u' || c == 'U') {
                    token->type = token_integer_unsigned_long;
                    ++token->length;
                    next_char(state);
                }
            }
        }
        return 0;
    } else if (c == '\n') {
        token->type = token_newline;
        token->length = 1;
        next_char(state);
    } else if (isspace(c)) {
        token->type = token_space;
        while (isspace(c) && c != '\n' && c != '\0') {
            ++token->length;
            c = next_char(state);
        }
        return 0;
    } else {
        token->length = 1;
        next_char(state);
    }
    return 0;
}

static struct preprocessed_source* preprocess_file(struct options* options, const char* file) {
    struct preprocessed_source* result = (struct preprocessed_source*)malloc(sizeof(struct preprocessed_source));
    if (result != NULL) {
        result->name = duplicate_string(file);
        result->errors = NULL;
        result->root = NULL;
        result->buffer = NULL;
        char* buffer = read_file(file);
        if (buffer != NULL) {
            result->buffer = buffer;

            struct tokenizer_state state;
            state.buffer = buffer;
            state.column = 1;
            state.line = 1;
            state.is_new_line = 1;
            state.offset = 0;

            struct token token;
            while (next_token(&state, &token) == 0) {
                switch (token.type) {
                    case token_directive:
                        break;
                    case token_unknown:
                        break;
                    case token_space:
                        break;
                    case token_stringify:
                        break;
                    case token_concat:
                        break;
                    case token_comment:
                        break;
                    case token_newline:
                        break;
                    case token_continue:
                        break;
                    case token_punc:
                        break;
                    case token_identifier:
                        break;
                    case token_string:
                        break;
                    case token_char:
                        break;
                    case token_integer:
                        break;
                    case token_integer_hex:
                        break;
                    case token_integer_octal:
                        break;
                    case token_integer_unsigned:
                        break;
                    case token_integer_long:
                        break;
                    case token_integer_unsigned_long:
                        break;
                    case token_integer_i64:
                        break;
                    case token_real_float:
                        break;
                    case token_real_double:
                        break;
                    case token_real_double_long:
                        break;
                }
            }
        }
    }
    return result;
}

struct preprocessed_source_list* preprocess(struct options* options) {
    struct preprocessed_source_list* head = NULL; 
    struct preprocessed_source_list* tail = NULL; 
    struct string_list* inputs = options->inputs;
    while (inputs != NULL) {
        struct preprocessed_source* file = preprocess_file(options, inputs->string);
        if (file != NULL) {
            struct preprocessed_source_list* entry = (struct preprocessed_source_list*)malloc(sizeof(struct preprocessed_source_list));
            if (entry != NULL) {
                entry->source = file;
                entry->next = NULL;
                if (head == NULL) {
                    head = entry;
                }
                if (tail != NULL) {
                    tail->next = entry;
                }
                tail = entry;
            }
        }
        inputs = inputs->next;
    }
	return head;
}

void print_preprocessed_source(FILE* file, struct preprocessed_source* source) {
    if (file != NULL && source != NULL) {
        if (source->buffer != NULL) {
            // fprintf(file, source->buffer);
        }
    }
}

void free_preprocessed_source_list(struct preprocessed_source_list* sources) {
    if (sources != NULL) {
        free_preprocessed_source_list(sources->next);
        free_preprocessed_source(sources->source);
        free(sources);
    }
}

void free_preprocessed_source(struct preprocessed_source* source) {
    if (source != NULL) {
        free_error_list(source->errors);
        free(source->name);
        free(source->buffer);
        free(source);
    }
}

