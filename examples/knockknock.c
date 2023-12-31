#define HEY_IMPLEMENTATION
#define SOKOL_IMPL
#include <hey_dsl.h>
#include <hey.h>
#include "common.h"
#include <hey_suffix.h>

// TODO: Make this a standard header?
hey_control_decision_t
ends_at_punctuation(
	hey_index_t* count,
	hey_exec_t* ctx, void* userdata
) {
	(void)userdata;

	const hey_state_t* state = hey_get_state(ctx);
	if (state->num_chars == 0) { return HEY_CONTINUE; }

	char last_char = state->text[state->num_chars - 1];
	if (
		last_char == '\n'
		|| last_char == '.'
		|| last_char == '?'
		|| last_char == '!'
	) {
		// Remove the last char
		*count = 1;
		return HEY_STOP_AND_RETRACT_CHARACTERS;
	} else {
		return HEY_CONTINUE;
	}
}

static void
knockknock(hey_exec_t* ctx, void* userdata) {
	(void)userdata;

	hey_dsl(ctx) {
		h_lit_special(
			"<s>### Instruction:\n"
			"Write a conversation between two characters A and B. A tells B a hillarious knock knock joke.\n"
			"\n"
			"### Response:\n"
			"A: Knock knock.\n"
			"B: Who's there?\n"
			"A: "
		);

		// Generate the setup
		hey_var_t who;
		h_generate(
			.controller = { .fn = ends_at_punctuation },
			.capture_into = &who,
		);

		// Reformat the setup line so that it always end with an exclamation mark.
		hey_str_t who_str = hey_get_var(ctx, who);
		h_fmt(
			"!\n"
			"B: %.*s who?\n"
			"A: ",
			who_str.length, who_str.chars
		);

		// Generate the punchline
		hey_var_t punchline;
		h_generate(
			.controller = { .fn = ends_at_punctuation },
			.capture_into = &punchline,
		);
		hey_str_t punchline_str = hey_get_var(ctx, punchline);

		hey_term_put(stderr, ANSI_CODE_RESET);
		fprintf(stderr, "\n------------\n");
		fprintf(stderr, "Who: |%.*s|\n", who_str.length, who_str.chars);
		fprintf(stderr, "Punchline: |%.*s|\n", punchline_str.length, punchline_str.chars);
	}
}

int
main(int argc, const char* argv[]) {
	return example_main(argc, argv, knockknock);
}
