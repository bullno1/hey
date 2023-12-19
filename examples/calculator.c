#define HEY_IMPLEMENTATION
#define SOKOL_IMPL
#include <hey.h>
#include <hey_suffix.h>
#include <hey_one_of.h>
#include <expr.h>
#include <tgmath.h>
#include "common.h"

static void
calculator(hey_exec_t* ctx, void* userdata) {
	(void)userdata;

	const exec_input_t* input = userdata;
	const hey_llm_t* llm = hey_get_llm(ctx);

	hey_push_str(ctx, input->input_string, true);

	hey_str_t begin_calc = HEY_STR("<<");
	hey_str_t end_calc = HEY_STR(">>");
	hey_str_t final_answer = HEY_STR("===");

	hey_var_t answer;

	while (true) {
		// Stop at either beign_calc or final_answer
		hey_index_t index;
		hey_generate(ctx, (hey_generate_options_t){
			.controller = hey_one_of(&(hey_one_of_t){
				.controllers = HEY_ARRAY(hey_controller_t,
					hey_ends_at_suffix(&begin_calc),
					hey_ends_at_suffix(&final_answer),
					hey_ends_at_token(llm->bos)
				),
				.index = &index,
			}),
		});

		if (index == 0) { // Begin calc
			hey_var_t expr;
			hey_generate(ctx, (hey_generate_options_t){
				.controller = hey_ends_at_suffix(&end_calc),
				.capture_into = &expr,
			});

			hey_str_t expr_str = hey_get_var(ctx, expr);
			expr_str.length -= 2; // Eliminate >>

			// Evaluate expression
			struct expr* parsed_expr = expr_create(
				expr_str.chars,
				expr_str.length,
				(struct expr_var_list[]) { 0 },
				(struct expr_func[]) { { 0 } }
			);
			if (parsed_expr == NULL) {
				hey_term_put(stderr, ANSI_CODE_RESET);
				fprintf(stderr, "\nInvalid expression: %.*s\n", expr_str.length, expr_str.chars);
				return;
			}

			float result = expr_eval(parsed_expr);
			expr_destroy(parsed_expr, (struct expr_var_list[]) { 0 });

			// Inject result
			if (round(result) == result) {
				hey_push_str_fmt(ctx, false, " = %d", (int)result);
			} else {
				hey_push_str_fmt(ctx, false, " = %f", result);
			}
		} else if (index == 1) { // Final answer
			// The space before the final answer
			hey_push_str(ctx, HEY_STR(" "), false);
			hey_generate(ctx, (hey_generate_options_t){
				.controller = hey_ends_at_token(llm->eos),
				.capture_into = &answer,
			});
			break;
		} else if (index == 2) { // Eos
			hey_term_put(stderr, ANSI_CODE_RESET);
			fprintf(stderr, "\neos emitted before final answer!\n");
			return;
		}
	}

	hey_str_t answer_str = hey_get_var(ctx, answer);
	hey_term_put(stderr, ANSI_CODE_RESET);
	fprintf(stderr, "\n------------\n");
	fprintf(stderr, "Answer: |%.*s|\n", answer_str.length, answer_str.chars);
}

int
main(int argc, const char* argv[]) {
	return example_main(argc, argv, calculator);
}
