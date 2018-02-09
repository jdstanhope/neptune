#include "string_list.h"
#include "neptune.h"
#include <stdlib.h>
#include <strings.h>

struct string_list* append_string_to_list(struct string_list* list, const char* string) {
	if (list == NULL) {
		struct string_list* result = (struct string_list*)malloc(sizeof(struct string_list));
		if (result != NULL) {
			result->string = duplicate_string(string);
			result->next = NULL;
		}
		return result;
	} else {
		struct string_list* last = list;
		while (last->next != NULL) {
			last = last->next;
		}
		last->next = append_string_to_list(NULL, string);
		return list;
	}
}

void free_string_list(struct string_list* list) {
	if (list != NULL) {
		if (list->next != NULL) {
			free_string_list(list->next);
		}
		free(list->string);
		free(list);
	}
}
