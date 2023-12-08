#ifndef HEY_COMPOSITE_LOGIT_PROCESSOR
#define HEY_COMPOSITE_LOGIT_PROCESSOR

#include "hey.h"

HEY_API hey_logit_processor_t
hey_make_composite_logit_processor(const hey_logit_processor_t processors[]);

#endif

#ifdef HEY_IMPLEMENTATION

HEY_PRIVATE void
hey_composite_logit_processor(hey_logit_t* logits, hey_token_t num_logits, hey_exec_t* ctx, void* userdata) {
	const hey_logit_processor_t* processors = userdata;
	for (hey_index_t i = 0; processors[i].fn != NULL; ++i) {
		processors[i].fn(logits, num_logits, ctx, processors[i].userdata);
	}
}

hey_logit_processor_t
hey_make_composite_logit_processor(const hey_logit_processor_t processors[]) {
	return (hey_logit_processor_t){
		.fn = hey_composite_logit_processor,
		.userdata = (void*)processors,
	};
}

#endif
