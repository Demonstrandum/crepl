#pragma once

#include "defaults.h"

// Tokens:
typedef enum {
	TT_LPAREN, TT_RPAREN,
	TT_IDENTIFIER,
	TT_NUMERIC,
	TT_OPERATOR,
	TT_NONE,
} TokenType;

typedef struct {
	TokenType type;
	const char *value;
} Token;

Token *new_token(TokenType, const char *);

// Operator properties.
typedef enum {
	LEFT_ASSOC,
	RIGHT_ASSOC,
	NEITHER_ASSOC,
} Associativity;

typedef enum {
	PREFIX,
	INFIX,
	POSTFIX,
} Fixity;

typedef struct {
	const char *value;
	u16 precedence;
	Associativity assoc;
	Fixity fixity;
} Operator;

static const u16 FUNCTION_PRECEDENCE = 9;
// Known operators from longest to shortests.
static const Operator KNOWN_OPERATORS[] = {
	// 3 characters long.
	{ "not", 8, RIGHT_ASSOC, PREFIX },
	// 2 characters long.
	{ "**", 10, RIGHT_ASSOC, INFIX },
	{ "<=", 4, LEFT_ASSOC, INFIX },
	{ ">=", 4, LEFT_ASSOC, INFIX },
	{ "==", 3, LEFT_ASSOC, INFIX },
	{ "/=", 3, LEFT_ASSOC, INFIX },
	// 1 character long.
	{ "-", 10, RIGHT_ASSOC, PREFIX },
	{ "+", 10, RIGHT_ASSOC, PREFIX },
	{ "¬", 10, RIGHT_ASSOC, PREFIX },
	{ "!", 10, LEFT_ASSOC, POSTFIX },
	{ "^", 10, RIGHT_ASSOC, INFIX },
	{ "*", 6, LEFT_ASSOC, INFIX },
	{ "/", 6, LEFT_ASSOC, INFIX },
	{ "+", 5, LEFT_ASSOC, INFIX },
	{ "-", 5, LEFT_ASSOC, INFIX },
	{ ">", 4, LEFT_ASSOC, INFIX },
	{ "<", 4, LEFT_ASSOC, INFIX },
	{ "=", 2, RIGHT_ASSOC, INFIX },
	{ ",", 1, RIGHT_ASSOC, INFIX },
};

// Parse tree nodes:
typedef enum {
	IDENT_NODE,
	NUMBER_NODE,
	UNARY_NODE,
	BINARY_NODE,
} NodeType;

struct _parse_node;

typedef struct {
	char *value;
} IdentNode;

typedef enum {
	FLOAT,
	INT,
	BIGINT, // Not supported yet.
	RATIO,  // Not supported yet.
} NumberType;

typedef struct {
	NumberType type;
	union {
		fsize f;
		ssize i;
		// Add bigint_t and ratio_t in future.
	} value;
} NumberNode;

typedef struct {
	const struct _parse_node *callee;
	const struct _parse_node *operand;
	bool is_postfix;
} UnaryNode;

typedef struct {
	const struct _parse_node *callee;
	const struct _parse_node *left;
	const struct _parse_node *right;
} BinaryNode;

typedef struct _parse_node {
	NodeType type;
	union {
		IdentNode ident;
		NumberNode number;
		UnaryNode unary;
		BinaryNode binary;
	} node;
} ParseNode;

// Functions:

void free_token(Token *);
void free_parsenode(ParseNode *);

Token *lex(char **);
Token *peek(char **);

NumberNode *make_number(NumberType, void *);
NumberNode *parse_number(const char *);
ParseNode *parse_prefix(const Token *, char **);
ParseNode *parse_infix(const ParseNode *, const Token *, char **, u16);
ParseNode *parse_expr(char **, u16);
ParseNode *parse(const char *);
