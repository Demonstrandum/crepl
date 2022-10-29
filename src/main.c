#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <wchar.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "defaults.h"
#include "error.h"
#include "parse.h"
#include "execute.h"
#include "displays.h"
#include "gui.h"

static const char *PROMPT = "::> ";

char *response = NULL;
pthread_t thread_id = 0;

void sigint_handle(int sig)
{
	printf("\b\b  ");  // Obsucre '^C' output.
	if (thread_id == 0) {
		puts("\b\b\033[90m//----\033[0m");
		printf("\033[1m");
		printf("Signal (%d), [Ctrl-D] to stop inputting.\n", sig);
		printf("\033[0m");
		rl_on_new_line();
		rl_replace_line("", 0);
		rl_redisplay();
	} else {
		pthread_cancel(thread_id);
		printf("\b\b\033[1mInterrputed expression evaluation");
		printf(" (thread 0x%lX).\033[0m\n", thread_id);
		thread_id = 0;
	}

}

void *evaluation_thread(void *ctx_void)
{
	Context *ctx = ctx_void;

	ParseNode *tree   = NULL;
	DataValue *result = NULL;

	tree = parse(response);

	if (tree == NULL || ERROR_TYPE != NO_ERROR) {
		handle_error();
		return (void *)EXIT_FAILURE;
	}

	printf("\033[%luC\033[1A",
		strlen(PROMPT)
		+ strlen(response));
	printf("\033[2m ≡ %s\033[0m\n", display_parsetree(tree));

	result = execute(ctx, tree);

	if (result == NULL || ERROR_TYPE != NO_ERROR) {
		handle_error();
		return (void *)EXIT_FAILURE;
	}

	if (result != NULL)
		printf("#=> %s\n", display_datavalue(result));
	if (tree != NULL)
		free_parsenode(tree);

	return (void *)EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	// Welcome messsage.
	printf("\033[1m");
	printf("CREPL * Calculator Read Eval Print Loop");
	printf("\033[0m");
	puts(" (v" VERSION ") (" COMPILER ") (" __DATE__ ")");

	bool verbose = false;
	bool gui_mode = false;
	// Parse command line arguments.
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-v") == 0
		||  strcmp(argv[i], "--verbose") == 0)
			verbose = true;
		else if (strcmp(argv[i], "-V") == 0
		||       strcmp(argv[i], "--version") == 0)
			{ puts("Version " VERSION "."); return EXIT_SUCCESS; }
		else if (strcmp(argv[i], "-g") == 0
		||       strcmp(argv[i], "--gui") == 0)
			gui_mode = true;
	}

#ifndef GUI
	if (gui_mode) {
		puts("This CREPL executable was not compiled with GUI support.");
		return EXIT_FAILURE;
	}
#else
	if (gui_mode) {
		int success = start_gui();
		if (success == 0)
			return EXIT_SUCCESS;
		return EXIT_FAILURE;
	}
#endif

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
		sprintf(cache_loc, "%s/.cache", tmp);

	mkdir(cache_loc, 0777);
	strcat(cache_loc, "/crepl.history");
	read_history(cache_loc);

	if (verbose) {
		printf("Reading history from `%s'.\n", cache_loc);
	}

	Context *ctx = base_context();

	do {
		char *line = readline(PROMPT);

		if (line == NULL
		|| strcmp("exit", downcase(trim(line))) == 0)
			break;

		if (response == NULL || strcmp(line, response) != 0)
			add_history(line);
		response = line;

		// Evaluation of input is done in thread.
		pthread_create(&thread_id, NULL, evaluation_thread, ctx);
		pthread_join(thread_id, NULL);
		thread_id = 0;
	} while (true);

	write_history(cache_loc);

	printf("\r\033[2K");
	printf("Buh-bye.\n");

	return EXIT_SUCCESS;
}


