#include "hey.h"
#include "hey_one_of.h"
#define HEY_IMPLEMENTATION
#define SOKOL_IMPL
#include "common.h"
#include <hey_suffix.h>
#include <hey_one_of.h>

hey_control_decision_t ends_at_punctuation(
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

	hey_push_str(
		ctx,
		HEY_STR(
			"<s>### Instruction:\n"
			"Write a conversation between two characters A and B. A tells B a hillarious knock knock joke.\n"
			"\n"
			"### Response:\n"
			"A: Knock knock.\n"
			"B: Who's there?\n"
			"A: "
		),
		true
	);

	hey_var_t who;
	hey_generate(ctx, (hey_generate_options_t){
		.controller = { .fn = ends_at_punctuation },
		.capture_into = &who,
	});

	hey_str_t who_str = hey_get_var(ctx, who);
	hey_push_str_fmt(
		ctx, false,
		"!\n"
		"B: %.*s who?\n"
		"A: ",
		who_str.length, who_str.chars
	);

	hey_var_t punchline;
	hey_generate(ctx, (hey_generate_options_t){
		.controller = { .fn = ends_at_punctuation },
		.capture_into = &punchline,
	});
	hey_str_t punchline_str = hey_get_var(ctx, punchline);

	hey_term_put(stderr, ANSI_CODE_RESET);
	fprintf(stderr, "\n------------\n");
	fprintf(stderr, "Who: |%.*s|\n", who_str.length, who_str.chars);
	fprintf(stderr, "Punchline: |%.*s|\n", punchline_str.length, punchline_str.chars);
}

int main(int argc, const char* argv[]) {
	return example_main(argc, argv, knockknock);
}
