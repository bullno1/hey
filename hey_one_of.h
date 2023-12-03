#ifndef HEY_ONE_OF_H
#define HEY_ONE_OF_H

#include "hey.h"

HEY_API hey_controller_t
hey_one_of(const hey_controller_t controllers[]);

#endif

#ifdef HEY_ONE_OF_IMPLEMENTATION

#define HEY_ONE_OF_PRIVATE static inline

HEY_ONE_OF_PRIVATE hey_control_decision_t
hey_one_of_controller(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	const hey_controller_t* controllers = userdata;

	for (hey_index_t i = 0; controllers[i].fn != NULL; ++i) {
		const hey_controller_t* controller = &controllers[i];

		hey_control_decision_t decision = controller->fn(
			count, ctx, controller->userdata
		);

		if (decision != HEY_CONTINUE) {
			return decision;
		}
	}

	return HEY_CONTINUE;
}

hey_controller_t
hey_one_of(const hey_controller_t controllers[]) {
	return (hey_controller_t){
		.userdata = (void*)controllers,
		.fn = hey_one_of_controller,
	};
}

#endif
