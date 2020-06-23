#include "prelude.h"

char *trim(char *str)
{
	while (isspace(*str))
        ++str;

	char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        --end;

    *(end + 1) = '\0';
    return str;
}


char *downcase(char *str)
{
	char *p = strdup(str);
	char *start = p;
	for (; *p; ++p) *p = tolower(*p);
	return start;
}
