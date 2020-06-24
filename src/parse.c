#include "prelude.h"

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
	}
	free(node);
}

/* --- Functions related to tokenising/lexing --- */

Token *new_token(TokenType type, const char *value)
{
	Token *t = malloc(sizeof(Token));
	t->type  = type;
	t->value = strdup(value);
	return t;
}

TokenType char_token_type(char c, TokenType last_token_type)
{
	if (c <= '9' && c >= '0') {
		if (last_token_type == TT_IDENTIFIER)
			return TT_IDENTIFIER;
		else
			return TT_NUMERIC;
	}
	if (c == '_'
	&& (last_token_type	== TT_IDENTIFIER
	|| last_token_type == TT_NUMERIC))
		return last_token_type;
	if (c == '.'
	&& (last_token_type == TT_OPERATOR
	|| last_token_type == TT_NONE))
		return TT_NUMERIC;
	if ((c == '.' || c == 'E')
	&& last_token_type == TT_NUMERIC)
		return TT_NUMERIC;
	if (c == '(')
		return TT_LPAREN;
	if (c == ')')
		return TT_RPAREN;
	if (c == '\0' || c == ' ' || c == '\t' || c == '\n' || c == '\r')
		return TT_NONE;

	// All possible operator/special-symbol characters:
	if ((c >= '!' && c <= '/')
	||  (c >= ':' && c <= '@')
	||  (c >= '{' && c <= '~')) {
		return TT_OPERATOR;
	}

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

	TokenType tt = char_token_type(**source, TT_NONE);

	// Skip over TT_NONE tokens (spaces, tabs, etc.).
	while (tt == TT_NONE) {
		(*source)++;
		if (**source == '\0')
			return NULL;
		tt = char_token_type(**source, tt);
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
	} else {
		while (tt == previous_tt) {
			span++;
			previous_tt = char_token_type(*(*source + span), previous_tt);
		}
	}

	char *sub_str = malloc(span * sizeof(char));
	strncpy(sub_str, *source, span);
	sub_str[span] = '\0';

	*source += span;
	Token *token = new_token(tt, sub_str);
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

// TODO: Support more than just INTs.
NumberNode *parse_number(const char *str)
{
	NumberNode *number = malloc(sizeof(NumberNode));
	number->type = INT;
	number->value.i = atoi(str);
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
			sprintf(ERROR_MSG, "Malformatted number literal (`%s').",
				token->value);
			return NULL;
		}

		node->type = NUMBER_NODE;
		node->node.number = *num;
		break;
	}
	case TT_IDENTIFIER: {
		node_into_ident(token->value, node);
		break;
	}
	case TT_OPERATOR: {
		// Verify this is a prefix operator.
		bool is_prefix = false;
		u16 precedence = 0;
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
		unary->is_postfix = false;

		node->type = UNARY_NODE;
		node->node.unary = *unary;

		printf("Parsed prefix as: %s\n\n", display_parsetree(node));

		break;
	}
	case TT_LPAREN: {
		node = parse_expr(rest, 0);
		token = lex(rest);
		if (token == NULL || token->type != TT_RPAREN) {
			ERROR_TYPE = PARSE_ERROR;
			sprintf(ERROR_MSG, "Unclosed paranthetical expression.\n"
				"  Missing `)' closing parenthesis.");
			free_token((Token *)token);
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

u16 token_precedence(Token *token)
{
	// Check if its an `)'.
	if (token->type == TT_RPAREN)
		return 0;
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
	char **rest, u16 last_precedence)
{
	bool is_postfix = false;
	bool is_operator = false;
	u16 precedence = 0;
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
		} else { // Function call.
			unary->callee = left;
			unary->operand = parse_expr(rest, FUNCTION_PRECEDENCE);
			unary->is_postfix = false;
		}

		ParseNode *node = malloc(sizeof(ParseNode));
		node->type = UNARY_NODE;
		node->node.unary = *unary;

		return node;
	}

	// Trick for right associativity is to pretend that what's
	// to the left of you is sligtly higher precedence.
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

ParseNode *parse_expr(char **slice, u16 precedence)
{
	Token *token = lex(slice);

	if (token == NULL)
		return NULL;

	ParseNode *left = parse_prefix(token, slice);

	if (left == NULL)
		return NULL;

	Token *token_ahead = peek(slice);

	if (token_ahead == NULL)
		return left;

	u16 current_precedence = token_precedence(token_ahead);
	u16 previous_precedence = 0;

	usize count = 0;
	while (precedence < current_precedence) {
		count += 1;
		if (count > 301) {
			ERROR_TYPE = PARSE_ERROR;
			strcpy(ERROR_MSG, "Could not finish parsing expression.");
			break;
		}
		if (current_precedence != FUNCTION_PRECEDENCE)
			token = lex(slice);
		else
			token = peek(slice);

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

	free_token((Token *)token_ahead);
	free_token((Token *)token);
	return left;
}

ParseNode *parse(char *source)
{
	return parse_expr(&source, 0);
}
