#ifndef _neptune_string_list_h_
#define _neptune_string_list_h_

struct string_list {
	char* string;
	struct string_list* next;
};

struct string_list* append_string_to_list(struct string_list* list, const char* string);
void free_string_list(struct string_list* list);

#endif


