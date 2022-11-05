#include "execute.h"
#include "error.h"
#include "parse.h"
#include "builtin.h"
#include "prelude.h"
#include "displays.h"

#include <math.h>

static const f32 LOCALS_REALLOC_GROWTH_FACTOR = 1.5;
static const fsize ZERO = 0.0;

static const DataValue nil = { .type = T_NIL, .value = NULL };

#define NUMERICAL_BINARY_OPERATION(OPERATION) do { \
	NumberNode *l_num = type_check(op, LHS, T_NUMBER, lhs); \
	NumberNode *r_num = type_check(op, RHS, T_NUMBER, rhs); \
	if (l_num == NULL || r_num == NULL)    \
		return NULL;                       \
	data->type = T_NUMBER;                 \
	data->value = num_ ##OPERATION (*l_num, *r_num); \
} while (0);

inline
void free_datavalue(DataValue *data)
{
	free(data->value);
	free(data);
}

static
void free_local(Local local)
{
	free(local.value.value);
}

static
void unlink_local(Local *local)
{
	if (--local->value.refcount == 0) free_local(*local);
}

DataValue *link_datavalue(DataValue *data)
{
	++data->refcount;
	return data;
}

void unlink_datavalue(DataValue *data)
{
	if (--data->refcount == 0) free_datavalue(data);
}

void free_context(Context *ctx)
{
	// If this context is getting free'd,
	// the superior context has one fewer references.
	if (ctx->superior != NULL)
		unlink_context(ctx->superior);
	// Unlink all references to locals.
	for (usize i = 0; i < ctx->locals_count; ++i)
		unlink_local(&ctx->locals[i]);
	free(ctx->locals);
	free(ctx);
}

inline
Context *link_context(Context *ctx)
{
	++ctx->refcount;
	return ctx;
}

/// Attempt to free context when no longer needed.
/// i.e. decrements reference count.
void unlink_context(Context *ctx)
{
	if (--ctx->refcount == 0) free_context(ctx);
}

static DataValue *recursive_execute(Context *ctx, const ParseNode *stmt);

/// Takes in an execution context (ctx) and a
/// statement as produced by the parser (stmt).
/// Returns what it evaluates to.
DataValue *execute(Context *ctx, const ParseNode *stmt)
{
	// Recurse dowm parse tree, execute each node, bottom up.
	DataValue *data = recursive_execute(ctx, stmt);

	// When line/statement is finished evaluating, bind `Ans'.
	if (data != NULL && ERROR_TYPE == NO_ERROR) {
		bind_data(ctx, "Ans", data);
		bind_data(ctx, "ans", data);
		bind_data(ctx,   "_", data);
	}

	return data;
}

static DataValue *recursive_execute(Context *ctx, const ParseNode *stmt)
{
	DataValue *data = malloc(sizeof(DataValue));
	data->refcount = 1;
	switch (stmt->type) {
	case IDENT_NODE: {
		// Resolve variables by searching through local
		// variables. If that fails, search through the
		// superior sopes varaibles, repeat until there
		// are no more superior scopes, if the local is
		// found yield its corresponding value, or else
		// throw an execution error.
		char *ident_name = stmt->node.ident.value;

		Context *current_ctx = ctx;
		while (current_ctx != NULL) {
			for (usize i = 0; i < current_ctx->locals_count; ++i) {
				Local *local = &current_ctx->locals[i];
				if (strcmp(local->name, ident_name) == 0) {
					*data = local->value;
					goto finished_search;
				}
			}
			current_ctx = current_ctx->superior;
		}

		ERROR_TYPE = EXECUTION_ERROR;
		sprintf(ERROR_MSG, "Could not find variable `%s'\n"
			"  in any local or superior scope.", ident_name);
		return NULL;

finished_search:
		break;
	}
	case NUMBER_NODE: {
		data->type = T_NUMBER;
		data->value = malloc(sizeof(NumberNode));
		memcpy(data->value, &stmt->node.number, sizeof(NumberNode));
		break;
	}
	case STRING_NODE: {
		data->type = T_STRING;
		data->value = malloc(stmt->node.str.len + 1);
		memcpy(data->value, stmt->node.str.value, stmt->node.str.len + 1);
		break;
	}
	case UNARY_NODE: { // Functions, essentially.
		DataValue *callee  = recursive_execute(ctx, stmt->node.unary.callee);
		DataValue *operand = recursive_execute(ctx, stmt->node.unary.operand);

		if (callee == NULL || operand == NULL)
			return NULL;

		// Juxtaposition of numbers, implies multiplication.
		if (callee->type == T_NUMBER && operand->type == T_NUMBER) {
			data->type = T_NUMBER;
			data->value = num_mul(*(NumberNode *)callee->value,
				*(NumberNode *)operand->value);
			break;
		}

		// Otheriwse, we expect a lambda or function pointer as callee.
		void *func = type_check("function", ARG, T_LAMBDA | T_FUNCTION_PTR, callee);
		if (func == NULL)
			return NULL;

		if (callee->type == T_FUNCTION_PTR) {
			FUNC_PTR(fn) = ((FnPtr *)func)->fn;
			free(data);
			data = fn(*operand);
		} else if (callee->type == T_LAMBDA) {
			Lambda *lambda = func;
			// Make the function call frame / local execution context.
			Context *local_ctx = make_context(lambda->name, lambda->scope);
			// Bind the arguments to the local context.
			// TODO: Make this a loop over the tuple if a tuple were provided.
			bind_data(local_ctx, lambda->args, operand);
			data = recursive_execute(local_ctx, lambda->body);
			// Temporary execution context spent.
			unlink_context(local_ctx);
		} else {
			fprintf(stderr, "prelimary type check failed to catch wrong function callee.\n");
			exit(2);
		}

		break;
	}
	case BINARY_NODE: {
		IdentNode ident = stmt->node.binary.callee->node.ident;
		if (stmt->node.binary.callee->type != IDENT_NODE) {
			ERROR_TYPE = EXECUTION_ERROR;
			strcpy(ERROR_MSG, "Binary operation has non-ident callee.");
			return NULL;
		}

		// Variables in case of function definition.
		char *func_name = NULL;
		const ParseNode *func_operand = NULL;
		const ParseNode *func_body = NULL;

		char *op = ident.value;
		// Equality is special:
		if (strcmp(op, "=") == 0) {
			if (stmt->node.binary.left->type == IDENT_NODE) {
				char *lvalue = stmt->node.binary.left->node.ident.value;
				free(data);
				data = recursive_execute(ctx, stmt->node.binary.right);
				bind_local(ctx, lvalue, data->type, data->value);
				break;
			} else if (stmt->node.binary.left->type == UNARY_NODE) {
				UnaryNode func = stmt->node.binary.left->node.unary;
				func_body = stmt->node.binary.right;
				if (func.callee->type != IDENT_NODE) {
					ERROR_TYPE = PARSE_ERROR;
					strcpy(ERROR_MSG, "Function assignment must have"
						" identifier as function name.");
					return NULL;
				}
				func_name = func.callee->node.ident.value;
				func_operand = func.operand;
			} else {
				ERROR_TYPE = PARSE_ERROR;
				strcpy(ERROR_MSG, "Left of assignment (`=') operator\n"
					"  must be an identifier or function application.");
				return NULL;
			}
		} else if (strcmp(op, "->") == 0) {  // Lambda expression.
			func_name = "<anon>";
			func_operand = stmt->node.binary.left;
			func_body = stmt->node.binary.right;
		}

		// Defines and binds a function.
		if (func_body != NULL) {
			// TODO: support tuples.
			char *arg_names = strdup(func_operand->node.ident.value);
			if (func_operand->type != IDENT_NODE) {
				ERROR_TYPE = PARSE_ERROR;
				strcpy(ERROR_MSG, "Function argument must be"
					" single identifier or tuple of arguments.");
				return NULL;
			}
			// TODO: handel NUL-delimted arg_names from tuple argument.
			const ParseNode *body = clone_node(func_body);
			Lambda *lambda = make_lambda(func_name, 1, arg_names, ctx, body);
			if (func_name[0] != '<')
				bind_local(ctx, func_name, T_LAMBDA, lambda);
			data->type = T_LAMBDA;
			data->value = lambda;
			break;
		}

		// How to evaluate specific operators.
		DataValue *lhs = recursive_execute(ctx, stmt->node.binary.left);
		if (lhs == NULL)
			return NULL;
		DataValue *rhs = recursive_execute(ctx, stmt->node.binary.right);
		if (rhs == NULL)
			return NULL;

		// Numerical binary operations.
		if (strcmp(op, "+") == 0) {
			NUMERICAL_BINARY_OPERATION(add);
		} else if (strcmp(op, "-") == 0) {
			NUMERICAL_BINARY_OPERATION(sub);
		} else if (strcmp(op, "*") == 0) {
			NUMERICAL_BINARY_OPERATION(mul);
		} else if (strcmp(op, "/") == 0) {
			NUMERICAL_BINARY_OPERATION(div);
		} else if (strcmp(op, "^") * strcmp(op, "**") == 0) {
			NUMERICAL_BINARY_OPERATION(pow);
		} else {
			ERROR_TYPE = EXECUTION_ERROR;
			sprintf(ERROR_MSG, "Do not know how to evaluate"
				" use of `%s' operator.", op);
			return NULL;
		}
		break;
	}
	default: {
		fprintf(stderr, "unknown node type / unexpected node: %s\n", display_parsetree(stmt));
		ERROR_TYPE = EXECUTION_ERROR;
		strcpy(ERROR_MSG,
			"Could not execute statement for unknown reason.");
		return NULL;
	}
	}

	return data;
}

Lambda *make_lambda(const char *name, usize arg_count, const char *args, Context *ctx, const ParseNode *body)
{
	Lambda *lambda = malloc(sizeof(Lambda));
	lambda->name = strdup(name);
	lambda->arg_count = arg_count;
	lambda->args = (char *)args;
	lambda->body = body;
	lambda->scope = link_context(ctx);
	return lambda;
}

DataValue *wrap_data(DataType type, void *value)
{
	DataValue *data = malloc(sizeof(DataValue));
	data->refcount = 1;
	data->type = type;
	data->value = value;
	return data;
}

void *type_check(const char *function_name, ParamPos pos,
	DataType type, const DataValue *value)
{
	if (value != NULL
	&& value->value != NULL
	&& (value->type & type) != 0)
		return (void *)value->value;

	ERROR_TYPE = TYPE_ERROR;
	sprintf(ERROR_MSG, "Wrong type for %s of `%s' operation,\n"
		"  expected type of `%s', got type of `%s'.",
		display_parampos(pos),
		function_name,
		display_datatype(type),
		value == NULL
			? "null-pointer"
			: display_datatype(value->type));
	return NULL;
}

Local *make_local(const char *name, DataType type, void *value)
{
	Local *local = malloc(sizeof(Local));
	local->name = strdup(name);
	local->value.refcount = 1;
	local->value.type = type;
	local->value.value = value;
	return local;
}


// Same as `bind_local`, but takes a pre-wrapped `DataValue`.
void bind_data(Context *ctx, const char *name, DataValue *data)
{
	data = link_datavalue(data);  // Taking reference to the data.
	bind_local(ctx, name, data->type, data->value);
}

// Locals is a dynamically growable array.
void bind_local(Context *ctx, const char *name, DataType type, void *value)
{
	// Check capacity.
	if (ctx->locals_count == ctx->locals_capacity) {
		// Grow array.
		ctx->locals_capacity *= LOCALS_REALLOC_GROWTH_FACTOR;
		ctx->locals = realloc(ctx->locals,
			sizeof(Local) * ctx->locals_capacity);
	}

	// Check if it already exists.
	Local *local_ptr = NULL;
	for (usize i = 0; i < ctx->locals_count; ++i) {
		Local *l = ctx->locals + i;
		if (strcmp(l->name, name) == 0) {
			local_ptr = l;
			break;
		}
	}

	Local *local = make_local(name, type, value);

	if (local_ptr != NULL) {
		*local_ptr = *local;
		return;
	}

	ctx->locals[ctx->locals_count++] = *local;
}

void bind_builtin_functions(Context *ctx)
{
	for (usize i = 0; i < len(builtin_fns); ++i) {
		struct _func_name_pair *pair =
			(struct _func_name_pair *)(builtin_fns + i);
		bind_local(ctx, pair->name, T_FUNCTION_PTR, &pair->function);
	}
}

void bind_default_globals(Context *ctx)
{
	fsize pi = M_PI;
	fsize e = M_E;
	fsize inf = HUGE_VAL;
	fsize nan = NAN;

	bind_local(ctx, "nil", T_NIL, NULL);
	bind_local(ctx, "pi", T_NUMBER, make_number(FLOAT, &pi));
	bind_local(ctx, "e", T_NUMBER, make_number(FLOAT, &e));
	bind_local(ctx, "inf", T_NUMBER, make_number(FLOAT, &inf));
	bind_local(ctx, "nan", T_NUMBER, make_number(FLOAT, &nan));
}

Context *make_context(const char *scope_name, Context *super_scope)
{
	Context *ctx = malloc(sizeof(Context));
	ctx->refcount = 1;
	ctx->function = scope_name;
	ctx->superior = super_scope;
	if (ctx->superior != NULL)  // Increment reference count to superior scope.
		++ctx->superior->refcount;

	// Initialise with 6 free spaces for local variables.
	// This may have to be reallocated if more than 6
	// variables need to exist :^).
	ctx->locals_count = 1;
	ctx->locals_capacity = 6;
	ctx->locals = malloc(sizeof(Local) * ctx->locals_capacity);

	// Create an initial local varaible with the value of the
	// name of the function/scope.
	Local *this_scope = make_local("__this_scope",
		T_STRING, (void *)ctx->function);
	Local *ans = make_local("Ans",
		T_NUMBER, (void *)make_number(FLOAT, (fsize *)&ZERO));
	ctx->locals[0] = *this_scope;
	ctx->locals[1] = *ans;
	// ^ Sets the first variable, default in every scope
	// (good for debuggin purposes).

	return ctx;
}

Context *base_context()
{
	Context *ctx = init_context();
	bind_default_globals(ctx); // Global variables.
	bind_builtin_functions(ctx); // Interface with certain C functions.
	// Load the "prelude" library, runs at start of every base context:
	execute_prelude(ctx);
	return ctx;
}

// Create main parent context.
Context *init_context()
{
	return make_context("<main>", NULL);
}
