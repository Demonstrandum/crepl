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
	DataValue *result = wrap_data(T_NUMBER, new_num); \
	return result; \
}

// This is cheaty, but hey oh.
fsize nice_sin(fsize alpha)
{
	fsize multiples_of_pi = alpha / M_PI;
	fsize integral = 0;
	fsize fractional = modfl(multiples_of_pi, &integral);
	if (fractional == 0.0f) // i.e. if alpha/pi is a whole number...
		return 0.0f;
	return sin(alpha);
}

#define gamma_upper_bound 1000
#define gamma_resolution 1000000
#define gamma_h 0.001 //upper bound / resolution

fsize gamma_integrad(float x, fsize num) {
	return pow(x, num) * exp(-x);
}

fsize gammae(fsize num) {
	fsize sum = 0;

	for (int i = 1; i < gamma_resolution; i++) {
    	sum += gamma_integrad(i*gamma_h, num);
    }

    return 0.5 * gamma_h * (gamma_integrad(0, num) + gamma_integrad(gamma_upper_bound, num) + 2 * sum);
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
	DataValue *result = wrap_data(T_NUMBER, new_num);
	return result;
}

DataValue *builtin_factorial(DataValue input)
{
	NumberNode *num = type_check("!", LHS, T_NUMBER, &input);

	NumberNode tmp = num_to_float(*num);
	NumberNode *new_num = malloc(sizeof(NumberNode));
	memcpy(new_num, &tmp, sizeof(NumberNode));

	DataValue *result = wrap_data(T_NUMBER, new_num);
	if (num == NULL)
		return NULL;

	fsize integral = 0;
	fsize fractional = modfl(num->value.f, &integral);
	if ((num->type == FLOAT && (num->value.f < 0))
	||  (num->type == INT && num->value.i < 0)) {
		ERROR_TYPE = EXECUTION_ERROR;
		strcpy(ERROR_MSG,
			"factorial (`!') is only defined for positve real numbers.");
	} else if ((num->type == FLOAT && (fractional != 0))) {
		new_num->value.f = gammae(num->value.f);
		result->value = new_num;
		return result;
		}

	if (new_num->value.f == 0) {
		new_num->value.f = 1;
		result->value = new_num;
		return result;
	}
	ssize i = new_num->value.f - 1;
	while (i > 1) {
		new_num->value.f *= i;
		--i;
	}
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

