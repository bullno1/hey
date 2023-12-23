#ifndef HEY_CHOOSE_H
#define HEY_CHOOSE_H

#include "hey.h"

HEY_API hey_index_t
hey_choose(hey_exec_t* ctx, const hey_str_t options[]);

#ifdef HEY_DSL
#define	h_choose(...) hey_choose(h_ctx(), HEY_ARRAY(hey_str_t, __VA_ARGS__))
#endif

#endif

#if defined(HEY_IMPLEMENTATION) && !defined(HEY_CHOOSE_IMPLEMENTATION)
#define HEY_CHOOSE_IMPLEMENTATION

typedef struct hey_choose_state_s {
	hey_index_t choice;
	hey_index_t placement_offset;
	const hey_str_t* options;
} hey_choose_state_t;

HEY_PRIVATE hey_control_decision_t
hey_choose_controller(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	hey_choose_state_t* choose_state = userdata;
	const hey_state_t* hey_state = hey_get_state(ctx);

	hey_str_t ctx_str = {
		.chars = hey_state->text + choose_state->placement_offset,
		.length = hey_state->num_chars - choose_state->placement_offset,
	};
	if (ctx_str.length < 0) { return HEY_CONTINUE; }

	for (hey_index_t i = 0; choose_state->options[i].length > 0; ++i) {
		hey_str_t option = choose_state->options[i];

		if (ctx_str.length < option.length) { continue; }

		if (HEY_STRNCMP(ctx_str.chars, option.chars, option.length) == 0) {
			choose_state->choice = i;
			*count = ctx_str.length - option.length;
			return HEY_STOP_AND_RETRACT_CHARACTERS;
		}
	}

	return HEY_CONTINUE;
}

HEY_PRIVATE void
hey_choose_logit_processor(
	hey_logit_t* logits, hey_token_t num_logits,
	hey_exec_t* ctx,
	void* userdata
) {
	hey_choose_state_t* choose_state = userdata;
	const hey_state_t* hey_state = hey_get_state(ctx);

	for (hey_token_t token = 0; token < num_logits; ++token) {
		if (logits[token] == HEY_LOGIT_IGNORE) { continue; }

		hey_str_t token_str = hey_detokenize(ctx, token);

		// Make sure we get past the healing prefix
		token_str.chars += hey_state->healing_prefix.length;
		token_str.length -= hey_state->healing_prefix.length;
		if (token_str.length <= 0) { continue; }

		// Check whether the token can satisfy at least one choice
		bool matched = false;
		for (hey_index_t i = 0; choose_state->options[i].chars != NULL; ++i) {
			hey_str_t option = choose_state->options[i];

			// It's safe to modify since this is a copy
			if (hey_state->num_chars > choose_state->placement_offset) {
				option.chars += hey_state->num_chars - choose_state->placement_offset;
				option.length -= hey_state->num_chars - choose_state->placement_offset;

				if (option.length <= 0) { continue; }
			}

			hey_index_t cmp_len = HEY_MIN(option.length, token_str.length);
			if (HEY_STRNCMP(option.chars, token_str.chars, cmp_len) == 0) {
				matched = true;
				break;
			}
		}

		if (!matched) {
			logits[token] = HEY_LOGIT_IGNORE;
		}
	}
}

hey_index_t
hey_choose(hey_exec_t* ctx, const hey_str_t* options) {
	const hey_state_t* hey_state = hey_get_state(ctx);

	hey_choose_state_t choose_state = {
		.choice = -1,
		.options = options,
		.placement_offset = hey_state->num_chars + hey_state->healing_prefix.length
	};

	hey_generate(
		ctx,
		(hey_generate_options_t){
			.controller = {
				.fn = hey_choose_controller,
				.userdata = &choose_state,
			},
			.logit_processor = {
				.fn = hey_choose_logit_processor,
				.userdata = &choose_state,
			},
		}
	);

	return choose_state.choice;
}

#endif
