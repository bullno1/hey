#define HEY_IMPLEMENTATION
#define SOKOL_IMPL
#include "common.h"
#include <hey_suffix.h>

static void
generate(hey_exec_t* ctx, void* userdata) {
	const exec_input_t* input = userdata;
	const hey_llm_t* llm = hey_get_llm(ctx);

	hey_push_str(ctx, input->input_string, true);

	hey_var_t answer;
	hey_generate(ctx, (hey_generate_options_t){
		.controller = hey_ends_at_token(llm->eos),
		.capture_into = &answer,
	});

	hey_str_t capture = hey_get_var(ctx, answer);

	hey_term_put(stderr, ANSI_CODE_RESET);
	fprintf(stderr, "\n------------\n");
	fprintf(stderr, "Capture span: [%d, %d)\n", answer.text.begin, answer.text.end);
	fprintf(stderr, "Capture: |%.*s|\n", capture.length, capture.chars);
}

int
main(int argc, const char* argv[]) {
	return example_main(argc, argv, generate);
}
