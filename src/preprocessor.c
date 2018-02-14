#include <stdlib.h>
#include <stdio.h>
#include "preprocessor.h"

static const char* read_file(const char* path) {
    char* result = NULL;
    FILE* file = fopen(path, "r+");
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        long end = ftell(file);
        fseek(file, 0, SEEK_SET);
        void* buffer = malloc(sizeof(char)*(end + 1));
        size_t read = fread(buffer, 1, end, file);
        if (read == end) {
            result = buffer;
            result[end] = '\0';
        } else {
            free(buffer);
        }
        fclose(file);
    }
    return result;
}

static struct preprocessed_source* preprocess_file(struct options* options, const char* file) {
    struct preprocessed_source* result = (struct preprocessed_source*)malloc(sizeof(struct preprocessed_source));
    if (result != NULL) {
        result->name = duplicate_string(file);
        result->buffer = read_file(file);
        result->errors = NULL;
        result->tokens = NULL;
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
            fprintf(file, source->buffer);
        }
    }
}

void free_preprocessed_source_list(struct preprocessed_source_list* sources) {
    if (sources != NULL) {
        free_preprocessed_source_list(sources->next);
        free_preprocessed_source(sources->source);
    }
}

void free_preprocessed_source(struct preprocessed_source* source) {
    if (source != NULL) {
        free_error_list(source->errors);
        free(source->buffer);
        free(source);
    }
}

