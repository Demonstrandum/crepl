#pragma once

#include "prelude.h"
#include "parse.h"
#include "error.h"

NumberNode num_to_float(NumberNode);
NumberNode num_to_int(NumberNode);
NumberNode *upcast_pair(NumberNode, NumberNode);
NumberNode *num_add(NumberNode, NumberNode);

