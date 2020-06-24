#include "builtin.h"

NumberNode num_to_float(NumberNode num)
{
	NumberNode result = num; // Copy.

	switch (num.type) {
	case INT:
		result.type = FLOAT;
		result.value.f = (fsize)num.value.i;
		break;
	case FLOAT:
		break;
	default: {
		ERROR_TYPE = EXECUTION_ERROR;
		strcpy(ERROR_MSG, "Unsupported number type.");
	}
	}
	return result;
}

NumberNode num_to_int(NumberNode num)
{
	NumberNode result = num; // Copy.

	switch (num.type) {
	case FLOAT:
		result.type = INT;
		result.value.i = (ssize)num.value.f;
		break;
	case INT:
		break;
	default: {
		ERROR_TYPE = EXECUTION_ERROR;
		strcpy(ERROR_MSG, "Unsupported number type.");
	}
	}
	return result;
}

NumberNode *upcast_pair(NumberNode lhs, NumberNode rhs)
{
	NumberNode *pair = malloc(2 * sizeof(NumberNode));

	switch (lhs.type) {
	case FLOAT: {
		pair[0] = lhs;
		pair[1] = num_to_float(rhs);
		break;
	}
	case INT: {
		switch (rhs.type) {
		case INT: {
			pair[0] = lhs;
			pair[1] = rhs;
			break;
		}
		case FLOAT: {
			pair[0] = num_to_float(lhs);
			pair[1] = rhs;
			break;
		}
		default:
			goto error;
		}
		break;
	}
	error:
	default: {
		ERROR_TYPE = EXECUTION_ERROR;
		strcpy(ERROR_MSG, "Unsupported number type.");
		return NULL;
	}
	}
	return pair;
}

NumberNode *num_add(NumberNode lhs, NumberNode rhs)
{
	NumberNode *upcasted = upcast_pair(lhs, rhs);
	if (upcasted == NULL)
		return NULL;

	NumberNode *result = upcasted + 0;

	switch (result->type) {
	case FLOAT:
		result->value.f = upcasted[0].value.f + upcasted[1].value.f;
		break;
	case INT:
		result->value.i = upcasted[0].value.i + upcasted[1].value.i;
		break;
	default: {
		ERROR_TYPE = EXECUTION_ERROR;
		strcpy(ERROR_MSG, "Unsupported number type.");
		return NULL;
	}
	}
	result = realloc(result, sizeof(NumberNode));
	return result;
}


