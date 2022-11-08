#pragma once

#include "defaults.h"
#include "parse.h"

/// Execution context / scope.
struct _context;

typedef enum {
	T_NIL     = 1 << 0,  // Empty type.
	T_NUMBER  = 1 << 1,  // NumberNode (ParsNode).
	T_STRING  = 1 << 2,  // Native char pointer.
	T_TUPLE   = 1 << 3,  // List of contigious data values.
	T_LAMBDA  = 1 << 4,  // User defined function.
	T_FUNCTION_PTR = 1 << 5,  // Wrapper of native function pointer.
} DataType;

typedef struct {
	usize refcount;
	bool onstack;
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
	const ParseNode *body;
	usize arg_count;
	char *args;  // NUL-delimited.
	struct _context *scope;  // Scope the function was defined in.
} Lambda;

#define FUNC_PTR(FUNC_NAME) \
	DataValue *(*FUNC_NAME)(DataValue)

typedef struct {
	FUNC_PTR(fn);
} FnPtr;

typedef struct {
	const char *name;
	DataValue *value;
} Local;

typedef struct _context {
	usize refcount;
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
DataValue *link_datavalue(DataValue *);
void unlink_datavalue(DataValue *);
void free_context(Context *);
Context *link_context(Context *);
void unlink_context(Context *);
Lambda *make_lambda(const char *, usize, const char *, Context *, const ParseNode *);
void *type_check(const char *, ParamPos, DataType, const DataValue *);
DataValue *execute(Context *, const ParseNode *);
DataValue *wrap_data(DataType, void *, bool);
DataValue *stack_data(DataType, void *);
DataValue *heap_data(DataType, void *);
Local make_local(const char *, DataValue *);
void bind_local(Context *, const char *, DataValue *);
void bind_builtin_functions(Context *);
Context *init_context();
Context *base_context();
Context *make_context(const char *, Context *);
