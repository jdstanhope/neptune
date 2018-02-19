#ifndef _neptune_prepprocesor_h__
#define _neptune_prepprocesor_h__

#include <stdio.h>
#include "neptune.h"
#include "options.h"

enum preprocessed_node_type {
    preprocessed_node_root,
    preprocessed_node_pragma,
    preprocessed_node_include,
    preprocessed_node_define,
    preprocessed_node_if,
    preprocessed_node_ifdef,
    preprocessed_node_else,
    preprocessed_node_elif,
    preprocessed_node_comment,
    preprocessed_node_use,
    preprocessed_node_code
};

struct preprocessed_node {
    enum preprocessed_node_type type;
    char* offset;
    size_t length;
    size_t line;
    size_t column;
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
