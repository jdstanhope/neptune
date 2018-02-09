#ifndef _neptune_h_
#define _neptune_h_

#include <stdio.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#define VERSION_BUILD 0

char* duplicate_string(const char* s);

enum error_code {
	error_code_none = 0,
	error_code_invalid_options = 1000,
	error_code_missing_output_argument,
	error_code_missing_include_argument
};

struct error_list {
	enum error_code code;
	char* message;
	char* file;
	int line;
	int column;
	struct error_list* next;
};

struct error_list* add_error_to_list(struct error_list* errors, enum error_code code, const char* message, const char* file, int line, int column);
int printf_errors(FILE* file, struct error_list* errors);
void free_error_list(struct error_list* errors);

#endif
