#pragma once

#include <math.h>
#include <unistd.h>

#include "defaults.h"
#include "parse.h"
#include "execute.h"
#include "error.h"

NumberNode num_to_float(NumberNode);
NumberNode num_to_int(NumberNode);
NumberNode *upcast_pair(NumberNode, NumberNode);

fsize gamma_func(float, fsize);
fsize gammae(fsize);
DataValue *builtin_sleep(DataValue);
DataValue *builtin_sin(DataValue);
DataValue *builtin_sinh(DataValue);
DataValue *builtin_cos(DataValue);
DataValue *builtin_cosh(DataValue);
DataValue *builtin_tan(DataValue);
DataValue *builtin_tanh(DataValue);
DataValue *builtin_exp(DataValue);
DataValue *builtin_abs(DataValue);
DataValue *builtin_log(DataValue);
DataValue *builtin_log2(DataValue);
DataValue *builtin_ln(DataValue);
DataValue *builtin_sqrt(DataValue);
DataValue *builtin_cbrt(DataValue);
DataValue *builtin_acos(DataValue);
DataValue *builtin_acosh(DataValue);
DataValue *builtin_asin(DataValue);
DataValue *builtin_asinh(DataValue);
DataValue *builtin_atan(DataValue);
DataValue *builtin_atanh(DataValue);
DataValue *builtin_ceil(DataValue);
DataValue *builtin_floor(DataValue);
DataValue *builtin_factorial(DataValue);
DataValue *builtin_neg(DataValue);
DataValue *builtin_pos(DataValue);
DataValue *builtin_Gamma(DataValue);

NumberNode *num_add(NumberNode, NumberNode);
NumberNode *num_sub(NumberNode, NumberNode);
NumberNode *num_mul(NumberNode, NumberNode);
NumberNode *num_div(NumberNode, NumberNode);
NumberNode *num_pow(NumberNode, NumberNode);

#define FUNC_PAIR(NAME) { #NAME, { builtin_##NAME } }

struct _func_name_pair {
	char *name;
	FnPtr function;
};

static const struct _func_name_pair builtin_fns[] = {
	FUNC_PAIR(sleep),
	FUNC_PAIR(sin),
	FUNC_PAIR(sinh),
	FUNC_PAIR(cos),
	FUNC_PAIR(cosh),
	FUNC_PAIR(tan),
	FUNC_PAIR(tanh),
	FUNC_PAIR(exp),
	FUNC_PAIR(abs),
	FUNC_PAIR(log),
	{ "log10", { builtin_log } },
	FUNC_PAIR(log2),
	FUNC_PAIR(ln),
	FUNC_PAIR(sqrt),
	FUNC_PAIR(cbrt),
	FUNC_PAIR(acos),
	{ "arccos", { builtin_acos } },
	FUNC_PAIR(acosh),
	{ "arccosh", { builtin_acosh } },
	FUNC_PAIR(asin),
	{ "arcsin", { builtin_asin } },
	FUNC_PAIR(asinh),
	{ "arcsinh", { builtin_asinh } },
	FUNC_PAIR(atan),
	{ "arctan", { builtin_atan } },
	FUNC_PAIR(atanh),
	{ "arctanh", { builtin_atanh } },
	FUNC_PAIR(ceil),
	FUNC_PAIR(floor),
	FUNC_PAIR(factorial),
	{ "!", { builtin_factorial } },
	FUNC_PAIR(neg),
	{ "-", { builtin_neg } },
	FUNC_PAIR(pos),
	{ "+", { builtin_pos } },
	FUNC_PAIR(Gamma),
};
