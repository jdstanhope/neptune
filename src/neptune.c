#include "neptune.h"
#include <stdlib.h>
#include <strings.h>

char* duplicate_string_n(const char* s, size_t len) {
	if (len > 0 && s != NULL) {
		char* result = malloc(sizeof(char)*(len + 1));
		if (result != NULL) {
			strncpy(result, s, len);
			result[len] = '\0';
			return result;
		}
	}
	return NULL;
}

char* duplicate_string(const char* s) {
	if (s != NULL) {
		size_t len = strlen(s);
		if (len > 0) {
			return duplicate_string_n(s, len);
		}
	}
	return NULL;
}

struct error_list* add_error_to_list(struct error_list* errors, enum error_code code, const char* message, const char* file, int line, int column) {
	struct error_list* result = (struct error_list*)malloc(sizeof(struct error_list));
	if (result == NULL) {
		return NULL;
	}
	if (errors == NULL) {
		result->code = code;
		result->message = duplicate_string(message);
		result->file = duplicate_string(file);
		result->line = line;
		result->column = column;
		result->next = NULL;
		return result;
	} else {
		struct error_list* tail = errors;
		while (tail->next != NULL) {
			tail = tail->next;
		}
		tail->next = add_error_to_list(NULL, code, message, file, line, column);
		return errors;
	}
}

int printf_errors(FILE* file, struct error_list* errors) {
	struct error_list* current = errors;
	while (current != NULL)  {
		if (current->file != NULL) {
			fprintf(file, "error(%d): %s\n", current->code, current->message);
		} else {
			fprintf(file, "error(%d): %s\n", current->code, current->message);
		}
		current = current->next;
	}
	return -1;
}

void free_error_list(struct error_list* errors) {
	if (errors != NULL) {
		if (errors->next != NULL) {
			free_error_list(errors->next);
		}
		free(errors->message);
		free(errors->file);
	}
}
