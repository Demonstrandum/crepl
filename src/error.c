#include "error.h"

#include <stdio.h>
#include <string.h>

const char *error_name(error_t err)
{
	switch (err) {
	case NO_ERROR:
		return "IMPROPERLY REPORTED ERROR";
	case SYNTAX_ERROR:
		return "Syntax Error";
	case PARSE_ERROR:
		return "Grammar Error";
	case EXECUTION_ERROR:
		return "Error while executing";
	default:
		return "UNKNOWN ERROR";
	}
}

#define DEFAULT_ERROR_MSG "No errors reported."

error_t ERROR_TYPE = NO_ERROR;
char ERROR_MSG[256] = DEFAULT_ERROR_MSG;

void handle_error()
{
	// Display error.
	printf("\033[31;1m%s\033[0m: %s\n",
		error_name(ERROR_TYPE),
		ERROR_MSG);
	// Reset error values.
	ERROR_TYPE = NO_ERROR;
	strcpy(ERROR_MSG, DEFAULT_ERROR_MSG);
}

