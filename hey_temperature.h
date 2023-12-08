#ifndef HEY_TEMPERATURE_H
#define HEY_TEMPERATURE_H

#include "hey.h"

HEY_API hey_logit_processor_t
hey_make_temperature_logit_processor(hey_logit_t temperature);

#endif

#ifdef HEY_IMPLEMENTATION

HEY_PRIVATE void
hey_temperature_logit_processor(hey_logit_t* logits, hey_token_t num_logits, hey_exec_t* ctx, void* userdata) {
	hey_logit_t temperature;
	HEY_MEMCPY(&temperature, &userdata, sizeof(hey_logit_t));

	for (hey_index_t i = 0; i < num_logits; ++i) {
		logits[i] /= temperature;
	}
}

hey_logit_processor_t
hey_make_temperature_logit_processor(hey_logit_t temperature) {
	hey_logit_processor_t processor = { .fn = hey_temperature_logit_processor };
	HEY_MEMCPY(&processor.userdata, &temperature, sizeof(hey_logit_t));
	return processor;
}

#endif
