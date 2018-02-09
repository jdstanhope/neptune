#ifndef _neptune_options_h_
#define _neptune_options_h_

#include "neptune.h"

enum options_action {
	options_action_help,
	options_action_error,
	options_action_version,
	options_action_compile,
	options_action_compile_and_link,
	options_action_link
};

struct options {
	enum options_action action;
	struct error_list* errors;
	struct string_list* includes;
	struct string_list* inputs;
	char* output;
};

struct options* parse_options(int argc, const char* argv[]);
void free_options(struct options* options);

#endif
