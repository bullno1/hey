#ifndef HEY_CHOOSE_H
#define HEY_CHOOSE_H

#include "hey.h"

HEY_API hey_index_t
hey_choose(hey_exec_t* ctx, const hey_str_t options[]);

#endif

#ifdef HEY_CHOOSE_IMPLEMENTATION

#define HEY_CHOOSE_PRIVATE static inline

typedef struct hey_choose_state_s {
	hey_index_t choice;
	hey_index_t initial_num_chars;
	hey_index_t initial_healing_len;
	const hey_str_t* options;
} hey_choose_state_t;

HEY_CHOOSE_PRIVATE hey_control_decision_t
hey_choose_controller(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	hey_choose_state_t* choose_state = userdata;
	const hey_state_t* hey_state = hey_get_state(ctx);

	hey_str_t ctx_str = {
		.chars = hey_state->text
			+ choose_state->initial_num_chars
			+ choose_state->initial_healing_len,
		.length = hey_state->num_chars
			- choose_state->initial_num_chars
			- choose_state->initial_healing_len
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

HEY_CHOOSE_PRIVATE void
hey_choose_logit_processor(hey_logit_t* logits, hey_token_t num_logits, hey_exec_t* ctx, void* userdata) {
	hey_choose_state_t* choose_state = userdata;
	const hey_state_t* hey_state = hey_get_state(ctx);
	hey_index_t offset = hey_state->num_chars - choose_state->initial_num_chars;

	// The positioning can be one of the following cases:
	//
	// [healing prefix][options]
	// [ctx][token]                 : Within healing prefix
	// [ctx      ][token]           : Partially in healing prefix
	// [ctx            ][token]     : Within options
	// [ctx               ][token]  : Overshoot options
	// [ctx       ][token        ]  : Cover options and prefix

	for (hey_token_t token = 0; token < num_logits; ++token) {
		if (logits[token] == HEY_LOGIT_IGNORE) { continue; }

		hey_str_t token_str = hey_detokenize(ctx, token);
		if (offset + token_str.length <= choose_state->initial_healing_len) { continue; }

		// Position is relative to when hey_choose is first called
		hey_span_t token_span = {
			.begin = offset,
			.end = offset + token_str.length,
		};

		bool matched = false;
		for (hey_index_t i = 0; choose_state->options[i].length > 0; ++i) {
			hey_str_t option = choose_state->options[i];
			hey_span_t option_span = {
				.begin = choose_state->initial_healing_len,
				.end = choose_state->initial_healing_len + option.length,
			};

			hey_span_t cmp_span = {
				.begin = HEY_MAX(token_span.begin, option_span.begin),
				.end = HEY_MIN(token_span.end, option_span.end),
			};

			hey_str_t token_cmp_str = token_str;
			if (cmp_span.begin > token_span.begin) {
				token_cmp_str.chars += cmp_span.begin - token_span.begin;
				token_cmp_str.length -= cmp_span.begin - token_span.begin;
			}
			if (cmp_span.end < token_span.end) {
				token_cmp_str.length -= token_span.end - cmp_span.end;
			}

			hey_str_t option_cmp_str = option;
			if (cmp_span.begin > option_span.begin) {
				option_cmp_str.chars += cmp_span.begin - option_span.begin;
				option_cmp_str.length -= cmp_span.begin - option_span.begin;
			}
			if (cmp_span.end < option_span.end) {
				option_cmp_str.length -= option_span.end - cmp_span.end;
			}

			if (
				HEY_STRNCMP(
					token_cmp_str.chars, option_cmp_str.chars,
					token_cmp_str.length
				) == 0
			) {
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
		.initial_num_chars = hey_state->num_chars,
		.initial_healing_len = hey_state->healing_prefix.length,
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
