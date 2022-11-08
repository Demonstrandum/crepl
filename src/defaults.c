#include "defaults.h"

const bool debug =
#ifdef DEBUG
	true;
#else
	false;
#endif

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

byte *remove_all_bytes(const byte *str, byte chr)
{
	byte *new = strdup(str);
	size_t str_len = strlen(str);
	size_t new_len = 0;

	for (size_t i = 0; i < str_len; ++i)
		if (str[i] != chr) new[new_len++] = str[i];

	new[new_len] = '\0';
	return new;
}

byte *trim(const byte *str)
{
	byte *p = strdup(str);
	while (isspace(*p))
        ++p;

	byte *end = p + strlen(p) - 1;
    while (end > p && isspace(*end))
        --end;

    *(end + 1) = '\0';
    return p;
}


byte *downcase(const byte *str)
{
	byte *p = strdup(str);
	byte *start = p;
	for (; *p; ++p) *p = tolower(*p);
	return start;
}
