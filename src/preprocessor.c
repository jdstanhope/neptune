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
    unsigned long line;
    unsigned long column;
};

enum token_type {
    token_unknown,
    token_space,
    token_directive,
    token_stringify,
    token_concat,
    token_comment,
    token_newline,
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
    enum token_type type;
};

static char next_char(struct tokenizer_state* state) {
    char c = *(state->buffer + state->offset);
    if (c == '\0') {
        return c;
    }
    ++state->offset;
    char n = *(state->buffer + state->offset);
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
        state->is_new_line = 0;
        token->type = token_string;
        token->length = 1;
        do {
            c = next_char(state);
            ++token->length;
            if (c == '\\') {
                next_char(state);
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
                next_char(state);
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
        next_char(state);
        return 0;
    } else if (ispunct(c)) {
        state->is_new_line = 0;
        token->type = token_punc;
        while (ispunct(c) && c != '\0') {
            ++token->length;
            c = next_char(state);            
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
                token->type = token_real_double;
                point = 1;
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
                token->type = token_real_double;
                point = 1;
                ++token->length;
                c = next_char(state);
            } else if ((c == 'e' || c == 'E') && exponent == 0) {
                token->type = token_real_double;
                exponent = 1;
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
    } else if (isspace(c)) {
        token->type = token_space;
        while (isspace(c) && c != '\0') {
            ++token->length;
            c = next_char(state);
        }
        return 0;
    } else {
        next_char(state);
    }
    return 0;
}

static struct preprocessed_token* tokenize(struct options* options, const char* buffer, struct error_list** errors) {
    struct tokenizer_state state;
    state.buffer = buffer;
    state.column = 1;
    state.line = 1;
    state.is_new_line = 1;
    state.offset = 0;
    struct token token = { 0 };
    while (next_token(&state, &token) == 0) {

    }
    return NULL;
}

static struct preprocessed_source* preprocess_file(struct options* options, const char* file) {
    struct preprocessed_source* result = (struct preprocessed_source*)malloc(sizeof(struct preprocessed_source));
    if (result != NULL) {
        result->name = duplicate_string(file);
        result->errors = NULL;
        result->tokens = NULL;
        result->buffer = NULL;
        char* buffer = read_file(file);
        if (buffer != NULL) {
            result->tokens = tokenize(options, buffer, &result->errors);
            result->buffer = buffer;
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

