#include "prelude.h"

char *PRELUDE_STATEMENTS[] = {
	"tau = 2pi",
	"phi = (1 + sqrt 5) / 2",
	"crepl = \"Calculator REPL, v" VERSION ".\""
};

void execute_prelude(Context *ctx)
{
	for (usize i = 0; i < len(PRELUDE_STATEMENTS); ++i) {
		const char *stmt_str = PRELUDE_STATEMENTS[i];

		ParseNode *stmt = parse(stmt_str);
		if (stmt == NULL || ERROR_TYPE != NO_ERROR)
			goto fatality;

		DataValue *result = execute(ctx, stmt);
		if (result == NULL || ERROR_TYPE != NO_ERROR)
			goto fatality;

		free(stmt);
	}
	return;

fatality:
	handle_error();
	fprintf(stderr, "\nFATAL: Prelude failed to run without error.\n");
	fprintf(stderr, "ABORTING!\n");
	exit(1);
}
