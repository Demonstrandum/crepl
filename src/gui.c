#include "gui.h"

#ifdef GUI

void grid_fit(GtkWidget *grid)
{
	g_object_set(G_OBJECT(grid),
		"column-homogeneous", TRUE,
		"row-homogeneous", TRUE,
		"column-spacing", 10,
		"row-spacing", 10,
		NULL);
}

typedef struct {
	GtkWidget *layout;
	GtkWidget *display;
	GtkWidget *output;
	Context *exe_ctx;
} Calculator;

typedef enum {
	// Num keys
	ACT_NUMBER_0 = 0,
	ACT_NUMBER_1,
	ACT_NUMBER_2,
	ACT_NUMBER_3,
	ACT_NUMBER_4,
	ACT_NUMBER_5,
	ACT_NUMBER_6,
	ACT_NUMBER_7,
	ACT_NUMBER_8,
	ACT_NUMBER_9,
	// Operators
	ACT_MUL, ACT_ADD,
	ACT_DIV, ACT_SUB,
	// Actions
	ACT_M_PLUS, ACT_M_MINUS,
	ACT_EXEC,
	ACT_DEL,

	ACTIONS_COUNT  //< Always last!
} Action;

typedef struct {
	Action action;
	GtkWidget *button;
	Calculator *calculator;
} ButtonPress;

static void write_to_editable(GtkEditable *editable, byte *input, gint len, gint *pos)
{
	// Wipe selection if inserting text.
	gtk_editable_delete_selection(editable);
	// Insert the string.
	gtk_editable_insert_text(editable, input, len, pos);
	// Move cursor forwards.
	*pos += 1;
	gtk_editable_set_position(editable, *pos);
}

static void button_press(GtkWidget *widget, gpointer data)
{
	UNUSED(widget);

	ButtonPress *press = (ButtonPress *)data;
	Action action = press->action;
	Calculator *ctx = press->calculator;

	printf("Action Number: %d.\n", action);

	GtkEntry *entry = GTK_ENTRY(ctx->display);
	GtkEntryBuffer *entry_buf =  gtk_entry_get_buffer(entry);
	UNUSED(entry_buf);

	gint curs_pos = gtk_editable_get_position(GTK_EDITABLE(ctx->display));

	if (action <= 9 && action >= 0) {
		byte digit = '0' + action;
		write_to_editable(GTK_EDITABLE(ctx->display), &digit, 1, &curs_pos);
		return;
	}

	switch (action) {
	case ACT_EXEC: {
		const byte *source = gtk_entry_get_text(entry);
		printf("Executing input: %s\n", source);

		ParseNode *tree   = NULL;
		DataValue *result = NULL;

		tree = parse(source);
		if (tree == NULL || ERROR_TYPE != NO_ERROR) {
			handle_error();
			return;
		}
		result = execute(ctx->exe_ctx, tree);
		if (result == NULL || ERROR_TYPE != NO_ERROR) {
			handle_error();
			return;
		}

		if (result != NULL)
			gtk_entry_set_text(GTK_ENTRY(ctx->output), display_datavalue(result));
		if (tree != NULL)
			free_parsenode(tree);
	} break;
	case ACT_DEL: {
		gboolean has_selection = gtk_editable_get_selection_bounds(
			GTK_EDITABLE(ctx->display), NULL, NULL);

		if (has_selection) {
			gtk_editable_delete_selection(GTK_EDITABLE(ctx->display));
		} else {
			printf("Delete char at %d -- %d.\n", curs_pos - 1, curs_pos);
			gtk_editable_delete_text(GTK_EDITABLE(ctx->display),
				curs_pos - 1, curs_pos);
		}
	} break;
	default:
		printf("Unhandled action: %d.\n", action);
	}
}

static Calculator ctx
	= { .layout  = NULL
	  , .display = NULL
	  , .output  = NULL
	  , .exe_ctx = NULL
	  };

static ButtonPress keys[ACTIONS_COUNT] = { 0 };

static void on_activate(GtkApplication *app)
{
	GtkWidget *window = gtk_application_window_new(app);

	GtkWidget *header = gtk_header_bar_new();
	gtk_header_bar_set_title((GtkHeaderBar *)header, "CREPL — Calculator");
	gtk_header_bar_set_show_close_button((GtkHeaderBar *)header, TRUE);

	// Init calculator keys.
	for (Action act = ACT_NUMBER_0; act < ACTIONS_COUNT; ++act) {
		keys[act].action = act;
		keys[act].calculator = &ctx;
	}
	// Init execution context.
	ctx.exe_ctx = base_context();

	// Calculator main layout.
	GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_set_margin_top   (layout, 20);
	gtk_widget_set_margin_start (layout, 20);
	gtk_widget_set_margin_bottom(layout, 20);
	gtk_widget_set_margin_end   (layout, 20);
	ctx.layout = layout;

	// Calculator input display.
	GtkWidget *input_display = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(input_display), "crepl");
	g_signal_connect(input_display, "activate",
		G_CALLBACK(button_press), (gpointer)(&keys[ACT_EXEC]));
	ctx.display = input_display;

	// Output/result display.
	GtkWidget *output_display = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(output_display), "...");
	gtk_editable_set_editable(GTK_EDITABLE(output_display), FALSE);
	gtk_entry_set_alignment(GTK_ENTRY(output_display), 1);
	gtk_entry_set_has_frame(GTK_ENTRY(output_display), FALSE);
	gtk_widget_set_can_focus(output_display, FALSE);
	ctx.output = output_display;

	gtk_widget_set_margin_top(output_display, 0);
	gtk_box_pack_start(GTK_BOX(layout),  input_display, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(layout), output_display, TRUE, TRUE, 2);

	// Calculator mode keys.
	GtkWidget *settings_grid = gtk_grid_new();
	grid_fit(settings_grid);

	GtkWidget *M_plus =  gtk_button_new_with_label("M+");  // ACT_M_PLUS.
	GtkWidget *M_minus = gtk_button_new_with_label("M-");  // ACT_M_MINUS.
	GtkWidget *execute = gtk_button_new_with_label("EXE"); // ACT_EXEC.
	GtkWidget *delete =  gtk_button_new_with_label("DEL"); // ACT_DEL.

	keys[ACT_EXEC].button = execute;
	g_signal_connect(execute, "clicked",
		G_CALLBACK(button_press), (gpointer)(&keys[ACT_EXEC]));
	keys[ACT_DEL].button = delete;
	g_signal_connect(delete, "clicked",
		G_CALLBACK(button_press), (gpointer)(&keys[ACT_DEL]));

	gtk_grid_attach(GTK_GRID(settings_grid), M_plus,  1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(settings_grid), M_minus, 2, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(settings_grid), execute, 3, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(settings_grid), delete,  4, 1, 1, 1);

	gtk_box_pack_start(GTK_BOX(layout), settings_grid, TRUE, TRUE, 10);

	/* Main Keys */
	GtkWidget *main_keys = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);

	// Calculator numpad.
	GtkWidget *numbers = gtk_grid_new();
	grid_fit(numbers);

	for (u8 n = 0; n < 9; ++n) {
		byte digit[2] = { '1' + n, '\0' };
		GtkWidget *button = gtk_button_new_with_label(digit);

		keys[n + 1].button = button;
		g_signal_connect(button, "clicked",
			G_CALLBACK(button_press), (gpointer)(&keys[n + 1]));

		gtk_grid_attach(GTK_GRID(numbers), button,
			n % 3 + 1, n / 3 + 1, 1, 1);
	}
	GtkWidget *number_zero = gtk_button_new_with_label("0");
	keys[ACT_NUMBER_0].button = number_zero;
	g_signal_connect(number_zero, "clicked",
		G_CALLBACK(button_press), (gpointer)(&keys[ACT_NUMBER_0]));
	gtk_grid_attach(GTK_GRID(numbers), number_zero, 2, 4, 1, 1);

	gtk_box_pack_start(GTK_BOX(main_keys), numbers, TRUE, TRUE, 10);

	// Calculator function keys.
	GtkWidget *operators_grid = gtk_grid_new();
	grid_fit(operators_grid);
	GtkWidget *functions_grid = gtk_grid_new();
	grid_fit(functions_grid);

	// Operators, column #1.
	GtkWidget *b_mul = gtk_button_new_with_label("*");
	GtkWidget *b_add = gtk_button_new_with_label("+");
	GtkWidget *b_div = gtk_button_new_with_label("/");
	GtkWidget *b_sub = gtk_button_new_with_label("-");

	// Operators column #2.
	GtkWidget *b_pow = gtk_button_new_with_label("^");
	GtkWidget *b_fac = gtk_button_new_with_label("!");
	GtkWidget *b_sqr = gtk_button_new_with_label("√");
	GtkWidget *b_equ = gtk_button_new_with_label("=");

	// Functions, column #3.
	GtkWidget *b_log = gtk_button_new_with_label("log");
	GtkWidget *b_sin = gtk_button_new_with_label("sin");
	GtkWidget *b_cos = gtk_button_new_with_label("cos");
	GtkWidget *b_abs = gtk_button_new_with_label("abs");

	// Constants, column #4.
	GtkWidget *b_pi  = gtk_button_new_with_label("π");
	GtkWidget *b_e   = gtk_button_new_with_label("e");
	GtkWidget *b_phi = gtk_button_new_with_label("φ");
	GtkWidget *b_ans = gtk_button_new_with_label("Ans");

	// Column #1.
	gtk_grid_attach(GTK_GRID(operators_grid), b_mul, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(operators_grid), b_add, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(operators_grid), b_div, 1, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(operators_grid), b_sub, 1, 4, 1, 1);
	// Column #2.
	gtk_grid_attach(GTK_GRID(operators_grid), b_pow, 2, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(operators_grid), b_fac, 2, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(operators_grid), b_sqr, 2, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(operators_grid), b_equ, 2, 4, 1, 1);
	// Column #3.
	gtk_grid_attach(GTK_GRID(functions_grid), b_log, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(functions_grid), b_sin, 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(functions_grid), b_cos, 1, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(functions_grid), b_abs, 1, 4, 1, 1);
	// Column #4.
	gtk_grid_attach(GTK_GRID(functions_grid), b_pi,  2, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(functions_grid), b_e,   2, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(functions_grid), b_phi, 2, 3, 1, 1);
	gtk_grid_attach(GTK_GRID(functions_grid), b_ans, 2, 4, 1, 1);

	gtk_box_pack_end(GTK_BOX(main_keys), functions_grid, TRUE, TRUE, 10);
	gtk_box_pack_end(GTK_BOX(main_keys), operators_grid, TRUE, TRUE, 10);

	// Set all keys/buttons to be unfocusable.
	for (Action act = ACT_NUMBER_0; act < ACTIONS_COUNT; ++act)
		if (keys[act].button != NULL)
			gtk_widget_set_can_focus(keys[act].button, FALSE);

	gtk_window_set_titlebar(GTK_WINDOW(window), header);
	gtk_box_pack_end(GTK_BOX(layout), main_keys, TRUE, TRUE, 10);
	gtk_container_add(GTK_CONTAINER(window), layout);

	gtk_widget_show_all(window);
}

int start_gui(void)
{
	printf("Start GUI.\n");

	GtkApplication *app = gtk_application_new("com.crepl.GtkApplication",
		G_APPLICATION_FLAGS_NONE);

	g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
	printf("Started application.\n");
	return g_application_run(G_APPLICATION(app), 0, NULL);
}

#else

#include <stdio.h>
int start_gui(void)
{
	fputs("Not compiled with GUI support.", stderr);
	return -1;
}

#endif
