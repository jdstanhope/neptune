#include <stdlib.h>
#include <stdio.h>

#include "neptune.h"
#include "options.h"
#include "preprocessor.h"
#include "compiler.h"
#include "linker.h"
#include "string_list.h"

/**
 * Need more comments.
 */
int main(int argc, const char* argv[]) {
	int exitCode = 0;
	struct options* options = parse_options(argc, argv);
	if (options == NULL) {
		fprintf(stderr, "error(%d): invalid command line options\n", error_code_invalid_options);
		return -1;
	}
	switch (options->action) {
		case options_action_error:
			exitCode = printf_errors(stderr, options->errors);
			break;
		case options_action_help:
			fprintf(stdout, "help ... todo\n");
			break;
		case options_action_version:
			fprintf(stdout, "neptune %d.%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_BUILD);
			break;
		case options_action_preprocess: {
			struct preprocessed_source_list* list = preprocess(options);
			struct preprocessed_source_list* current = list;
			while (current != NULL) {
				print_preprocessed_source(stdout, current->source);				
				current = current->next;
			}
			free_preprocessed_source_list(list);
			break; }
		case options_action_compile: {
			struct object_code* obj = compile(options);
			if (obj != NULL) {
				if (obj->errors != NULL) {
					exitCode = printf_errors(stderr, obj->errors);
				} else {
					exitCode = save_object(obj);
				}
				free_object(obj);
			}
			break; }
		case options_action_link: {
			struct object_code_list* objs = load_objects(options);
			if (objs != NULL) {
				if (objs->errors != NULL) {
					exitCode = printf_errors(stderr, objs->errors);
				} else {
					struct linked_exectuable* exec = link_objects(objs);
					if (exec->errors != NULL) {
					} else {
						exitCode = save_executable(exec);
					}
				}
				free_objects(objs);
			}
			break; }
		case options_action_compile_and_link:
			// compile_and_link(options);
			break;
	}
	free_options(options);
	return exitCode;
}
