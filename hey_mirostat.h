#ifndef HEY_MIROSTAT_H
#define HEY_MIROSTAT_H

#include "hey_sampling.h"

HEY_API hey_sampler_t
hey_make_mirostat_sampler(
	hey_exec_t* ctx,
	hey_rand_state_t* rand_state,
	hey_logit_t tau,
	hey_logit_t eta,
	hey_logit_t mu
);

#endif

#ifdef HEY_IMPLEMENTATION

typedef struct {
	hey_rand_state_t* rand_state;
	hey_logit_t tau;
	hey_logit_t eta;
	hey_logit_t mu;
	hey_logit_t softmax[];
} hey_mirostat_sampler_state_t;

HEY_PRIVATE hey_token_t
hey_sample_mirostat(const hey_logit_t* logits, hey_token_t num_logits, hey_exec_t* ctx, void* userdata) {
	hey_mirostat_sampler_state_t* state = userdata;
	hey_softmax(logits, num_logits, state->softmax);

	hey_logit_t mu = state->mu;
	hey_token_t num_items_left = num_logits;
	for (hey_token_t i = 0; i < num_logits; ++i) {
		hey_logit_t surprise = -log2(state->softmax[i]);

		if (surprise > mu && num_items_left > 1) {
			--num_items_left;
			state->softmax[i] = HEY_LOGIT_IGNORE;
		} else {
			state->softmax[i] = logits[i];
		}
	}

	hey_softmax(state->softmax, num_logits, state->softmax);

	hey_token_t token = hey_sampling_pick_random(
		state->softmax, num_logits, state->rand_state
	);

	hey_logit_t observed_surprise = -log2f(state->softmax[token]);
	hey_logit_t error = observed_surprise - state->tau;
	state->mu = mu - state->eta * error;

	return token;
}

hey_sampler_t
hey_make_mirostat_sampler(
	hey_exec_t* ctx,
	hey_rand_state_t* rand_state,
	hey_logit_t tau,
	hey_logit_t eta,
	hey_logit_t mu
) {
	const hey_llm_t* llm = hey_get_llm(ctx);

	hey_mirostat_sampler_state_t* state = hey_malloc(
		ctx,
		sizeof(hey_mirostat_sampler_state_t) + sizeof(hey_logit_t) * llm->vocab_size
	);
	state->rand_state = rand_state;
	state->tau = tau;
	state->eta = eta;
	state->mu = mu;

	return (hey_sampler_t){
		.fn = hey_sample_mirostat,
		.userdata = state,
	};
}

#endif
