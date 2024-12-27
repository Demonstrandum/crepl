#include "defaults.h"
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

DataValue *builtin_sleep(DataValue seconds)
{
	NumberNode *num = type_check("sleep", ARG, T_NUMBER, &seconds);
	if (num == NULL) return NULL;

	NumberNode *time = malloc(sizeof(NumberNode));
	*time = num_to_int(*num);
	if (time->value.i < 0) time->value.i = 0;

	sleep((unsigned)time->value.i);
	return heap_data(T_NUMBER, time);
}

#define MATH_WRAPPER(NAME, FUNC)\
DataValue *builtin_ ##NAME (DataValue input) \
{ \
	NumberNode *num = type_check(#NAME, ARG, T_NUMBER, &input); \
	\
	if (num == NULL) \
		return NULL; \
	\
	NumberNode *new_num = malloc(sizeof(NumberNode)); \
	\
	NumberNode tmp = num_to_float(*num); \
	tmp.value.f = FUNC(tmp.value.f); \
	\
	memcpy(new_num, &tmp, sizeof(NumberNode)); \
	DataValue *result = heap_data(T_NUMBER, new_num); \
	return result; \
}

// This is cheaty, but hey.
fsize nice_sin(fsize alpha)
{
	fsize multiples_of_pi = alpha / M_PI;
	fsize integral = 0;
	fsize fractional = modfl(multiples_of_pi, &integral);
	if (fractional == 0.0f) // i.e. if alpha/pi is a whole number...
		return 0.0f;
	return sin(alpha);
}

fsize gamma_complete(fsize num) {
	if (num == 0)
		return INF;

	fsize integral = 0;
	fsize fractional = modfl(num, &integral);

	if (fractional == 0) {
		if (num < 0)
			return INF;
		fsize res = 1;
		for (fsize i = num - 1; i > 1; --i)
			res *= i;
		return res;
	}

	if (num < 0) {
		// Gamma(x) = (1/x) * Gamma(x + 1)
		fsize acc = 1 / num;
		fsize i = num + 1;
		do {
			acc *= 1 / i;
			i += 1;
		} while (i < 0);

		return acc * tgammal(i);
	}
	return tgammal(num);
}

MATH_WRAPPER(sin, nice_sin)
MATH_WRAPPER(sinh, sinhl)
MATH_WRAPPER(cos, cosl)
MATH_WRAPPER(cosh, coshl)
MATH_WRAPPER(tan, tanl)
MATH_WRAPPER(tanh, tanhl)
MATH_WRAPPER(exp, expl)
MATH_WRAPPER(abs, fabsl)
MATH_WRAPPER(log, log10l)
MATH_WRAPPER(log2, log2l)
MATH_WRAPPER(ln, logl)
MATH_WRAPPER(sqrt, sqrtl)
MATH_WRAPPER(cbrt, cbrtl)
MATH_WRAPPER(acos, acosl)
MATH_WRAPPER(acosh, acoshl)
MATH_WRAPPER(asin, asinl)
MATH_WRAPPER(asinh, asinhl)
MATH_WRAPPER(atan, atanl)
MATH_WRAPPER(atanh, atanhl)
// TODO: atan2, hypot
MATH_WRAPPER(ceil, ceil)
MATH_WRAPPER(floor, floor)
MATH_WRAPPER(Gamma, gamma_complete)

DataValue *builtin_neg(DataValue input)
{
	NumberNode *num = type_check("-", RHS, T_NUMBER, &input);
	if (num == NULL)
		return NULL;
	NumberNode *new_num = malloc(sizeof(NumberNode));
	memcpy(new_num, num, sizeof(NumberNode));
	switch (new_num->type) {
	case INT: {
		new_num->value.i *= -1;
		break;
	}
	case FLOAT: {
		new_num->value.f *= -1.0f;
		break;
	}
	default: {
		ERROR_TYPE = TYPE_ERROR;
		strcpy(ERROR_MSG, "Unsupported number type.");
		return NULL;
	}
	}
	DataValue *result = heap_data(T_NUMBER, new_num);
	return result;
}

DataValue *builtin_pos(DataValue input)
{
	NumberNode *num = type_check("+", RHS, T_NUMBER, &input);
	if (num == NULL)
		return NULL;
	NumberNode *new_num = malloc(sizeof(NumberNode));
	memcpy(new_num, num, sizeof(NumberNode));
	return heap_data(T_NUMBER, new_num);
}

DataValue *builtin_factorial(DataValue input)
{
	NumberNode *num = type_check("!", LHS, T_NUMBER, &input);

	if (num == NULL)
		return NULL;

	NumberNode tmp = num_to_float(*num);
	NumberNode *new_num = malloc(sizeof(NumberNode));
	memcpy(new_num, &tmp, sizeof(NumberNode));
	new_num->value.f = gamma_complete(new_num->value.f + 1);

	DataValue *result = heap_data(T_NUMBER, new_num);
	result->value = new_num;
	return result;
}


#define BINARY_FUNCTION(NAME, OP) \
NumberNode *num_ ## NAME (NumberNode lhs, NumberNode rhs) \
{ \
	NumberNode *upcasted = upcast_pair(lhs, rhs); \
	if (upcasted == NULL) \
		return NULL; \
	\
	NumberNode *result = upcasted + 0; \
	\
	switch (result->type) { \
	case FLOAT: \
		result->value.f = upcasted[0].value.f OP upcasted[1].value.f; \
		break; \
	case INT: \
		result->value.i = upcasted[0].value.i OP upcasted[1].value.i; \
		break; \
	default: { \
		ERROR_TYPE = EXECUTION_ERROR; \
		strcpy(ERROR_MSG, "Unsupported number type."); \
		return NULL; \
	} \
	} \
	result = realloc(result, sizeof(NumberNode)); \
	return result; \
}

BINARY_FUNCTION(add, +)  // `num_add` function.
BINARY_FUNCTION(sub, -)  // `num_sub` function.
BINARY_FUNCTION(mul, *)  // `num_mul` function.

// `num_div` function is different, it always gives a float.
NumberNode *num_div(NumberNode lhs, NumberNode rhs)
{
	NumberNode *result = malloc(sizeof(NumberNode));
	result->type = FLOAT;
	result->value.f = num_to_float(lhs).value.f / num_to_float(rhs).value.f;
	return result;
}

NumberNode *num_pow(NumberNode lhs, NumberNode rhs)
{
	NumberNode *upcasted = upcast_pair(lhs, rhs);
	if (upcasted == NULL)
		return NULL;

	NumberNode *result = upcasted + 0;

	switch (result->type) {
	case FLOAT:
		result->value.f = powl(upcasted[0].value.f, upcasted[1].value.f);
		break;
	case INT:
		result->value.i = upcasted[1].value.i < 0
			? ((fsize)1) / ipow(upcasted[0].value.i, -upcasted[1].value.i)
			:  ipow(upcasted[0].value.i, upcasted[1].value.i);
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
