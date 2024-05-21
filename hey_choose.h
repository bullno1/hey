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

#if 0
	printf("\n---\n");
	printf("Context: {%.*s}\n", ctx_str.length, ctx_str.chars);
	printf("---\n");
#endif

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

	hey_str_t prefix = {
		.chars = hey_state->text + choose_state->placement_offset,
		.length = hey_state->num_chars - choose_state->placement_offset,
	};

	for (hey_token_t token = 0; token < num_logits; ++token) {
		if (logits[token] == HEY_LOGIT_IGNORE) { continue; }

		hey_str_t token_str = hey_detokenize(ctx, token);

		// Skip 0-length token or the loop will never progress
		if (token_str.length == 0) {
			logits[token] = HEY_LOGIT_IGNORE;
			continue;
		}

		// Make sure we get past the healing prefix
		token_str.chars += hey_state->healing_prefix.length;
		token_str.length -= hey_state->healing_prefix.length;
		if (token_str.length <= 0) { continue; }

		// Check whether the token can satisfy at least one choice
		bool matched = false;
		for (hey_index_t i = 0; choose_state->options[i].chars != NULL; ++i) {
			hey_str_t option = choose_state->options[i];
			hey_str_t option_suffix = option;

			if (hey_state->num_chars > choose_state->placement_offset) {
				// Skip options which does not match the prefix
				hey_index_t cmp_len = HEY_MIN(prefix.length, option.length);
				if (HEY_STRNCMP(option.chars, prefix.chars, cmp_len) != 0) {
					continue;
				}

				// Skip options which cannot get past the placement offset
				option_suffix.length -= hey_state->num_chars - choose_state->placement_offset;

				if (option_suffix.length <= 0) {
					continue;
				}
				option_suffix.chars += hey_state->num_chars - choose_state->placement_offset;
			}

			// Skip tokens which is shorter than token
			if (token_str.length > option.length) {
				continue;
			}

			hey_index_t cmp_len = HEY_MIN(option_suffix.length, token_str.length);
			if (HEY_STRNCMP(option_suffix.chars, token_str.chars, cmp_len) == 0) {
#if 0
				printf("\n---\n");
				printf("Token: {%.*s}\n", token_str.length, token_str.chars);
				printf("Option: {%.*s}\n", option.length, option.chars);
				printf("Option suffix: {%.*s}\n", option_suffix.length, option_suffix.chars);
				printf("---\n");
#endif

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

	// TODO: Initialize placement offset on first controller call
	// Healing is not initialized until hey_generate is called
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
