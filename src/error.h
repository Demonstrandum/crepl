#pragma once

typedef enum {
	NO_ERROR,
	SYNTAX_ERROR,
	PARSE_ERROR,
	TYPE_ERROR,
	EXECUTION_ERROR,
} error_t;


const char *error_name(error_t);

extern error_t ERROR_TYPE;
extern char ERROR_MSG[256];

void handle_error(void);
