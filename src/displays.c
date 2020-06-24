#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prelude.h"
#include "parse.h"
#include "execute.h"
#include "displays.h"

char *display_parampos(ParamPos pos)
{
	switch (pos) {
	case LHS:
		return "left-hand-side";
	case RHS:
		return "right-hand-side";
	default:
		return "argument";
	}
}

char *display_datatype(DataType type)
{
	switch (type) {
	case T_NUMBER:
		return "number";
	case T_FUNCTION_PTR:
		return "function";
	case T_STRING:
		return "text-string";
	default:
		return "unknown-type";
	}
}

char *display_parsetree(const ParseNode *tree)
{
	if (tree == NULL)
		return "NULL";
	switch (tree->type) {
	case IDENT_NODE: {
		return tree->node.ident.value;
	}
	case NUMBER_NODE: {
		// TODO: Check the number type (int, float, etc.).
		char *number = malloc(sizeof(char) * 64); // Guess?
		sprintf(number, "%ld", tree->node.number.value.i);
		return number;
	}
	case UNARY_NODE: {
		UnaryNode unary = tree->node.unary;
		char *operand_str = display_parsetree(unary.operand);
		char *callee_str  = display_parsetree(unary.callee);

		char *unary_str = malloc(sizeof(char) * (
			+ strlen(operand_str)
			+ strlen(callee_str)
			+ 4 /* <- Extra padding */));
		if (unary.is_postfix)
			sprintf(unary_str, "(%s %s)", operand_str, callee_str);
		else
			sprintf(unary_str, "(%s %s)", callee_str, operand_str);
		return unary_str;
	}
	case BINARY_NODE: {
		BinaryNode binary = tree->node.binary;
		char *left_str   = display_parsetree(binary.left);
		char *right_str  = display_parsetree(binary.right);
		char *callee_str = display_parsetree(binary.callee);

		char *binary_str = malloc(sizeof(char) * (
			+ strlen(left_str)
			+ strlen(right_str)
			+ strlen(callee_str)
			+ 6 /* <- Extra padding */));
		sprintf(binary_str, "(%s %s %s)", left_str, callee_str, right_str);
		return binary_str;
	}
	default:
		return "[Unknown Parse Node]";
	}
}

char *display_datavalue(const DataValue *data)
{
	// Safe bet.
	char *string = malloc(sizeof(char) * 512);

	switch (data->type) {
	case T_NUMBER: {
		NumberNode *num = data->value;
		// TODO: Handle more than just INT type.
		sprintf(string, "%ld", num->value.i);
		break;
	}
	default:
		sprintf(string, "<%s at 0x%p>",
				display_datatype(data->type),
				data->value);
	}
	return string;
}
