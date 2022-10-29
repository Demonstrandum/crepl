#pragma once

#include "defaults.h"
#include "parse.h"

typedef enum {
	T_NIL     = 1 << 0,  // Empty type.
	T_NUMBER  = 1 << 1,  // NumberNode (ParsNode).
	T_STRING  = 1 << 2,  // Native char pointer.
	T_TUPLE   = 1 << 3,  // List of contigious data values.
	T_LAMBDA  = 1 << 4,  // User defined function.
	T_FUNCTION_PTR = 1 << 5,  // Wrapper of native function pointer.
} DataType;

typedef struct {
	DataType type;
	void *value;
} DataValue;

// (a , (b , c)) == (a, b, c), so (,) is a cons operator.
typedef struct {
	usize length;
	usize capacity;
	DataValue *items;
} Tuple;

typedef struct {
	char *name;
	ParseNode *body;
	usize arg_count;
	char *args;  // NUL-delimited.
} Lambda;

#define FUNC_PTR(FUNC_NAME) \
	DataValue *(*FUNC_NAME)(DataValue)

typedef struct {
	FUNC_PTR(fn);
} FnPtr;

typedef struct {
	const char *name;
	DataValue value;
} Local;

struct _context;

typedef struct _context {
	struct _context *superior;
	const char *function;
	// `locals` works as a dynamic array;
	usize locals_count;
	usize locals_capacity;
	Local *locals;
} Context;

typedef enum {
	ARG, LHS, RHS
} ParamPos;

void free_datavalue(DataValue *);
Lambda *make_lambda(const char *, usize, const char *, ParseNode *);
void *type_check(const char *, ParamPos, DataType, const DataValue *);
DataValue *execute(Context *, const ParseNode *);
DataValue *wrap_data(DataType, void *);
Local *make_local(const char *, DataType, void *);
void bind_local(Context *, const char *, DataType, void *);
void bind_data(Context *, const char *, DataValue *);
void bind_builtin_functions(Context *);
Context *init_context();
Context *base_context();
Context *make_context(const char *, Context *);
