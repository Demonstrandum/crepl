#pragma once

#include <math.h>

#include "prelude.h"
#include "parse.h"
#include "execute.h"
#include "error.h"

NumberNode num_to_float(NumberNode);
NumberNode num_to_int(NumberNode);
NumberNode *upcast_pair(NumberNode, NumberNode);

DataValue *builtin_sin(DataValue);

extern FnPtr builtin_fns[];

NumberNode *num_add(NumberNode, NumberNode);
NumberNode *num_sub(NumberNode, NumberNode);
NumberNode *num_mul(NumberNode, NumberNode);
NumberNode *num_div(NumberNode, NumberNode);

