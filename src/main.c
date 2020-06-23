#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "prelude.h"
#include "error.h"
#include "parse.h"
#include "displays.h"

static const char *PROMPT = "::> ";

void sigint_handle(int _)
{
	printf("\b\b  ");  // Obsucre '^C' output.
	printf("\nInterrupted, [Ctrl-D] to stop inputting.\n");
    rl_on_new_line();
	rl_replace_line("", 0);
    rl_redisplay();
}

int main(int argc, char **argv)
{
	// Welcome messsage.
	printf("\033[1m");
	printf("CREPL — Calculator Read Eval Print Loop");
	printf("\033[0m");
	puts(" (" COMPILER ") (" __DATE__ ")");
	puts("Type \"exit\" or [Ctrl-D] (i.e. EOF) to quit.");

	// Configure readline.
	rl_clear_signals();
	rl_bind_key('\t', rl_insert);
	signal(SIGINT, sigint_handle);

	// Create or fetch history file.
	char cache_loc[128] = "~/.cache/";
	char *tmp = NULL;

	if ((tmp = getenv("XDG_CACHE_HOME")) != NULL)
		strcpy(cache_loc, tmp);
	else if ((tmp = getenv("HOME")) != NULL)
		sprintf(cache_loc, "%s/.cache/", tmp);

	mkdir(cache_loc, 0777);
	strcat(cache_loc, "/crepl.history");
	read_history(cache_loc);

	char *response = NULL;
	do {
		char *line = readline(PROMPT);

		if (line == NULL
		|| strcmp("exit", downcase(trim(line))) == 0)
			break;

		if (response == NULL || strcmp(line, response) != 0)
			add_history(line);
		response = line;

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
	} while (true);

	write_history(cache_loc);

	printf("\r\033[2K");
	printf("Buh-bye.\n");

	return EXIT_SUCCESS;
}


