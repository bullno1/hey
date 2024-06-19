#ifndef HEY_REPETITION_PENALTY_H
#define HEY_REPETITION_PENALTY_H

#include "hey.h"

HEY_API hey_logit_processor_t
hey_make_repetition_penalty_logit_processor(
	hey_exec_t* ctx,
	hey_index_t num_tokens,
	hey_logit_t repetition_penalty,
	hey_logit_t frequency_penalty,
	hey_logit_t presence_penalty
);

#endif

#ifdef HEY_IMPLEMENTATION

#define HEY_TOKEN_INVALID (-1)
#define HEY_TOKEN_DELETED (-2)

typedef struct {
	hey_index_t size;
	hey_index_t hash_exp;
	hey_index_t previous_num_tokens;
	hey_index_t insert_index;
	hey_logit_t repetition_penalty;
	hey_logit_t frequency_penalty;
	hey_logit_t presence_penalty;
	hey_token_t* recent_tokens;
	hey_token_t* hash_keys;
	hey_index_t* hash_values;
} hey_repetition_penalty_state_t;

#ifdef _MSC_VER

#include <intrin.h>

HEY_PRIVATE uint32_t
hey_ilog2(uint32_t x) {
	uint32_t result;
	_BitScanReverse(&result, x);
	return result;
}

#else

#include <limits.h>

HEY_PRIVATE uint32_t
hey_ilog2(uint32_t x) {
    return sizeof(uint32_t) * CHAR_BIT - __builtin_clz(x) - 1;
}

#endif

HEY_PRIVATE uint64_t
hey_hash_token(hey_token_t token) {
	// https://lemire.me/blog/2018/08/15/fast-strongly-universal-64-bit-hashing-everywhere/
	uint64_t h = token;
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdL;
	h ^= h >> 33;
	h *= 0xc4ceb9fe1a85ec53L;
	h ^= h >> 33;
	return h;
}

HEY_PRIVATE int32_t
hey_msi(uint64_t hash, int32_t exp, int32_t idx) {
    uint32_t mask = ((uint32_t)1 << exp) - 1;
    uint32_t step = (hash >> (64 - exp)) | 1;
    return (idx + step) & mask;
}

HEY_PRIVATE hey_index_t
hey_msi_find_slot(
	hey_token_t key,
	hey_index_t exp,
	hey_token_t* keys
) {
	uint64_t hash = hey_hash_token(key);
	hey_index_t i = (hey_index_t)hash;
	hey_index_t grave_slot = -1;

	while (true) {
		i = hey_msi(hash, (int32_t)exp, (int32_t)i);

		if (keys[i] == HEY_TOKEN_INVALID) {
			return grave_slot >= 0 ? grave_slot : i;
		} else if (keys[i] == key) {
			return i;
		} else if (keys[i] == HEY_TOKEN_DELETED && grave_slot < 0) {
			grave_slot = i;
		}
	}
}

HEY_PRIVATE void
hey_ring_buf_reset(hey_repetition_penalty_state_t* state) {
	for (hey_index_t i = 0; i < state->size; ++i) {
		state->recent_tokens[i] = HEY_TOKEN_INVALID;
	}

	hey_index_t hash_size = 1 << state->hash_exp;
	for (hey_index_t i = 0; i < hash_size; ++i) {
		state->hash_keys[i] = HEY_TOKEN_INVALID;
	}

	state->insert_index = 0;
	state->previous_num_tokens = 0;
}

HEY_PRIVATE void
hey_repetition_penalty_logit_processor(
	hey_logit_t* logits, hey_token_t num_logits,
	hey_exec_t* ctx,
	void* userdata
) {
	hey_repetition_penalty_state_t* state = userdata;
	const hey_state_t* hey_state = hey_get_state(ctx);

	hey_index_t previous_num_tokens = state->previous_num_tokens;
	hey_index_t current_num_tokens = hey_state->num_tokens;
	if (current_num_tokens <= previous_num_tokens) {
		// Some rewinding happened, just start from the beginning
		previous_num_tokens = HEY_MIN(0, current_num_tokens - state->size);
		hey_ring_buf_reset(state);
	}

	hey_index_t insert_index = state->insert_index;
	for (hey_index_t i = previous_num_tokens; i < current_num_tokens; ++i) {
		hey_token_t new_token = hey_state->tokens[i];
		hey_token_t old_token = state->recent_tokens[insert_index];
		state->recent_tokens[insert_index] = new_token;
		insert_index = (insert_index + 1) % state->size;

		if (old_token >= 0) {
			hey_index_t old_hash_slot = hey_msi_find_slot(
				old_token, state->hash_exp, state->hash_keys
			);
			if ((--state->hash_values[old_hash_slot]) == 0) {
				state->hash_keys[old_hash_slot] = HEY_TOKEN_DELETED;
			}
		}

		hey_index_t new_hash_slot = hey_msi_find_slot(
			new_token, state->hash_exp, state->hash_keys
		);
		if (state->hash_keys[new_hash_slot] >= 0) {
			++state->hash_values[new_hash_slot];
		} else {
			state->hash_keys[new_hash_slot] = new_token;
			state->hash_values[new_hash_slot] = 1;
		}
	}
	state->previous_num_tokens = current_num_tokens;
	state->insert_index = insert_index;

	hey_logit_t repetition_penalty = state->repetition_penalty;
	hey_logit_t frequency_penalty = state->frequency_penalty;
	hey_logit_t presence_penalty = state->presence_penalty;
	hey_index_t hash_size = 1 << state->hash_exp;
	for (hey_index_t i = 0; i < hash_size; ++i) {
		hey_index_t token = state->hash_keys[i];
		if (token < 0) { continue; }

		hey_logit_t logit = logits[token];
		hey_logit_t score = logit > 0 ? logit / repetition_penalty : logit * repetition_penalty;

		hey_index_t frequency = state->hash_values[i];
		score -= (hey_logit_t)frequency * frequency_penalty;
		score -= (hey_logit_t)(frequency > 0) * presence_penalty;

		logits[token] = score;
	}
}

hey_logit_processor_t
hey_make_repetition_penalty_logit_processor(
	hey_exec_t* ctx,
	hey_index_t num_tokens,
	hey_logit_t repetition_penalty,
	hey_logit_t frequency_penalty,
	hey_logit_t presence_penalty
) {
	hey_index_t hash_exp = (hey_index_t)hey_ilog2((uint32_t)num_tokens);
	hey_index_t hash_size = 1 << hash_exp;
	// Grow so that load factor is at least 0.5
	hey_index_t extra = hash_size == num_tokens ? 1 : 2;
	hash_exp += extra;
	hash_size <<= extra;

	hey_repetition_penalty_state_t* state = hey_malloc(ctx, sizeof(hey_repetition_penalty_state_t));
	*state = (hey_repetition_penalty_state_t){
		.size = num_tokens,
		.repetition_penalty = repetition_penalty,
		.frequency_penalty = frequency_penalty,
		.presence_penalty = presence_penalty,
		.previous_num_tokens = 0,
		.hash_exp = hash_exp,
		.recent_tokens = hey_malloc(ctx, sizeof(hey_token_t) * num_tokens),
		.hash_keys = hey_malloc(ctx, sizeof(hey_token_t) * hash_size),
		.hash_values = hey_malloc(ctx, sizeof(hey_index_t) * hash_size),
	};

	hey_ring_buf_reset(state);

	return (hey_logit_processor_t){
		.fn = hey_repetition_penalty_logit_processor,
		.userdata = state,
	};
}

#endif
