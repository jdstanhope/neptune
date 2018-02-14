#include "neptune.h"
#include "options.h"
#include "string_list.h"
#include <stdlib.h>
#include <strings.h>

static const char* next_arg(int argc, const char* argv[], int* index, size_t* offset) {
	if (*index >= argc) {
		return NULL;
	}
	if (*offset == 0) {
		const char* arg = argv[*index];
		size_t i = 0;
		size_t len = strlen(arg);
		while (i < len && arg[i] != '=') {
			++i;
		}
		if (i == len) {
			*offset = 0;
			*index = *index + 1;
			return arg;
		} else {
			*offset = i + 1;
			return arg;
		}
	} else {
		const char* result = argv[*index] + *offset;
		*offset = 0;
		*index = *index + 1;
		return result;
	}
}

struct options* parse_options(int argc, const char* argv[]) {
	struct options* result = (struct options*)malloc(sizeof(struct options));
	if (result == NULL) {
		return NULL;
	}
	result->action = options_action_help;
	result->errors = NULL;
	result->includes = NULL;
	result->inputs = NULL;
	result->output = NULL;

	int index = 1;
	size_t offset = 0;
	const char* arg = next_arg(argc, argv, &index, &offset);
	while (arg != NULL) {
		if (arg[0] == '-') {
			if (strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
				result->action = options_action_version;
			} else if (strcmp(arg, "-c") == 0) {
				result->action = options_action_compile;
			} else if (strcmp(arg, "-E") == 0) {
				result->action = options_action_preprocess;
			} else if (strcmp(arg, "-o") == 0) {
				const char* output = next_arg(argc, argv, &index, &offset);
				if (output != NULL) {
					result->output = duplicate_string(arg);
				} else {
					result->action = options_action_error;
					result->errors = add_error_to_list(result->errors, error_code_missing_output_argument, "invalid usage of -o, missing argument", NULL, 0, 0);
				}
			} else if (strncmp(arg, "-I", 2) == 0) {
				if (strlen(arg) > 2) {
					result->includes = append_string_to_list(result->inputs, arg + 2);
				} else {
					const char* include = next_arg(argc, argv, &index, &offset);
					if (include != NULL) {
						result->includes = append_string_to_list(result->inputs, include);
					} else {
						result->action = options_action_error;
						result->errors = add_error_to_list(result->errors, error_code_missing_include_argument, "invalid usage of -I, missing argument", NULL, 0, 0);
					}
				}
			} else if (strncmp(arg, "-D", 2) == 0) {
			} else {
				result->action = options_action_help;
			}
		} else {
			result->inputs = append_string_to_list(result->inputs, arg);
		}
		arg = next_arg(argc, argv, &index, &offset);
	}
	return result;
}

void free_options(struct options* options) {
	if (options != NULL) {
		free_string_list(options->includes);
		free_string_list(options->inputs);
		free_error_list(options->errors);
		free(options->output);
		free(options);
	}
}
