#include "execute.h"
#include "error.h"
#include "parse.h"
#include "builtin.h"
#include "displays.h"

/// Takes in an execution context (ctx) and a
/// statement as produced by the parser (stmt).
/// Returns what it evaluates to.
DataValue *execute(Context *ctx, const ParseNode *stmt)
{
	DataValue *data = malloc(sizeof(DataValue));
	switch (stmt->type) {
	case NUMBER_NODE: {
		data->type = T_NUMBER;
		data->value = malloc(sizeof(NumberNode));
		memcpy(data->value, &stmt->node.number, sizeof(NumberNode));
		break;
	}
	case BINARY_NODE: {
		IdentNode ident = stmt->node.binary.callee->node.ident;
		if (stmt->node.binary.callee->type != IDENT_NODE) {
			ERROR_TYPE = EXECUTION_ERROR;
			strcpy(ERROR_MSG, "Binary operation has non-ident callee.");
			return NULL;
		}
		// How to evaluate specific operators.
		char *op = ident.value;
		DataValue *lhs = execute(ctx, stmt->node.binary.left);
		DataValue *rhs = execute(ctx, stmt->node.binary.right);

		if (strcmp(op, "+") == 0) {
			NumberNode *l_num = type_check("+", LHS, T_NUMBER, lhs);
			NumberNode *r_num = type_check("+", RHS, T_NUMBER, rhs);
			if (l_num == NULL || r_num == NULL)
				return NULL;
			// Finally, the addition is performed.
			data->type = T_NUMBER;
			data->value = num_add(*l_num, *r_num);
		} else if (strcmp(op, "-") == 0) {

		} else {
			ERROR_TYPE = EXECUTION_ERROR;
			sprintf(ERROR_MSG, "Do not know how to evaluate"
				" use of `%s' operator.", op);
			return NULL;
		}
		break;
	}
	default: {
		ERROR_TYPE = EXECUTION_ERROR;
		strcpy(ERROR_MSG,
			"Could not execute statement for unknown reason.");
		return NULL;
	}
	}
	return data;
}

DataValue *wrap_data(DataType type, void *value)
{
	DataValue *data = malloc(sizeof(DataValue));
	data->type = type;
	data->value = value;
	return data;
}

void *type_check(char *function_name, ParamPos pos,
	DataType type, DataValue *value)
{
	if (value->type == type) {
		return value->value;
	}
	ERROR_TYPE = TYPE_ERROR;
	sprintf(ERROR_MSG, "Wrong type for %s of `%s' operation,"
		" needed type of `%s'.",
		display_parampos(pos),
		function_name,
		display_datatype(type));
	return NULL;
}

Local *make_local(char *name, DataType type, void *value)
{
	Local *local = malloc(sizeof(Local));
	local->name = name;
	local->value.type = type;
	local->value.value = value;
	return local;
}

Context *init_context()
{
	Context *ctx = malloc(sizeof(Context));
	ctx->superior = NULL; // There is no context superior to this one.
	ctx->function = "<main>"; // Main function/scope.

	// Initialise with 16 free spaces for local variables.
	// This may have to be reallocated if more than 16
	// variables need to exist :^).
	ctx->locals_count = 1;
	ctx->locals_capacity = 16;
	ctx->locals = malloc(sizeof(Local) * ctx->locals_capacity);

	// Create an initial local varaible with the value of the
	// name of the function/scope.
	Local *scope_name = make_local(
		"__this_scope", T_STRING, ctx->function);
	ctx->locals = scope_name;
	// ^ Sets the first variable, default in every scope
	// (good for debuggin purposes).

	return ctx;
}

Context *make_context(char *scope_name, Context *super_scope)
{
	Context *ctx = init_context();
	ctx->function = scope_name;
	ctx->superior = super_scope;
	return ctx;
}
