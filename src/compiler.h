#ifndef _neptune_compiler_h_
#define _neptune_compiler_h_

#include "neptune.h"
#include "options.h"

struct object_code {
	struct options* options;
	struct error_list* errors;    
};

struct object_code_list {
	struct object_code* code;
	struct error_list* errors;    
	struct object_code_list* next;
};

struct object_code* compile(struct options* options);
int save_object(struct object_code* object);
void free_object(struct object_code* object);

struct object_code_list* load_objects(struct options* options);
void free_objects(struct object_code_list* objects);

#endif
