#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <stdlib.h>
#include <string.h>

#include "prelude.h"
#include "error.h"
#include "parse.h"
#include "displays.h"

static const char *PROMPT = "::> ";

int main(int argc, char **argv)
{
	printf("\033[1m");
	printf("CREPL — Calculator Read Eval Print Loop");
	printf("\033[0m");
	puts(" (" COMPILER ") (" __DATE__ ")");
	puts("Type \"exit\" or [Ctrl+D] (i.e. EOF) to quit.");

	rl_bind_key('\t', rl_complete);

	char *response = NULL;
	do {
		response = readline(PROMPT);
		add_history(response);

		if (response == NULL
		|| strcmp("exit", downcase(trim(response))) == 0)
			break;

		// Try to lex & parse the input.
		ParseNode *tree = parse(response);

		if (ERROR_TYPE != NO_ERROR) {
			handle_error();
			continue;
		}

		if (tree == NULL) continue;

		printf("\033[%luC\033[1A", strlen(PROMPT) + strlen(response));
		printf(" ≡ %s\n", display_parsetree(tree));
		printf("#=> %s\n", response);

		free_parsenode(tree);
		free(response);
	} while (true);

	printf("\r\033[2K");
	printf("Buh-bye.\n");

	return EXIT_SUCCESS;
}


