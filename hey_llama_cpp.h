#ifndef HEY_LLAMA_CPP_H
#define HEY_LLAMA_CPP_H

#include "hey.h"

typedef struct llama_context hey_llama_context_t;
typedef struct llama_context_params hey_llama_context_params_t;

HEY_API size_t
hey_llama_cpp_adapter_size(hey_llama_context_t* context);

HEY_API hey_llm_t
hey_llama_cpp_adapter_init(
	hey_llama_context_t* context,
	hey_llama_context_params_t params,
	void* mem
);

#endif

#if defined(HEY_IMPLEMENTATION) && !defined(HEY_LLAMA_CPP_IMPLEMENTATION)
#define HEY_LLAMA_CPP_IMPLEMENTATION

#include <llama.h>

typedef struct {
	struct llama_context* context;
	hey_index_t num_tokens;
	hey_index_t batch_size;
	hey_token_t tokens[];
} hey_llama_cpp_adapter_t;

HEY_PRIVATE hey_index_t
hey_llama_cpp_tokenize(
	const char* text, hey_index_t num_chars,
	hey_token_t* tokens, hey_index_t num_tokens,
	bool allow_special,
	void* ctx
) {
	hey_llama_cpp_adapter_t* adapter = ctx;
	const struct llama_model* model = llama_get_model(adapter->context);

	hey_index_t num_tokens_out = llama_tokenize(
		model, text, num_chars,
		tokens, num_tokens,
		false,
		allow_special
	);

	return num_tokens_out >= 0 ? num_tokens_out : -num_tokens_out;
}

HEY_PRIVATE hey_index_t
hey_llama_cpp_detokenize(
	hey_token_t token,
	char* text, hey_index_t num_chars,
	void* ctx
) {
	hey_llama_cpp_adapter_t* adapter = ctx;
	const struct llama_model* model = llama_get_model(adapter->context);

	hey_index_t num_chars_out = llama_token_to_piece(
		model, token,
		text, num_chars,
		false
	);

	return num_chars_out >= 0 ? num_chars_out : -num_chars_out;
}

HEY_PRIVATE hey_index_t
hey_llama_cpp_find_prefix_len(
	const hey_token_t* restrict seq1, hey_index_t seq1_len,
	const hey_token_t* restrict seq2, hey_index_t seq2_len
) {
	hey_index_t cmp_len = HEY_MIN(seq1_len, seq2_len);
	hey_index_t i;
	for (i = 0; i < cmp_len; ++i) {
		if (seq1[i] != seq2[i]) {
			break;
		}
	}

	return i;
}

HEY_PRIVATE void
hey_llama_cpp_eval(
	hey_token_t* tokens, hey_index_t num_tokens,
	hey_logit_t* logits,
	void* ctx
) {
	hey_llama_cpp_adapter_t* adapter = ctx;

	hey_index_t prefix_len = hey_llama_cpp_find_prefix_len(
		adapter->tokens, adapter->num_tokens,
		tokens, num_tokens
	);

	llama_kv_cache_seq_rm(adapter->context, 0, prefix_len, -1);

	hey_index_t num_tokens_left = num_tokens - prefix_len;
	hey_index_t n_past = prefix_len;
	while (num_tokens_left > 0) {
		hey_index_t n_eval = HEY_MIN(
			adapter->batch_size, num_tokens_left
		);
		llama_decode(
			adapter->context,
			llama_batch_get_one(tokens + n_past, n_eval, n_past, 0)
		);
		n_past += n_eval;
		num_tokens_left -= n_eval;
	}

	hey_index_t vocab_size = llama_n_vocab(llama_get_model(adapter->context));
	HEY_MEMCPY(
		logits,
		llama_get_logits(adapter->context),
		sizeof(hey_logit_t) * vocab_size
	);
	HEY_MEMCPY(
		adapter->tokens + prefix_len,
		tokens + prefix_len,
		sizeof(hey_token_t) * (num_tokens - prefix_len)
	);
	adapter->num_tokens = num_tokens;
}

size_t
hey_llama_cpp_adapter_size(hey_llama_context_t* context) {
	return sizeof(hey_llama_cpp_adapter_t) + sizeof(hey_token_t) * llama_n_ctx(context);
}

hey_llm_t
hey_llama_cpp_adapter_init(
	hey_llama_context_t* context,
	hey_llama_context_params_t params,
	void* mem
) {
	const struct llama_model* model = llama_get_model(context);
	hey_llama_cpp_adapter_t* adapter = mem;
	*adapter = (hey_llama_cpp_adapter_t){
		.context = context,
		.batch_size = params.n_batch,
	};

	return (hey_llm_t){
		.ctx = adapter,
		.vocab_size = llama_n_vocab(model),
		.context_size = llama_n_ctx_train(model),
		.bos = llama_token_bos(model),
		.eos = llama_token_eos(model),
		.nl = llama_token_nl(model),
		.tokenize = hey_llama_cpp_tokenize,
		.detokenize = hey_llama_cpp_detokenize,
		.eval = hey_llama_cpp_eval,
	};
}

#endif
