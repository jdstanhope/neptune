#ifndef _neptune_prepprocesor_h__
#define _neptune_prepprocesor_h__

#include <stdio.h>
#include "neptune.h"
#include "options.h"

enum preprocessed_token_type {
    preprocessed_token_pragma,
    preprocessed_token_include,
    preprocessed_token_define,
    preprocessed_token_if,
    preprocessed_token_ifdef,
    preprocessed_token_elif,
    preprocessed_token_comment,
    preprocessed_token_use,
    preprocessed_token_keyword,
    preprocessed_token_punct,
    preprocessed_token_identifier
};

struct preprocessed_token {
    enum preprocessed_token_type type;
    char* offset;
    size_t length;
    size_t line;
    size_t column;
    struct preprocessed_token* next;
};

struct preprocessed_source {
    char* name;
    char* buffer;
    struct preprocessed_token* tokens;
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
