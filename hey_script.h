#ifndef HEY_SCRIPT_H
#define HEY_SCRIPT_H

#include "hey.h"
#include "hey_choose.h"
#include "hey_suffix.h"

typedef struct hey_script_arg_parser_s {
	hey_str_t (*fn)(hey_exec_t* ctx, void* userdata);
	void* userdata;
} hey_script_arg_parser_t;

typedef struct hey_script_step_s {
	hey_str_t description;
	const struct hey_script_action_def_s* action;
	hey_str_t* args;
} hey_script_step_t;

typedef struct hey_script_arg_def_s {
	hey_str_t name;
	hey_str_t description;
	hey_str_t example;
	hey_script_arg_parser_t parser;
} hey_script_arg_def_t;

typedef struct hey_script_action_def_s {
	hey_str_t name;
	hey_str_t description;
	const hey_script_arg_def_t* args;
	hey_str_t example_description;
} hey_script_action_def_t;

typedef struct hey_script_receiver_s {
	void (*fn)(const hey_script_step_t* step, void* userdata);
	void* userdata;
} hey_script_receiver_t;

HEY_API hey_script_arg_parser_t hey_script_string_parser;

HEY_API hey_script_arg_parser_t hey_script_number_parser;

HEY_API void
hey_script_generate(
	hey_exec_t* ctx,
	hey_script_receiver_t receiver,
	hey_str_t stop_at,
	const hey_script_action_def_t* actions
);

HEY_API void
hey_script_push_example(
	hey_exec_t* ctx,
	const hey_script_action_def_t* action
);

#endif

#if defined(HEY_IMPLEMENTATION) && !defined(HEY_SCRIPT_IMPLEMENTATION)
#define HEY_SCRIPT_IMPLEMENTATION

HEY_PRIVATE void
hey_script_keep_digit(
	hey_logit_t* logits, hey_token_t num_logits,
	hey_exec_t* ctx,
	void* userdata
) {
	(void)userdata;

	const hey_state_t* state = hey_get_state(ctx);
	for (hey_token_t token = 0; token < num_logits; ++token) {
		hey_str_t token_str = hey_detokenize(ctx, token);

		for (hey_index_t i = state->healing_prefix.length; i < token_str.length; ++i) {
			char ch = token_str.chars[i];
			if (!('0' <= ch && ch <= '9')) {
				logits[token] = HEY_LOGIT_IGNORE;
				break;
			}
		}
	}
}

HEY_PRIVATE hey_str_t
hey_script_parse_string(hey_exec_t* ctx, void* userdata) {
	(void)userdata;

	const hey_llm_t* llm = hey_get_llm(ctx);
	hey_var_t string;
	hey_generate(ctx, (hey_generate_options_t){
		.controller = hey_ends_at_token(llm->nl),
		.capture_into = &string,
	});

	// Exclude the new line
	string.text.end -= 1;
	string.tokens.end -= 1;
	return hey_get_var(ctx, string);
}

HEY_PRIVATE hey_str_t
hey_script_parse_number(hey_exec_t* ctx, void* userdata) {
	(void)userdata;

	const hey_llm_t* llm = hey_get_llm(ctx);
	hey_var_t number;
	hey_generate(ctx, (hey_generate_options_t){
		.controller = hey_ends_at_token(llm->nl),
		.logit_processor = { .fn = hey_script_keep_digit },
		.capture_into = &number,
	});

	return hey_get_var(ctx, number);
}

hey_script_arg_parser_t hey_script_number_parser = {
	.fn = hey_script_parse_number,
};

hey_script_arg_parser_t hey_script_string_parser = {
	.fn = hey_script_parse_string,
};

void
hey_script_generate(
	hey_exec_t* ctx,
	hey_script_receiver_t receiver,
	hey_str_t stop_at,
	const hey_script_action_def_t* actions
) {
	hey_index_t num_actions;
	hey_index_t max_num_args = 0;
	for (
		num_actions = 0;
		actions[num_actions].name.chars != NULL;
		++num_actions
	) {
		const hey_script_action_def_t* action = &actions[num_actions];
		hey_index_t num_args = 0;
		while (action->args[num_args].name.chars != NULL) {
			++num_args;
		}

		max_num_args = HEY_MAX(max_num_args, num_args);
	}

	hey_str_t* action_names = hey_malloc(ctx, sizeof(hey_str_t) * num_actions + 1);
	for (hey_index_t i = 0; i < num_actions; ++i) {
		action_names[i] = actions[i].name;
	}
	action_names[num_actions] = (hey_str_t){ 0 };

	hey_str_t* args = hey_malloc(ctx, sizeof(hey_str_t) * max_num_args);

	const hey_llm_t* llm = hey_get_llm(ctx);
	while (true) {
		hey_index_t action_or_quit = hey_choose(
			ctx, HEY_ARRAY(hey_str_t,
				HEY_STR("#"),
				stop_at
			)
		);

		if (action_or_quit == 1) { break; }

		hey_push_str(ctx, HEY_STR(" "), false);
		hey_var_t description;
		hey_generate(ctx, (hey_generate_options_t){
			.controller = hey_ends_at_token(llm->nl),
			.capture_into = &description,
		});

		hey_push_str(ctx, HEY_STR("- action: "), false);
		hey_index_t action_index = hey_choose(ctx, action_names);
		hey_push_tokens(ctx, &llm->nl, 1);

		const hey_script_action_def_t* action = &actions[action_index];
		hey_index_t arg_index;
		for (
			arg_index = 0;
			action->args[arg_index].name.chars != NULL;
			++arg_index
		) {
			const hey_script_arg_def_t* arg = &action->args[arg_index];
			hey_push_str_fmt(ctx, false, "  %.*s: ", arg->name.length, arg->name.chars);
			args[arg_index] = arg->parser.fn(ctx, arg->parser.userdata);
		}

		if (receiver.fn != NULL) {
			receiver.fn(
				&(hey_script_step_t){
					.description = hey_get_var(ctx, description),
					.action = action,
					.args = args,
				},
				receiver.userdata
			);
		}
	}
}

void
hey_script_push_example(
	hey_exec_t* ctx,
	const hey_script_action_def_t* action
) {
	hey_push_str_fmt(
		ctx, false,
		"# %.*s\n"
		"- action: %.*s\n",
		action->example_description.length, action->example_description.chars,
		action->name.length, action->name.chars
	);

	for (hey_index_t i = 0; action->args[i].parser.fn != NULL; ++i) {
		const hey_script_arg_def_t* arg = &action->args[i];

		hey_push_str_fmt(
			ctx, false,
			"  %.*s: %.*s\n",
			arg->name.length, arg->name.chars,
			arg->example.length, arg->example.chars
		);
	}
}

#endif
