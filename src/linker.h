#ifndef _neptune_linker_h_
#define _neptune_linker_h_

#include "neptune.h"
#include "options.h"
#include "compiler.h"

struct linked_exectuable {
    struct error_list* errors;
};

struct linked_exectuable* link_objects(struct object_code_list* objects);
int save_executable(struct linked_exectuable* executable);

#endif
