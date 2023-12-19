#ifndef HEY_ONE_OF_H
#define HEY_ONE_OF_H

#include "hey.h"

typedef struct hey_one_of_s {
	hey_index_t* index;
	const hey_controller_t* controllers;
} hey_one_of_t;

HEY_API hey_controller_t
hey_one_of(hey_one_of_t* oneof);

#ifdef HEY_DSL

#define h_one_of(...) \
	hey_one_of(&(hey_one_of_t){ \
		.index = &h_scope().last_index, \
		.controllers = HEY_ARRAY(hey_controller_t, __VA_ARGS__), \
	})

#define h_one_of_reason() h_scope().last_index

#endif

#endif

#if defined(HEY_IMPLEMENTATION) && !defined(HEY_ONE_OF_IMPLEMENTATION)
#define HEY_ONE_OF_IMPLEMENTATION

HEY_PRIVATE hey_control_decision_t
hey_one_of_controller(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	const hey_one_of_t* oneof = userdata;

	for (hey_index_t i = 0; oneof->controllers[i].fn != NULL; ++i) {
		const hey_controller_t* controller = &oneof->controllers[i];

		hey_control_decision_t decision = controller->fn(
			count, ctx, controller->userdata
		);

		if (decision != HEY_CONTINUE) {
			if (oneof->index != NULL) {
				*(oneof->index) = i;
			}

			return decision;
		}
	}

	return HEY_CONTINUE;
}

hey_controller_t
hey_one_of(hey_one_of_t* oneof) {
	return (hey_controller_t){
		.userdata = oneof,
		.fn = hey_one_of_controller,
	};
}

#endif
