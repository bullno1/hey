#ifndef HEY_SUFFIX_H
#define HEY_SUFFIX_H

#include "hey.h"

HEY_API hey_controller_t
hey_ends_at_suffix(const hey_str_t* suffix);

HEY_API hey_controller_t
hey_ends_at_isuffix(const hey_str_t* suffix);

HEY_API hey_controller_t
hey_ends_at_token(hey_token_t token);

#endif

#ifdef HEY_IMPLEMENTATION

#include <stdint.h>

#define HEY_SUFFIX_PRIVATE static inline

HEY_SUFFIX_PRIVATE hey_control_decision_t
hey_ends_at_suffix_controller(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	const hey_str_t* suffix = userdata;
	const hey_state_t* state = hey_get_state(ctx);
	if (state->num_chars < suffix->length) { return HEY_CONTINUE; }

	if (
		HEY_STRNCMP(
			suffix->chars,
			state->text + state->num_chars - suffix->length,
			suffix->length
		) == 0
	) {
		return HEY_STOP;
	} else {
		return HEY_CONTINUE;
	}
}

HEY_SUFFIX_PRIVATE hey_control_decision_t
hey_ends_at_isuffix_controller(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	const hey_str_t* suffix = userdata;
	const hey_state_t* state = hey_get_state(ctx);
	if (state->num_chars < suffix->length) { return HEY_CONTINUE; }

	if (
		HEY_STRNICMP(
			suffix->chars,
			state->text + state->num_chars - suffix->length,
			suffix->length
		) == 0
	) {
		return HEY_STOP;
	} else {
		return HEY_CONTINUE;
	}
}

HEY_SUFFIX_PRIVATE hey_control_decision_t
hey_ends_at_token_controller(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	hey_token_t token = (hey_token_t)(uintptr_t)userdata;
	const hey_state_t* state = hey_get_state(ctx);

	if (state->num_tokens == 0) { return HEY_CONTINUE; }

	return state->tokens[state->num_tokens - 1] == token ? HEY_STOP : HEY_CONTINUE;
}

hey_controller_t
hey_ends_at_suffix(const hey_str_t* suffix) {
	return (hey_controller_t){
		.userdata = (void*)suffix,
		.fn = hey_ends_at_suffix_controller,
	};
}

hey_controller_t
hey_ends_at_isuffix(const hey_str_t* suffix) {
	return (hey_controller_t){
		.userdata = (void*)suffix,
		.fn = hey_ends_at_isuffix_controller,
	};
}

hey_controller_t
hey_ends_at_token(hey_token_t token) {
	HEY_ASSERT(sizeof(token) <= sizeof(void*), "Token is too big");

	return (hey_controller_t){
		.userdata = (void*)(uintptr_t)token,
		.fn = hey_ends_at_token_controller,
	};
}

#endif
