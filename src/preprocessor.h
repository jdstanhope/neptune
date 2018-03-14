#ifndef _neptune_prepprocesor_h__
#define _neptune_prepprocesor_h__

#include <stdio.h>
#include "neptune.h"
#include "options.h"

enum raw_token_type {
    raw_token_unknown,
    raw_token_space,
    raw_token_newline,
    raw_token_directive,
    raw_token_stringify,
    raw_token_concat,
    raw_token_comment,
    raw_token_continue,
    raw_token_punc,
    raw_token_identifier,
    raw_token_string,
    raw_token_char,
    raw_token_integer,
    raw_token_integer_hex,
    raw_token_integer_octal,
    raw_token_integer_unsigned,
    raw_token_integer_long,
    raw_token_integer_unsigned_long,
    raw_token_integer_i64,
    raw_token_real_float,
    raw_token_real_double,
    raw_token_real_double_long,
};

struct raw_token {
    enum raw_token_type type;
    char* text;
    size_t length;
    unsigned int line;
    unsigned int column;
    struct raw_token* next;
};

enum preprocessed_node_type {
    preprocessed_node_root,
    preprocessed_node_pragma,
    preprocessed_node_include,
    preprocessed_node_define,
    preprocessed_node_undef,
    preprocessed_node_expr,
    preprocessed_node_if,
    preprocessed_node_ifdef,
    preprocessed_node_ifndef,
    preprocessed_node_else,
    preprocessed_node_elif,
    preprocessed_node_line,
    preprocessed_node_error,
    preprocessed_node_unknown,
    preprocessed_node_comment,
    preprocessed_node_block,
    preprocessed_node_token
};

struct preprocessed_node {
    enum preprocessed_node_type type;
    union _value {
        struct _include {
            char* name;
            int scope;
        } include;
    } value;
    struct raw_token* head;
    struct raw_token* tail;
    struct preprocessed_node* next;
    struct preprocessed_node* first;
};

struct preprocessed_source {
    char* name;
    char* buffer;
    struct preprocessed_node* root;
    struct error_list* errors;
};

struct preprocessed_source_list {
    struct preprocessed_source* source;
    struct preprocessed_source_list* next;
};

struct preprocessed_source_list* preprocess(struct options* options);
void print_preprocessed_source(FILE* file, struct preprocessed_source* source);
void free_preprocessed_source_list(struct preprocessed_source_list* sources);
void free_preprocessed_source(struct preprocessed_source* source);

#endif 
