#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
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

static void put_token(struct tokenizer_state* state, struct token* token) {
    if (token->type == token_directive) {
        state->is_new_line = 1;
    }
    state->offset = state->offset - token->length;
}

static struct preprocessed_node* make_node(enum preprocessed_node_type type) {
    struct preprocessed_node* result = (struct preprocessed_node *)malloc(sizeof(struct preprocessed_node));
    if (result != NULL) {
        result->type = type;
        result->offset = NULL;
        result->length = 0;
        result->column = 0;
        result->line = 0;
        result->next = NULL;
        result->first = NULL;
    }
    return result;
}

static struct preprocessed_node* make_token_node(struct token* token) {
    struct preprocessed_node* result = (struct preprocessed_node *)malloc(sizeof(struct preprocessed_node));
    if (result != NULL) {
        result->type = preprocessed_node_token;
        result->offset = token->text;
        result->length = token->length;
        result->line = token->line;
        result->column = token->column;
        result->next = NULL;
        result->first = NULL;
    }
    return result;
}

static void append_child_node(struct preprocessed_node* parent, struct preprocessed_node* child) {
    if (parent->first == NULL) {
        parent->first = child;
    } else {
        struct preprocessed_node* last = parent->first;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = child;
    }
}

static struct preprocessed_node* preprocess_easy_directive(enum preprocessed_node_type type, struct tokenizer_state* state, struct token* token) {
    struct preprocessed_node* result = make_node(type);
    append_child_node(result, make_token_node(token));
    while (next_token(state, token) == 0) {
        append_child_node(result, make_token_node(token));
        if (token->type == token_continue) {
            next_token(state, token);
            append_child_node(result, make_token_node(token));
        } else if (token->type == token_newline) {
            return result;            
        }
    }
    return result;
}

static struct preprocessed_node* preprocess_include(struct tokenizer_state* state, struct token* token) {
    return preprocess_easy_directive(preprocessed_node_include, state, token);
}

static struct preprocessed_node* preprocess_undef(struct tokenizer_state* state, struct token* token) {
    return preprocess_easy_directive(preprocessed_node_undef, state, token);
}

static struct preprocessed_node* preprocess_define(struct tokenizer_state* state, struct token* token) {
    return preprocess_easy_directive(preprocessed_node_define, state, token);
}

static struct preprocessed_node* preprocess_node(struct tokenizer_state* state, struct token* token);

static struct preprocessed_node* preprocess_ifdef(struct tokenizer_state* state, struct token* token) {
    struct preprocessed_node* result = make_node(preprocessed_node_ifdef);
    append_child_node(result, make_token_node(token));
    while (next_token(state, token) == 0) {
        append_child_node(result, make_token_node(token));
        if (token->type == token_continue) {
            next_token(state, token);
            append_child_node(result, make_token_node(token));
        } else if (token->type == token_newline) {
            next_token(state, token);
            struct preprocessed_node* body = preprocess_node(state, token);
            append_child_node(result, body);
            next_token(state, token);
            if (token->type == token_directive) {
                // TODO: eat possible space
                append_child_node(result, make_token_node(token));
                next_token(state, token);
                if (strncmp("endif", token->text, token->length) == 0) {
                    append_child_node(result, make_token_node(token));
                    return result;
                } else {
                    return NULL;
                }
            } else {
                put_token(state, token);
                return result;
            }
        } else {

        }
    }
    return result;
}

static struct preprocessed_node* preprocess_ifndef(struct tokenizer_state* state, struct token* token) {
    struct preprocessed_node* result = preprocess_ifdef(state, token);
    if (result != NULL) {
        result->type = preprocessed_node_ifndef;
    }
    return result;
}

static struct preprocessed_node* preprocess_if(struct tokenizer_state* state, struct token* token) {
    struct preprocessed_node* result = make_node(preprocessed_node_if);
    return result;
}

static struct preprocessed_node* preprocess_pragma(struct tokenizer_state* state, struct token* token) {
    return preprocess_easy_directive(preprocessed_node_pragma, state, token);
}

static struct preprocessed_node* preprocess_error(struct tokenizer_state* state, struct token* token) {
    return preprocess_easy_directive(preprocessed_node_error, state, token);
}

static struct preprocessed_node* preprocess_line(struct tokenizer_state* state, struct token* token) {
    return preprocess_easy_directive(preprocessed_node_line, state, token);
}

static struct preprocessed_node* preprocess_unknown(struct tokenizer_state* state, struct token* token) {
    return preprocess_easy_directive(preprocessed_node_unknown, state, token);
}

static struct preprocessed_node* preprocess_directive(struct tokenizer_state* state, struct token* token) {
    // TODO: prepend initial token to node
    if (next_token(state, token) == 0) {
        if (token->type == token_space) {
            next_token(state, token);
            // TODO: prepend token to node
        }        
        if (token->type == token_identifier) {
            if (strncmp("include", token->text, token->length) == 0) {
                return preprocess_include(state, token);
            } else if (strncmp("define", token->text, token->length) == 0) {
                return preprocess_define(state, token);
            } else if (strncmp("undef", token->text, token->length) == 0) {
                return preprocess_undef(state, token);
            } else if (strncmp("ifndef", token->text, token->length) == 0) {
                return preprocess_ifndef(state, token);
            } else if (strncmp("ifdef", token->text, token->length) == 0) {
                return preprocess_ifdef(state, token);
            } else if (strncmp("if", token->text, token->length) == 0) {
                return preprocess_if(state, token);
            } else if (strncmp("pragma", token->text, token->length) == 0) {
                return preprocess_pragma(state, token);
            } else if (strncmp("error", token->text, token->length) == 0) {
                return preprocess_error(state, token);
            } else if (strncmp("line", token->text, token->length) == 0) {
                return preprocess_line(state, token);
            } else {
                return preprocess_unknown(state, token);
            }
        } else {
            return preprocess_unknown(state, token);            
        }
    }
    return NULL;
}

static struct preprocessed_node* preprocess_block(struct tokenizer_state* state, struct token* token) {
    struct preprocessed_node* block = make_node(preprocessed_node_block);
    block->line = token->line;
    block->column = token->column;
    append_child_node(block, make_token_node(token));
    while (next_token(state, token) == 0) {
        if (token->type == token_directive) {
            put_token(state, token);
            return block;
        } else {
            append_child_node(block, make_token_node(token));
        }
    }
    return block;
}

static struct preprocessed_node* preprocess_node(struct tokenizer_state* state, struct token* token) {
    if (token->type == token_directive) {
        return preprocess_directive(state, token);
    } else {
        return preprocess_block(state, token);
    }
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

            struct preprocessed_node* root = make_node(preprocessed_node_root);
            struct token token;
            while (next_token(&state, &token) == 0) {
                struct preprocessed_node* node = preprocess_node(&state, &token);
                if (node != NULL) {
                    append_child_node(root, node);
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

