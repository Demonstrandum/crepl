#pragma once

#include "prelude.h"
#include "parse.h"

typedef enum {
	T_NUMBER,  // NumberNode (ParsNode).
	T_STRING,  // (Native?) char pointer.
	T_FUNCTION_PTR,  // Native function pointer.
} DataType;

typedef struct {
	DataType type;
	void *value;
} DataValue;

typedef struct {
	char *name;
	DataValue value;
} Local;

struct _context;

typedef struct _context {
	struct _context *superior;
	char *function;
	// `locals` works as a dynamic array;
	usize locals_count;
	usize locals_capacity;
	Local *locals;
} Context;

typedef enum {
	ARG, LHS, RHS
} ParamPos;

void free_datavalue(DataValue *);
void *type_check(char *, ParamPos, DataType, DataValue *);
DataValue *execute(Context *, const ParseNode *);
Local *make_local(char *, DataType, void *);
Context *init_context();
Context *make_context(char *, Context *);
