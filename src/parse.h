#pragma once

#include "defaults.h"

// Tokens:
typedef enum {
	TT_LPAREN, TT_RPAREN,
	TT_IDENTIFIER,
	TT_NUMERIC,
	TT_OPERATOR,
	TT_STRING,
	TT_NONE,
} TokenType;

typedef struct {
	TokenType type;
	const char *value;
} Token;

Token *new_token(TokenType, const char *);

/* Operator properties. */

// permitted precedence levels: 0...65,535.
typedef u16 iprec;
static const iprec min_prec = 0x0000;
static const iprec max_prec = 0xffff;

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
	iprec precedence;
	Associativity assoc;
	Fixity fixity;
} Operator;

static const iprec FUNCTION_PRECEDENCE = 90;
// Known operators from longest to shortests.
static const Operator KNOWN_OPERATORS[] = {
    // 4 characters long.
    { "where", 5, RIGHT_ASSOC, INFIX },
	// 3 characters long.
	{ "not", 25, RIGHT_ASSOC, PREFIX },
	// 2 characters long.
	{ "**", 100, RIGHT_ASSOC, INFIX },
	{ "<=",  40,  LEFT_ASSOC, INFIX },
	{ ">=",  40,  LEFT_ASSOC, INFIX },
	{ "==",  30,  LEFT_ASSOC, INFIX },
	{ "/=",  30,  LEFT_ASSOC, INFIX },
	{ "->",  25, RIGHT_ASSOC, INFIX },
	// 1 character long.
	{ "-", 100, RIGHT_ASSOC, PREFIX },
	{ "+", 100, RIGHT_ASSOC, PREFIX },
	{ "¬", 100, RIGHT_ASSOC, PREFIX },
	{ "!", 100,  LEFT_ASSOC, POSTFIX },
	{ "^", 100, RIGHT_ASSOC, INFIX },
	{ "*",  60,  LEFT_ASSOC, INFIX },
	{ "/",  60,  LEFT_ASSOC, INFIX },
	{ "+",  50,  LEFT_ASSOC, INFIX },
	{ "-",  50,  LEFT_ASSOC, INFIX },
	{ ">",  40,  LEFT_ASSOC, INFIX },
	{ "<",  40,  LEFT_ASSOC, INFIX },
	{ "=",  20, RIGHT_ASSOC, INFIX },
	{ ",",  10, RIGHT_ASSOC, INFIX },
	{ ";",   1,  LEFT_ASSOC, INFIX },
	/* left paren is only zero-precedence op: { "(", 0, ... } */
};

// Parse tree nodes:
typedef enum {
	IDENT_NODE,
	NUMBER_NODE,
	STRING_NODE,
	UNARY_NODE,
	BINARY_NODE,
} NodeType;

struct _parse_node;

typedef struct {
	char *value;
} IdentNode;

typedef struct {
	usize len;
	byte *value;
} StringNode;

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
		StringNode str;
		NumberNode number;
		UnaryNode unary;
		BinaryNode binary;
	} node;
} ParseNode;

// Functions:

void free_token(Token *);
void free_parsenode(ParseNode *);
ParseNode *clone_node(const ParseNode *);

Token *lex(char **);
Token *peek(char **);

NumberNode *make_number(NumberType, void *);
NumberNode *parse_number(const char *);
ParseNode *parse_prefix(const Token *, char **);
ParseNode *parse_infix(const ParseNode *, const Token *, char **, iprec);
ParseNode *parse_expr(char **, iprec);
ParseNode *parse(const char *);
