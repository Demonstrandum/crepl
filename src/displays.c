#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "parse.h"
#include "execute.h"
#include "displays.h"

char *display_nil(void)
{
	return "nil";
}

char *display_lambda(Lambda *lambda)
{
	char *str = calloc(128, sizeof(char));
	char *ptr = str;
	ptr += sprintf(ptr, "<lambda %s of (", lambda->name);
	char *arg_name = lambda->args;
	for (usize i = 0; i < lambda->arg_count; ++i) {
		usize offs = sprintf(ptr, "%s", arg_name);
		arg_name += offs + 1;
		ptr += offs;
		ptr += sprintf(ptr, ", ");
	}
	ptr -= 2;  // remove trailing comma and space.
	ptr += sprintf(ptr, ") at %p>", (void *)lambda);
	return str;
}

char *display_numbernode(NumberNode num)
{
	char *str = malloc(sizeof(char) * 128); // Hope that's enough.
	switch (num.type) {
	case INT:
		sprintf(str, "%ld", num.value.i);
		break;
	case FLOAT:
		sprintf(str, "%.15LG", num.value.f);
		break;
	default:
		strcpy(str, "undisplayable-number-type");
	}
	return str;
}

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
	static char unknown[] = "unknown-type-X";
	switch (type) {
	case T_NIL:
		return "nil";
	case T_NUMBER:
		return "number";
	case T_LAMBDA:
		return "lambda";
	case T_TUPLE:
		return "tuple";
	case T_FUNCTION_PTR:
		return "function";
	case T_STRING:
		return "text-string";
	default:
		unknown[sizeof(unknown) - 2] = '0' + (char)type;
		return unknown;
	}
}

char *display_parsetree(const ParseNode *tree)
{
	static char unknown[256] = { '\0' };
	if (tree == NULL)
		return "NULL";
	switch (tree->type) {
	case IDENT_NODE: {
		return tree->node.ident.value;
	}
	case NUMBER_NODE: {
		return display_numbernode(tree->node.number);
	}
	case STRING_NODE: {  // TODO: Escape the string.
		usize l = strlen(tree->node.str.value);
		byte *str = malloc(l + 2);
		str[0] = '"';
		strcpy(str + 1, tree->node.str.value);
		str[l + 1] = '"';
		str[l + 2] = '\0';
		return str;
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
		sprintf(unknown, "[unknown-parse-node: %d]", tree->type);
		return unknown;
	}
}

char *display_datavalue(const DataValue *data)
{
	char *string;

	if (data == NULL)
		return "internal-null-pointer";

	switch (data->type) {
	case T_NIL: {
		return display_nil();
	}
	case T_NUMBER: {
		if (data->value == NULL)
			return "number-with-null-value";
		NumberNode *num = data->value;
		return display_numbernode(*num);
	}
	case T_STRING: {
		char *inside = data->value;
		usize len = strlen(inside);
		string = malloc(len + 2);
		strcpy(string + 1, inside);
		string[0] = '"';
		string[len + 1] = '"';
		string[len + 2] = '\0';
		break;
	}
	case T_LAMBDA: {
		return display_lambda(data->value);
	}
	default:
		string = malloc(sizeof(char) * 128); // Safe bet.
		sprintf(string, "<%s at %p>",
				display_datatype(data->type),
				data->value);
	}
	return string;
}
