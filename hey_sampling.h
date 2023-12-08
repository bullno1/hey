#ifndef HEY_SAMPLING_H
#define HEY_SAMPLING_H

#include "hey.h"

// The default implementation is just stdlib's rand which is not good.
// There is only a single global state.
// For a much better alternative, consider: https://github.com/mattiasgustavsson/libs/blob/main/docs/rnd.md
#ifndef HEY_RAND
#include <stdlib.h>
#define HEY_RAND_OUT_TYPE int
#define HEY_RAND_STATE_TYPE char
#define HEY_RAND_INIT(STATE, SEED) srand(SEED)
#define HEY_RAND_NEXT(STATE) rand()
#define HEY_RAND_MAX RAND_MAX
#endif

typedef HEY_RAND_OUT_TYPE hey_rand_out_t;
typedef HEY_RAND_STATE_TYPE hey_rand_state_t;

HEY_API hey_sampler_t
hey_make_argmax_sampler(void);

HEY_API hey_sampler_t
hey_make_random_sampler(hey_exec_t* ctx, hey_rand_state_t* rand_state);

#endif

#ifdef HEY_IMPLEMENTATION

#include <tgmath.h>

typedef struct hey_random_sampler_state_s {
	hey_rand_state_t* rand_state;
	hey_logit_t softmax[];
} hey_random_sampler_state_t;

HEY_PRIVATE void
hey_softmax(const hey_logit_t* input, hey_token_t num_items, hey_logit_t* output) {
	hey_logit_t max_score = -INFINITY;
	for (hey_token_t i = 0; i < num_items; ++i) {
		max_score = HEY_MAX(input[i], max_score);
	}

	hey_logit_t sum = 0;
	for (hey_token_t i = 0; i < num_items; ++i) {
		hey_logit_t p = exp(input[i] - max_score);
		sum += p;
		output[i] = p;
	}

	for (hey_token_t i = 0; i < num_items; ++i) {
		output[i] /= sum;
	}
}

HEY_PRIVATE hey_token_t
hey_sample_random(const hey_logit_t* logits, hey_token_t num_logits, hey_exec_t* ctx, void* userdata) {
	hey_random_sampler_state_t* state = userdata;

	hey_softmax(logits, num_logits, state->softmax);

	hey_logit_t sum = 0;
	for (hey_token_t i = 0; i < num_logits; ++i) {
		sum += state->softmax[i];
	}

	hey_rand_out_t rnd = HEY_RAND_NEXT(&state->rand_state);
	hey_logit_t threshold = ((hey_logit_t)rnd / (hey_logit_t)HEY_RAND_MAX) * sum;

	for (hey_token_t i = 0; i < num_logits; ++i) {
		threshold -= state->softmax[i];

		if (threshold <= 0) {
			return i;
		}
	}

	return 0;
}

hey_sampler_t
hey_make_argmax_sampler(void) {
	return (hey_sampler_t){
		.fn = hey_sample_argmax,
	};
}

hey_sampler_t
hey_make_random_sampler(hey_exec_t* ctx, hey_rand_state_t* rand_state) {
	const hey_llm_t* llm = hey_get_llm(ctx);

	hey_random_sampler_state_t* state = hey_malloc(
		ctx,
		sizeof(hey_random_sampler_state_t) + sizeof(hey_logit_t) * llm->vocab_size
	);
	state->rand_state = rand_state;

	return (hey_sampler_t){
		.fn = hey_sample_random,
		.userdata = state,
	};
}

#endif
