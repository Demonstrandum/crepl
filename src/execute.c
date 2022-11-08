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

#define NUMERICAL_BINARY_OPERATION(DATA, OPERATION, LEFT, RIGHT) do { \
	NumberNode *l_num = type_check(op, LHS, T_NUMBER, (LEFT));  \
	NumberNode *r_num = type_check(op, RHS, T_NUMBER, (RIGHT)); \
	if (l_num == NULL || r_num == NULL)    \
		return NULL;                       \
	(DATA) = heap_data(T_NUMBER, num_ ##OPERATION (*l_num, *r_num)); \
} while (0);

inline
DataValue *link_datavalue(DataValue *data)
{
	++data->refcount;
	return data;
}

void unlink_datavalue(DataValue *data)
{
	if (--data->refcount == 0) free_datavalue(data);
}

inline
void free_datavalue(DataValue *data)
{
	if (debug)
		fprintf(stderr, "freeing data(stack: %d): %s    \033[2m(%p)\033[0m\n", data->onstack, display_datavalue(data), data->value);
	if (!data->onstack)
		free(data->value);
	free(data);  // data-wrapper itself is always malloc'd.
}

void free_context(Context *ctx)
{
	if (debug)
		fprintf(stderr, "destorying context: %s:\n", ctx->function);
	// If this context is getting free'd,
	// the superior context has one fewer references.
	if (ctx->superior != NULL)
		unlink_context(ctx->superior);
	// Unlink all references from locals.
	for (usize i = 0; i < ctx->locals_count; ++i) {
		if (debug)
			fprintf(stderr, "  unlinked local data (ref: %zu): %s    \033[2m(%p)\033[0m\n", ctx->locals[i].value->refcount - 1, display_datavalue(ctx->locals[i].value), ctx->locals[i].value->value);
		unlink_datavalue(ctx->locals[i].value);
	}
	// Free dynamic array of locals.
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
		bind_local(ctx, "Ans", data);
		bind_local(ctx, "ans", data);
		bind_local(ctx,   "_", data);
	}

	return data;
}

static DataValue *recursive_execute(Context *ctx, const ParseNode *stmt)
{
	DataValue *data = stack_data(T_NIL, NULL);
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
					free(data);
					data = link_datavalue(local->value);  // another reference.
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
		NumberNode *new_num = malloc(sizeof(NumberNode));
		memcpy(new_num, &stmt->node.number, sizeof(NumberNode));
		free(data);
		data = heap_data(T_NUMBER, new_num);
		break;
	}
	case STRING_NODE: {
		char *string = malloc(stmt->node.str.len + 1);
		memcpy(string, stmt->node.str.value, stmt->node.str.len + 1);
		free(data);
		data = heap_data(T_STRING, string);
		break;
	}
	case UNARY_NODE: { // Functions, essentially.
		DataValue *callee  = recursive_execute(ctx, stmt->node.unary.callee);
		DataValue *operand = recursive_execute(ctx, stmt->node.unary.operand);

		if (callee == NULL || operand == NULL) {
			if (callee != NULL) unlink_datavalue(callee);
			if (operand != NULL) unlink_datavalue(operand);
			free(data);
			return NULL;
		}

		// Juxtaposition of numbers, implies multiplication.
		if (callee->type == T_NUMBER && operand->type == T_NUMBER) {
			NumberNode *new_num = num_mul(*(NumberNode *)callee->value,
				*(NumberNode *)operand->value);
			free(data);
			data = heap_data(T_NUMBER, new_num);
			goto unary_discard;
		}

		// Otheriwse, we expect a lambda or function pointer as callee.
		void *func = type_check("function", ARG, T_LAMBDA | T_FUNCTION_PTR, callee);
		if (func == NULL) {
			free(data);
			data = NULL;
			goto unary_discard;
		}

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
			bind_local(local_ctx, lambda->args, operand);
			free(data);
			data = recursive_execute(local_ctx, lambda->body);
			// Temporary execution context spent.
			unlink_context(local_ctx);
		} else {
			fprintf(stderr, "prelimary type check failed to catch wrong function callee.\n");
			exit(2);
		}

unary_discard:
		// Operation operator and operand are discarded after result produced.
		unlink_datavalue(callee);
		unlink_datavalue(operand);
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
				if (data == NULL) return NULL;
				bind_local(ctx, lvalue, data);
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
			free(data);
			data = heap_data(T_LAMBDA, lambda);
			if (func_name[0] != '<')
				bind_local(ctx, func_name, data);
			break;
		}

		free(data);
		// How to evaluate specific operators.
		DataValue *lhs = recursive_execute(ctx, stmt->node.binary.left);
		if (lhs == NULL) {
			return NULL;
		}
		DataValue *rhs = recursive_execute(ctx, stmt->node.binary.right);
		if (rhs == NULL) {
			unlink_datavalue(lhs);
			return NULL;
		}

		// Numerical binary operations.
		if (strcmp(op, "+") == 0) {
			NUMERICAL_BINARY_OPERATION(data, add, lhs, rhs);
		} else if (strcmp(op, "-") == 0) {
			NUMERICAL_BINARY_OPERATION(data, sub, lhs, rhs);
		} else if (strcmp(op, "*") == 0) {
			NUMERICAL_BINARY_OPERATION(data, mul, lhs, rhs);
		} else if (strcmp(op, "/") == 0) {
			NUMERICAL_BINARY_OPERATION(data, div, lhs, rhs);
		} else if (strcmp(op, "^") * strcmp(op, "**") == 0) {
			NUMERICAL_BINARY_OPERATION(data, pow, lhs, rhs);
		} else {
			ERROR_TYPE = EXECUTION_ERROR;
			sprintf(ERROR_MSG, "Do not know how to evaluate"
				" use of `%s' operator.", op);
			data = NULL;
		}
		// Operation operands are discarded.
		unlink_datavalue(lhs);
		unlink_datavalue(rhs);
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

DataValue *wrap_data(DataType type, void *value, bool onstack)
{
	DataValue *data = malloc(sizeof(DataValue));
	data->refcount = 1;
	data->type = type;
	data->value = value;
	data->onstack = onstack;
	return data;
}

DataValue *stack_data(DataType type, void *value)
{ return wrap_data(type, value, true); }

DataValue *heap_data(DataType type, void *value)
{ return wrap_data(type, value, false); }

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

Local make_local(const char *name, DataValue *data)
{
	return (Local){
		.name = strdup(name),
		.value = link_datavalue(data),
	};
}


// Locals is a dynamically growable array.
void bind_local(Context *ctx, const char *name, DataValue *data)
{
	// Check if it already exists.
	Local *local_ptr = NULL;
	for (usize i = 0; i < ctx->locals_count; ++i) {
		Local *l = ctx->locals + i;
		if (strcmp(l->name, name) == 0) {
			local_ptr = l;
			break;
		}
	}
	// Reassignment: slot already exists.
	if (local_ptr != NULL) {
		unlink_datavalue(local_ptr->value);  // one fewer things pointing to old allocated data.
		local_ptr->value = link_datavalue(data);  // now points to new data.
		return;
	}

	// Check capacity.
	if (ctx->locals_count == ctx->locals_capacity) {
		// Grow array.
		ctx->locals_capacity *= LOCALS_REALLOC_GROWTH_FACTOR;
		ctx->locals = realloc(ctx->locals,
			sizeof(Local) * ctx->locals_capacity);
	}

	Local local = make_local(name, data);
	ctx->locals[ctx->locals_count++] = local;
}

void bind_builtin_functions(Context *ctx)
{
	for (usize i = 0; i < len(builtin_fns); ++i) {
		struct _func_name_pair *pair =
			(struct _func_name_pair *)(builtin_fns + i);
		bind_local(ctx, pair->name, stack_data(T_FUNCTION_PTR, &pair->function));
	}
}

void bind_default_globals(Context *ctx)
{
	fsize pi = M_PI;
	fsize e = M_E;
	fsize inf = HUGE_VAL;
	fsize nan = NAN;

	bind_local(ctx, "nil", stack_data(T_NIL, NULL));
	bind_local(ctx, "pi",  heap_data(T_NUMBER, make_number(FLOAT, &pi)));
	bind_local(ctx, "e",   heap_data(T_NUMBER, make_number(FLOAT, &e)));
	bind_local(ctx, "inf", heap_data(T_NUMBER, make_number(FLOAT, &inf)));
	bind_local(ctx, "nan", heap_data(T_NUMBER, make_number(FLOAT, &nan)));
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
	Local this_scope = make_local("__this_scope",
		stack_data(T_STRING, (void *)ctx->function));
	Local ans = make_local("Ans",
		heap_data(T_NUMBER, make_number(FLOAT, (fsize *)&ZERO)));
	ctx->locals[0] = this_scope;
	ctx->locals[1] = ans;
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
