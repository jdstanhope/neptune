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

static int next_token(struct tokenizer_state* state, struct raw_token** t) {
    char c = *(state->buffer + state->offset);
    if (c == '\0') {
        return -1;
    }

    struct raw_token* token = (struct raw_token*)malloc(sizeof(struct raw_token));
    *t = token;
    token->text = state->buffer + state->offset;
    token->length = 0;
    token->line = state->line;
    token->column = state->column;
    token->type = raw_token_unknown;
    token->next = NULL;

    char p = *(state->buffer + state->offset + 1);
    if (c == '#' && p == '#') {
        state->is_new_line = 0;
        token->type = raw_token_concat;
        token->length = 2;
        next_char(state);
        next_char(state);
        return 0;
    } else if (c == '#') {
        if (state->is_new_line == 1) {
            state->is_new_line = 0;
            token->type = raw_token_directive;
            token->length = 1;
            next_char(state);
        } else {
            token->type = raw_token_stringify;
            token->length = 1;
            next_char(state);
        }
        return 0;
    } else if (c == '/' && p == '*') {
        state->is_new_line = 0;
        token->type = raw_token_comment;
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
        token->type = raw_token_comment;
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
        token->type = raw_token_string;
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
        token->type = raw_token_char;
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
        token->type = raw_token_continue;
        token->length = 1;
        c = next_char(state);
        if (c != '\n') {
            // escaped nothing?
        }
        return 0;
    } else if (ispunct(c)) {
        state->is_new_line = 0;
        token->type = raw_token_punc;
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
        token->type = raw_token_identifier;
        while ((isalnum(c) || c == '_') && c != '\0') {
            ++token->length;
            c = next_char(state);
        }
        return 0;
    } else if (isnumber(c)) {
        state->is_new_line = 0;
        token->type = raw_token_integer;
        int point = 0;
        int exponent = 0;
        if (c == '0') {
            ++token->length;
            c = next_char(state);
            if (c == '.') {
                point = 1;
                token->type = raw_token_real_double;
                ++token->length;
                c = next_char(state);
            } else if (c == 'x' || c == 'X') {
                token->type = raw_token_integer_hex;
                ++token->length;
                c = next_char(state);
                while (ishexnumber(c) && c != '\0') {
                    ++token->length;
                    c = next_char(state);
                }
                return 0;
            } else if (isnumber(c)) {
                token->type = raw_token_integer_octal;
                while (isnumber(c) && c != '\0') {
                    ++token->length;
                    c = next_char(state);
                }
                return 0;
            } else {
                token->type = raw_token_integer;
                return 0;
            }
        } 
        while (isnumber(c) && c != '\0') {
            ++token->length;
            c = next_char(state);
            if (c == '.' && point == 0 && exponent == 0) {
                point = 1;
                token->type = raw_token_real_double;
                ++token->length;
                c = next_char(state);
            } else if ((c == 'e' || c == 'E') && exponent == 0) {
                exponent = 1;
                token->type = raw_token_real_double;
                ++token->length;
                c = next_char(state);
                if (c == '-' || c == '+') {
                    ++token->length;
                    c = next_char(state);
                }
            }
        }
        if (c == 'f' || c == 'F') {
            token->type = raw_token_real_float;
            ++token->length;
            next_char(state);
        } else if (c == 'i') {
            token->type = raw_token_integer_i64;
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
            token->type = raw_token_integer_unsigned;
            ++token->length;
            c = next_char(state);
            if (c == 'l' || c == 'L') {
                token->type = raw_token_integer_unsigned_long;
                ++token->length;
                next_char(state);                
            }
        } else if (c == 'l' || c == 'L') {
            ++token->length;
            c = next_char(state);
            if (point || exponent) {
                token->type = raw_token_real_double_long;  
            } else {
                token->type = raw_token_integer_long;
                if (c == 'u' || c == 'U') {
                    token->type = raw_token_integer_unsigned_long;
                    ++token->length;
                    next_char(state);
                }
            }
        }
        return 0;
    } else if (c == '\n') {
        token->type = raw_token_newline;
        token->length = 1;
        next_char(state);
    } else if (isspace(c)) {
        token->type = raw_token_space;
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

static struct raw_token* tokenize_file(const char* buffer) {
    struct tokenizer_state state;
    state.buffer = buffer;
    state.column = 1;
    state.line = 1;
    state.is_new_line = 1;
    state.offset = 0;

    struct raw_token* head = NULL;
    struct raw_token* previous = NULL;
    struct raw_token* token = NULL;
    while (next_token(&state, &token) == 0) {
        if (head == NULL) {
            head = token;
        }
        if (previous != NULL) {
            previous->next = token;
        }
        previous = token;
    }
    return head;
}

static struct raw_token* next_preprocess_token(struct raw_token* token) {
    struct raw_token* current = token->next;
    while (current != NULL && (current->type == raw_token_space || current->type == raw_token_comment)) {
        current = current->next;
    }
    return current;
}

static void put_token(struct tokenizer_state* state, struct raw_token* token) {
    if (token->type == raw_token_directive) {
        state->is_new_line = 1;
    }
    state->offset = state->offset - token->length;
}

static struct preprocessed_node* make_node(enum preprocessed_node_type type) {
    struct preprocessed_node* result = (struct preprocessed_node *)malloc(sizeof(struct preprocessed_node));
    if (result != NULL) {
        result->type = type;
        result->head = NULL;
        result->tail = NULL;
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

static struct raw_token* next_identifier(struct raw_token* token) {
    while (token != NULL && token->type != raw_token_identifier) {
        token = next_preprocess_token(token);
    }
    return token;
}

static struct raw_token* next_newline(struct raw_token* token) {
    while (token != NULL && token->type != raw_token_newline) {
        token = next_preprocess_token(token);
        if (token != NULL && token->type == raw_token_continue) {
            token = next_preprocess_token(token);
            if (token != NULL && token->type == raw_token_newline) {
                token = next_preprocess_token(token);
            }
        }
    }
    return token;
}

static struct preprocessed_node* parse_node(struct raw_token* token);

static struct preprocessed_node* parse_unknown(struct raw_token* token) {
    struct preprocessed_node* unknown = make_node(preprocessed_node_unknown);
    unknown->head = token;
    unknown->tail = next_newline(token);
    return unknown;
}

static struct preprocessed_node* parse_include(struct raw_token* token) {
    // TODO: needs to actually parse structure
    struct preprocessed_node* node = parse_unknown(token);
    node->type = preprocessed_node_include;
    return node;
}

static struct preprocessed_node* parse_ifdef(struct raw_token* token) {
    struct preprocessed_node* parent = make_node(preprocessed_node_ifdef);
    parent->head = token;
    /* read to body of conditional */
    parent->tail = next_newline(token);
    struct raw_token* next = next_preprocess_token(parent->tail);
    while (next != NULL) {
        if (next->type == raw_token_directive) {
            struct raw_token* identifier = next_identifier(next);
            if (identifier != NULL && identifier->type == raw_token_identifier) {
                if (strncmp(identifier->text, "endif", identifier->length) == 0) {
                    /* done when we hit an #endif */
                    parent->tail = next_newline(identifier);
                    return parent;
                } else {
                    /* some other preprocessor directive */
                    struct preprocessed_node* child = parse_node(next);
                    append_child_node(parent, child);
                    next = child->tail;
                }
            } else {
                /* most likely an error of some kind */
                struct preprocessed_node* child = parse_node(next);
                append_child_node(parent, child);
                next = child->tail;
            }
        } else {
            /* block of code */
            struct preprocessed_node* child = parse_node(next);
            append_child_node(parent, child);
            next = child->tail;
        }
        next = next_preprocess_token(next);
    }
    // TODO: report error, unterminated parent
    parent->tail = next;
    return parent;
}

static struct preprocessed_node* parse_ifndef(struct raw_token* token) {
    struct preprocessed_node* parent = parse_ifdef(token);
    parent->type = preprocessed_node_ifndef;
    return parent;
}

static struct preprocessed_node* parse_if(struct raw_token* token) {
    struct preprocessed_node* root = make_node(preprocessed_node_if);
    struct preprocessed_node* parent = root;
    root->head = token;
    /* read to body of conditional */
    root->tail = next_newline(token);
    struct raw_token* next = next_preprocess_token(root->tail);
    while (next != NULL) {
        if (next->type == raw_token_directive) {
            struct raw_token* identifier = next_identifier(next);
            if (identifier != NULL && identifier->type == raw_token_identifier) {
                if (strncmp(identifier->text, "endif", identifier->length) == 0) {
                    /* done when we hit an #endif */
                    root->tail = next_newline(identifier);
                    parent->tail = root->tail;
                    return root;
                } else if (strncmp(identifier->text, "else", identifier->length) == 0) {
                    /* switch to else */
                    struct preprocessed_node* child = make_node(preprocessed_node_else);
                    child->head = next;
                    child->tail = next_newline(identifier);
                    append_child_node(parent, child);
                    parent = child;
                    next = child->tail;
                } else if (strncmp(identifier->text, "elif", identifier->length) == 0) {
                    /* switch to elif */
                    struct preprocessed_node* child = make_node(preprocessed_node_elif);
                    child->head = next;
                    child->tail = next_newline(identifier);
                    append_child_node(parent, child);
                    parent = child;
                    next = child->tail;
                } else {
                    /* some other preprocessor directive */
                    struct preprocessed_node* child = parse_node(next);
                    append_child_node(parent, child);
                    parent->tail = child->tail;
                    next = child->tail;
                }
            } else {
                /* most likely an error of some kind */
                struct preprocessed_node* child = parse_node(next);
                append_child_node(parent, child);
                parent->tail = child->tail;
                next = child->tail;
            }
        } else {
            /* block of code */
            struct preprocessed_node* child = parse_node(next);
            append_child_node(parent, child);
            /* update parent's end point */
            parent->tail = child->tail;
            next = child->tail;
        }
        next = next_preprocess_token(next);
    }
    // TODO: report error, unterminated root
    root->tail = next;
    return root;
}

static struct preprocessed_node* parse_error(struct raw_token* token) {
    // TODO: needs to actually parse structure
    struct preprocessed_node* node = parse_unknown(token);
    node->type = preprocessed_node_error;
    return node;
}

static struct preprocessed_node* parse_pragma(struct raw_token* token) {
    // TODO: needs to actually parse structure
    struct preprocessed_node* node = parse_unknown(token);
    node->type = preprocessed_node_pragma;
    return node;
}

static struct preprocessed_node* parse_define(struct raw_token* token) {
    // TODO: needs to actually parse structure
    struct preprocessed_node* node = parse_unknown(token);
    node->type = preprocessed_node_define;
    return node;
}

static struct preprocessed_node* parse_undef(struct raw_token* token) {
    // TODO: needs to actually parse structure
    struct preprocessed_node* node = parse_unknown(token);
    node->type = preprocessed_node_undef;
    return node;
}

static struct preprocessed_node* parse_directive(struct raw_token* token) {
    struct raw_token* directive = next_preprocess_token(token);
    if (directive->type == raw_token_identifier) {
        if (strncmp(directive->text, "include", 8) == 0) {
            return parse_include(token);
        } else if (strncmp(directive->text, "ifdef", 5) == 0) {
            return parse_ifdef(token);
        } else if (strncmp(directive->text, "ifndef", 6) == 0) {
            return parse_ifndef(token);
        } else if (strncmp(directive->text, "if", 2) == 0) {
            return parse_if(token);
        } else if (strncmp(directive->text, "error", 5) == 0) {
            return parse_error(token);
        } else if (strncmp(directive->text, "pragma", 6) == 0) {
            return parse_pragma(token);
        } else if (strncmp(directive->text, "define", 6) == 0) {
            return parse_define(token);
        } else if (strncmp(directive->text, "undef", 5) == 0) {
            return parse_undef(token);
        } else {
            return parse_unknown(token);
        }
    } else {
        return parse_unknown(token);
    }
    return NULL;
}

static struct preprocessed_node* parse_block(struct raw_token* token) {
    struct preprocessed_node* block = make_node(preprocessed_node_block);
    block->head = token;
    block->tail = token; /* incase the block is a newline */
    struct raw_token* next = next_preprocess_token(token);
    while (next != NULL && next->type != raw_token_directive) {
        block->tail = next;
        next = next_preprocess_token(next);
    }
    return block;
}

static struct preprocessed_node* parse_node(struct raw_token* token) {
    if (token->type == raw_token_directive) {
        return parse_directive(token);
    } else {
        return parse_block(token);
    }
}

static struct preprocessed_node* preprocess_tokens(struct raw_token* head) {
    struct preprocessed_node* root = make_node(preprocessed_node_root);
    root->head = head;
    struct raw_token* token = head;
    while (token != NULL) {
        struct preprocessed_node* child = parse_node(token);
        append_child_node(root, child);
        token = next_preprocess_token(child->tail);
    }
    return root;
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
            struct raw_token* token = tokenize_file(buffer);
            if (token != NULL) {
                result->root = preprocess_tokens(token);
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

static void print_node(FILE* file, int depth, struct preprocessed_node* node) {
    while (node != NULL) {
        switch (node->type) {
            case preprocessed_node_root:
                fprintf(file, "%*croot\n", 2*depth, ' ');        
                print_node(file, depth+1, node->first);
                break;
            case preprocessed_node_include:
                fprintf(file, "%*cinclude\n", 2*depth, ' ');        
                break;
            case preprocessed_node_define:
                fprintf(file, "%*cdefine\n", 2*depth, ' ');        
                break;
            case preprocessed_node_undef:
                fprintf(file, "%*cundef\n", 2*depth, ' ');        
                break;
            case preprocessed_node_ifdef:
                fprintf(file, "%*cifdef\n", 2*depth, ' ');
                print_node(file, depth+1, node->first);
                break;
            case preprocessed_node_ifndef:
                fprintf(file, "%*cifndef\n", 2*depth, ' ');
                print_node(file, depth+1, node->first);
                break;
            case preprocessed_node_else:
                fprintf(file, "%*celse\n", 2*depth, ' ');
                print_node(file, depth+1, node->first);
                break;
            case preprocessed_node_elif:
                fprintf(file, "%*celif\n", 2*depth, ' ');
                print_node(file, depth+1, node->first);
                break;
            case preprocessed_node_if:
                fprintf(file, "%*cif\n", 2*depth, ' ');
                print_node(file, depth+1, node->first);
                break;
            case preprocessed_node_pragma:
                fprintf(file, "%*cpragma\n", 2*depth, ' ');        
                break;
            case preprocessed_node_error:
                fprintf(file, "%*cerror\n", 2*depth, ' ');
                break;
            case preprocessed_node_unknown:
                fprintf(file, "%*cunknown\n", 2*depth, ' ');        
                break;
            case preprocessed_node_line:
                fprintf(file, "%*cline\n", 2*depth, ' ');        
                break;
            case preprocessed_node_block:
                fprintf(file, "%*cblock\n", 2*depth, ' ');        
                break;
        }
        node = node->next;
    }
}

void print_preprocessed_source(FILE* file, struct preprocessed_source* source) {
    if (file != NULL && source != NULL) {
        if (source->root != NULL) {
            print_node(file, 0, source->root);
            // print_node(file, 0, source->root);
            // struct raw_token* token = source->root->head;
            // while (token != NULL) {
            //     for (size_t i=0 ; i<token->length ; ++i) {
            //         fputc(*(token->text+i), file);
            //     }
            //     token = token->next;
            // }
        }
    }
}

static void free_raw_tokens(struct raw_token* token) {
    struct raw_token* next = token;
    while (next != NULL) {
        struct raw_token* temp = next;
        next = temp->next;
        free(temp);
    }
}

static void free_processed_node(struct preprocessed_node* node) {
    if (node != NULL) {
        free_processed_node(node->first);
        free_processed_node(node->next);
        free(node);
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
        free_raw_tokens(source->root->head);
        free_processed_node(source->root);
        free(source);
    }
}

