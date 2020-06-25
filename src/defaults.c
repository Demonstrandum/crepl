#include "defaults.h"

ssize ipow(ssize base, usize exp)
{
    ssize result = 1;
    do {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    } while (true);

    return result;
}

char *trim(char *str)
{
	char *p = strdup(str);
	while (isspace(*p))
        ++p;

	char *end = p + strlen(p) - 1;
    while (end > p && isspace(*end))
        --end;

    *(end + 1) = '\0';
    return p;
}


char *downcase(char *str)
{
	char *p = strdup(str);
	char *start = p;
	for (; *p; ++p) *p = tolower(*p);
	return start;
}
