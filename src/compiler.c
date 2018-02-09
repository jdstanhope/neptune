#include "compiler.h"
#include <stdlib.h>

struct object_code* compile(struct options* options) {
	struct object_code* result = (struct object_code*)malloc(sizeof(struct object_code));
	if (result != NULL) {
		result->options = options; /* not to be freed, this is a shared ptr */
		result->errors = NULL;
	}
    return result;
}

int save_object(struct object_code* object) {
	return 0;
}

void free_object(struct object_code* object) {
	if (object != NULL) {
		free_error_list(object->errors);
		free(object);
	}
}

struct object_code_list* load_objects(struct options* options) {
	return NULL;
}

void free_objects(struct object_code_list* objects) {
	
}
