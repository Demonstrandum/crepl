#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "displays.h"
#include "parse.h"

void free_token(Token *token)
{
	free((char *)token->value);
	free(token);
}

void free_parsenode(ParseNode *node)
{
	switch (node->type) {
	case IDENT_NODE:
		free((char *)node->node.ident.value);
		break;
	case STRING_NODE:
		free((char *)node->node.str.value);
		break;
	case NUMBER_NODE:
		break;
	case UNARY_NODE:
		free_parsenode((ParseNode *)node->node.unary.callee);
		free_parsenode((ParseNode *)node->node.unary.operand);
		break;
	case BINARY_NODE:
		free_parsenode((ParseNode *)node->node.binary.callee);
		free_parsenode((ParseNode *)node->node.binary.left);
		free_parsenode((ParseNode *)node->node.binary.right);
		break;
	default:
		fprintf(stderr, "unhandled node: %d\n", node->type);
		exit(2);
	}
	free(node);
}

ParseNode *clone_node(const ParseNode *node)
{
	ParseNode *new = malloc(sizeof(ParseNode));
	*new = *node;
	switch (node->type) {
	case IDENT_NODE:
		new->node.ident.value = strdup(node->node.ident.value);
		break;
	case STRING_NODE:
		new->node.str.value = strdup(node->node.str.value);
		break;
	case NUMBER_NODE:
		break;
	case UNARY_NODE:
		new->node.unary.callee = clone_node(node->node.unary.callee);
		new->node.unary.operand = clone_node(node->node.unary.operand);
		break;
	case BINARY_NODE:
		new->node.binary.callee = clone_node(node->node.binary.callee);
		new->node.binary.left = clone_node(node->node.binary.left);
		new->node.binary.right = clone_node(node->node.binary.right);
		break;
	default:
		fprintf(stderr, "unhandled node: %d\n", node->type);
		exit(2);
	}
	return new;
}

/* --- Functions related to tokenising/lexing --- */

Token *new_token(TokenType type, const char *value)
{
	Token *t = malloc(sizeof(Token));
	t->type  = type;
	t->value = strdup(value);
	return t;
}

TokenType char_token_type(char c, char last_char, TokenType last_token_type)
{
	if (c <= '9' && c >= '0') {
		if (last_token_type == TT_IDENTIFIER)
			return TT_IDENTIFIER;
		return TT_NUMERIC;
	}
	if (c == '_'
	&& (last_token_type == TT_IDENTIFIER
	|| last_token_type == TT_NUMERIC))
		return last_token_type;
	if (c == '.'
	&& (last_token_type == TT_OPERATOR
	|| last_token_type == TT_NONE))
		return TT_NUMERIC;
	if ((c == '.' || c == 'E' || c == 'P')  // Scientific notation.
	&& last_token_type == TT_NUMERIC)
		return TT_NUMERIC;
	if ((c == '+' || c == '-') && last_char == 'E')
		return TT_NUMERIC;
	if ((c == 'x' || c == 'X' || c == 'o' || c == 'O') && last_char == '0')
		return TT_NUMERIC;
	if (c == '(')
		return TT_LPAREN;
	if (c == ')')
		return TT_RPAREN;
	if (c == '\0' || c == ' ' || c == '\t' || c == '\n' || c == '\r')
		return TT_NONE;
	if (c == '"')
		return TT_STRING;

	// All possible operator/special-symbol characters:
	if (((c >= '!' && c <= '/')
	 || (c >= ':' && c <= '@')
	 || (c >= '{' && c <= '~')
	 || (c >= '[' && c <= '`'))
	&& c != '_') return TT_OPERATOR;

	// Anything else is treated as an identifier.
	return TT_IDENTIFIER;
}

/// Returns a token matching in the source, and increments the
/// pointer to point after the match.  This function is to be
/// applied repeatedly until the source is empty (NULL).
/// NOTE: Return of NULL means EOF or an Error was found.
/// NOTE: `source' is a pointer to a pointer, so that the
///       character `source' is pointing to can be offset.
Token *lex(char **source)
{
	if (**source == '\0')
		return NULL;  // No more tokens.

	TokenType tt = char_token_type(**source, ' ', TT_NONE);

	// Skip over TT_NONE tokens (spaces, tabs, etc.).
	while (tt == TT_NONE) {
		(*source)++;
		if (**source == '\0')
			return NULL;
		tt = char_token_type(**source, *(*source - 1), tt);
	}

	// First of all, check if it matches an operator.
	for (usize i = 0; i < len(KNOWN_OPERATORS); ++i) {
		const char *operator = KNOWN_OPERATORS[i].value;
		usize operator_len = strlen(operator);

		if (strncmp(*source, operator, operator_len) == 0) {
			Token *token = new_token(TT_OPERATOR, operator);
			*source += operator_len;
			return token;
		}
	}

	// If we match an operator-type character, and it wasn't
	// found in our for-loop, it means it is an unknown operator,
	// and we should report it as an error.
	if (tt == TT_OPERATOR) {
		ERROR_TYPE = SYNTAX_ERROR;
		sprintf(ERROR_MSG, "Operator `%c' does not exist.", **source);
		return NULL;
	}

	// Now, we check the type of the current character,
	// then we check the next character, if the type is the same
	// we continue collecting the characters until the type is no longer
	// the same, and we return the token.

	// That's to say, characters of the same `kind' coalesce.
	TokenType previous_tt = tt;
	usize span = 0;

	// Do not coalesce parentheses.
	if (tt == TT_RPAREN || tt == TT_LPAREN) {
		span++;
	} else if (tt == TT_STRING) {  // String literals are not like others.
		++span;  // Skip opening quote.
		while ((*source)[span] != '"') {
			if ((*source)[span] == '\\' && (*source)[span + 1] == '"')
				++span;  // Don't stop at escaped quote.
			++span;
		}
		++span;  // Skip ending quote.
	} else {
		while (tt == previous_tt) {
			span++;
			previous_tt = char_token_type(
				*(*source + span),
				*(*source + span - (span == 0 ? 0 : 1)),
				previous_tt);
		}
	}

	char *sub_str = malloc(span * sizeof(char));
	strncpy(sub_str, *source, span);
	sub_str[span] = '\0';

	*source += span;
	Token *token = new_token(tt, sub_str);

	free(sub_str);
	return token;
}

Token *peek(char **source)
{
	char *original = *source;
	Token *token = lex(source);
	*source = original;
	return token;
}

/* --- Functions related to parsing into a tree --- */

void node_into_ident(const char *str, ParseNode *node)
{
	IdentNode *ident = malloc(sizeof(IdentNode));
	ident->value = strdup(str);
	node->type = IDENT_NODE;
	node->node.ident = *ident;
}

NumberNode *make_number(NumberType type, void *val)
{
	NumberNode *num = malloc(sizeof(NumberNode));
	num->type = type;
	num->value.i = -1;
	switch (type) {
	case INT: {
		num->value.i = *(ssize *)val;
		break;
	}
	case FLOAT: {
		num->value.f = *((fsize *)val);
		break;
	}
	default:
		fprintf(stderr, "Unhandled number type: %d\n", type);
		abort();
	}
	return num;
}

// Parse number literals:
// e.g. 3, 8.2, 2E32, 3E+4, 1.6E-19, 0b010110, 0xff32a1, 0o0774, etc.
// TODO: Parse binary, hexadecimal and octal literals (0b, 0x, 0o)
//       as well as hex/binary `P' power notation..
NumberNode *parse_number(const char *str)
{
	NumberNode *number = malloc(sizeof(NumberNode));

	str = remove_all_bytes(str, '_');

	char *exponent_ptr = strstr(str, "E");
	char *neg_exponent_ptr = strstr(str, "E-");
	char *decimal_point_ptr = strstr(str, ".");

	// Sanity.
	if (exponent_ptr != NULL) {
		// No trailing 'E'.
		if (*(exponent_ptr + 1) == '\0')
			return NULL;
		// No trailing 'E+' or 'E-'.
		if ((*(exponent_ptr + 1) == '+'
		||   *(exponent_ptr + 1) == '-')
		&&   *(exponent_ptr + 2) == '\0')
			return NULL;
		// No repreated 'E' and no decimal point ('.') after 'E'.
		if (strstr(exponent_ptr + 1, "E") != NULL
		||  strstr(exponent_ptr + 1, ".") != NULL)
			return NULL;
	}
	if (decimal_point_ptr != NULL) {
		// No trailing decimal point ('.').
		if (*(decimal_point_ptr + 1) == '\0')
			return NULL;
		// No decimal point ('.') after first decimal point.
		if (strstr(decimal_point_ptr + 1, ".") != NULL)
			return NULL;
	}

	// No negative exponent and no decimal point, means
	// the number literal is certainly an integer.
	if (neg_exponent_ptr == NULL && decimal_point_ptr == NULL) {
		number->type = INT;
		ssize significand = strtoll(str, NULL, 0);
		if (exponent_ptr == NULL) { // No power-term.
			number->value.i = significand;
			return number;
		}

		usize exponent = strtoull(exponent_ptr + 1, NULL, 10);
		ssize power_term = ipow(10, exponent);
		if (power_term >= 0 && exponent <= 18) {
			// Probably didn't overflow.
			number->value.i = significand * power_term;
			return number;
		}
		// Fallback to float.
	}

	number->type = FLOAT;
	number->value.f = strtold(str, NULL);

	return number;
}

ParseNode *parse_prefix(const Token *token, char **rest)
{
	ParseNode *node = malloc(sizeof(ParseNode));

	switch (token->type) {
	case TT_NUMERIC: {
		NumberNode *num = parse_number(token->value);
		if (num == NULL) {
			ERROR_TYPE = SYNTAX_ERROR;
			sprintf(ERROR_MSG, "Malformed number literal (`%s').",
				token->value);
			return NULL;
		}

		node->type = NUMBER_NODE;
		node->node.number = *num;
		break;
	}
	case TT_IDENTIFIER: {
		// Parse `let`-`in` expressions.
		if (strcmp(token->value, "let") == 0) {
			ParseNode *bindings = parse_expr(rest, min_prec);
			if (bindings == NULL) {
				ERROR_TYPE = PARSE_ERROR;
				sprintf(ERROR_MSG, "Missing bindings in let-in expression.");
				return NULL;
			}
			token = lex(rest);
			if (token == NULL || strcmp(token->value, "in") != 0) {
				ERROR_TYPE = PARSE_ERROR;
				sprintf(ERROR_MSG, "Unfinished `let ... in ...` expression. Missing `in`.");
				return NULL;
			}
			ParseNode *expr = parse_expr(rest, min_prec);
			if (expr == NULL) {
				ERROR_TYPE = PARSE_ERROR;
				sprintf(ERROR_MSG, "Missing result in let-in expression.");
				return NULL;
			}
			node->type = BINARY_NODE;
			ParseNode *callee = malloc(sizeof(ParseNode));
			callee->type = IDENT_NODE;
			callee->node.ident = (IdentNode){ .value = strdup("let-in") };
			node->node.binary.callee = callee;
			node->node.binary.left = bindings;
			node->node.binary.right = expr;
			break;
		}
		// Otherwise, just produce an ident.
		node_into_ident(token->value, node);
		break;
	}
	case TT_STRING: {
		node->type = STRING_NODE;  // TODO: Parse string escapes etc.
		node->node.str.value = strdup(token->value + 1);
		node->node.str.len = strlen(token->value) - 2;
		node->node.str.value[node->node.str.len] = '\0';
		break;
	}
	case TT_OPERATOR: {
		// Verify this is a prefix operator.
		bool is_prefix = false;
		iprec precedence = min_prec;
		for (usize i = 0; i < len(KNOWN_OPERATORS); ++i) {
			Operator op = KNOWN_OPERATORS[i];
			if (strcmp(token->value, op.value) == 0 && op.fixity == PREFIX) {
				is_prefix = true;
				precedence = op.precedence;
				break;
			}
		}
		if (!is_prefix) {
			ERROR_TYPE = PARSE_ERROR;
			sprintf(ERROR_MSG,
				"`%s' operator cannot be used as a prefix.\n"
				"  Missing left-hand-side argument of `%s' operator.",
				token->value, token->value);
			return NULL;
		}

		UnaryNode *unary = malloc(sizeof(UnaryNode));
		ParseNode *callee = malloc(sizeof(ParseNode));
		node_into_ident(token->value, callee);

		unary->callee = callee;
		unary->operand = parse_expr(rest, precedence);
		if (unary->operand == NULL) {
			ERROR_TYPE = PARSE_ERROR;
			sprintf(ERROR_MSG, "Missing right-hand-side of prefix"
				" operator `%s'.", token->value);
			return NULL;
		}
		unary->is_postfix = false;

		node->type = UNARY_NODE;
		node->node.unary = *unary;

		break;
	}
	case TT_LPAREN: {
		node = parse_expr(rest, min_prec);
		token = lex(rest);
		if (token == NULL || token->type != TT_RPAREN) {
			ERROR_TYPE = PARSE_ERROR;
			sprintf(ERROR_MSG, "Unclosed paranthetical expression.\n"
				"  Missing `)' closing parenthesis.");
			return NULL;
		}
		free_token((Token *)token);
		break;
	}
	default:
		return NULL;
	}

	return node;
}

iprec token_precedence(Token *token)
{
	// Check if its an `)'.
	if (token->type == TT_RPAREN)
		return 0;
	if (token->type == TT_IDENTIFIER && strcmp(token->value, "in") == 0) {
		return 0;
	}
	// Check if its an operator.
	for (usize i = 0; i < len(KNOWN_OPERATORS); ++i) {
		Operator op = KNOWN_OPERATORS[i];
		if (strcmp(token->value, op.value) == 0
		&& (op.fixity == INFIX || op.fixity == POSTFIX)) {
			return op.precedence;
		}
	}
	// Otherwise, assume it's a function application.
	return FUNCTION_PRECEDENCE;
}

ParseNode *parse_infix(const ParseNode *left,
	const Token *token,
	char **rest, iprec last_precedence)
{
	bool is_postfix = false;
	bool is_operator = false;
	iprec precedence = min_prec;
	Associativity assoc = NEITHER_ASSOC;

	// Extract information on operator (if operator at all)
	for (usize i = 0; i < len(KNOWN_OPERATORS); ++i) {
		Operator op = KNOWN_OPERATORS[i];
		if (strcmp(token->value, op.value) == 0) {
			is_operator = true;
			precedence = op.precedence;
			assoc = op.assoc;
			if (op.fixity == POSTFIX) {
				is_postfix = true;
				break;
			}
		}
	}

	// Function calls and post fix operators share some common code.
	if (is_postfix || !is_operator) {
		UnaryNode *unary = malloc(sizeof(UnaryNode));

		if (is_postfix) { // Postfix operator.
			ParseNode *node = malloc(sizeof(ParseNode));
			node_into_ident(token->value, node);
			unary->callee = node;
			unary->operand = left;
			unary->is_postfix = true;
		} else { // Function call, probably.
			if (token->type == TT_OPERATOR)
				return NULL; // Not a real operator, not a function.

			unary->callee = left;
			// The minus one (- 1) makes function application right
			// associative, this is unconventional, and makes applications
			// on functions that return functions not very pretty.
			// However, it makes for more natural syntax for multiplication
			// by juxtaposition.
			// e.g.  3 sin 2  =>  (3 (sin 2)) vs ((3 sin) 2)  [<- error]
			unary->operand = parse_expr(rest, FUNCTION_PRECEDENCE - 1);
			if (unary->operand == NULL)
				return NULL;
			unary->is_postfix = false;
		}

		ParseNode *node = malloc(sizeof(ParseNode));
		node->type = UNARY_NODE;
		node->node.unary = *unary;

		return node;
	}

	// Trick for right associativity is to pretend that what's
	// to the left of you is slightly higher precedence.
	if (assoc == RIGHT_ASSOC)
		precedence -= 1;

	// Don't allow chaining of operators with no
	// left or right associativity.
	if (assoc == NEITHER_ASSOC && precedence == last_precedence) {
		ERROR_TYPE = PARSE_ERROR;
		sprintf(ERROR_MSG, "Operator `%s' cannot be chained with\n"
			"  operators of equal precedence, please use parentheses.",
			token->value);
		return NULL;
	}

	// Binary operator:

	// Call node.
	ParseNode *callee = malloc(sizeof(ParseNode));
	node_into_ident(token->value, callee);

	// Root binary node.
	ParseNode *binary_node = malloc(sizeof(ParseNode));
	BinaryNode *binary = malloc(sizeof(BinaryNode));

	// Populate binary struct.
	binary->callee = callee;
	binary->left = left;
	binary->right = parse_expr(rest, precedence);

	if (binary->right == NULL) {
		ERROR_TYPE = PARSE_ERROR;
		sprintf(ERROR_MSG,
			"Attempted to use `%s' infix-operator as a suffix.\n"
			"  Missing right-hand-side argument of `%s' operator.",
			token->value, token->value);
		return NULL;
	}

	// Populate parse node as binary parse node.
	binary_node->type = BINARY_NODE;
	binary_node->node.binary = *binary;

	return binary_node;
}

ParseNode *parse_expr(char **slice, iprec precedence)
{
	Token *token = peek(slice);

	if (token == NULL)
		return NULL;

	// Never consume a `)` or `in` token.
	switch (token->type) {
		case TT_RPAREN: return NULL;
		case TT_IDENTIFIER: if (strcmp(token->value, "in") == 0) return NULL;
		default: break;
	}

	// Advance tokens.
	token = lex(slice);
	ParseNode *left = parse_prefix(token, slice);


	if (left == NULL)
		return NULL;

	Token *token_ahead = peek(slice);

	if (token_ahead == NULL)
		return left;

	iprec current_precedence = token_precedence(token_ahead);
	iprec previous_precedence = min_prec;

	usize count = 0;
	while (precedence < current_precedence) {
		count += 1;
		if (count > 301) {
			ERROR_TYPE = PARSE_ERROR;
			strcpy(ERROR_MSG, "Could not finish parsing expression.");
			break;
		}
		token = current_precedence == FUNCTION_PRECEDENCE
			? peek(slice)
			:  lex(slice);

		if (token == NULL)
			break;

		left = parse_infix(left, token, slice, previous_precedence);
		if (left == NULL || **slice == '\0')
			break;

		token_ahead = peek(slice);
		if (token_ahead == NULL)
			break;

		previous_precedence = current_precedence;
		current_precedence = token_precedence(token_ahead);
	}

	if (token_ahead != NULL)
		free_token((Token *)token_ahead);
	if (token != NULL)
		free_token((Token *)token);
	return left;
}

ParseNode *parse(const char *source)
{
	char *stepper = strdup(source);
	char *start = stepper;
	ParseNode *tree = parse_expr(&stepper, min_prec);
	free(start);
	return tree;
}
