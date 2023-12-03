#ifndef HEY_LLAMA_CPP_H
#define HEY_LLAMA_CPP_H

#include "hey.h"

typedef struct llama_context hey_llama_context_t;

HEY_API size_t
hey_llama_cpp_adapter_size(hey_llama_context_t* context);

HEY_API hey_llm_t
hey_llama_cpp_adapter_init(hey_llama_context_t* context, void* mem);

#endif

#ifdef HEY_LLAMA_CPP_IMPLEMENTATION

#define HEY_LLAMA_CPP_PRIVATE static inline

// TODO: Proper include path
#include "../llama.cpp/llama.h"

typedef struct hey_llama_cpp_adapter_s {
	struct llama_context* context;
	hey_index_t num_tokens;
	hey_token_t tokens[];
} hey_llama_cpp_adapter_t;

HEY_LLAMA_CPP_PRIVATE hey_index_t
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

HEY_LLAMA_CPP_PRIVATE hey_index_t
hey_llama_cpp_detokenize(
	hey_token_t token,
	char* text, hey_index_t num_chars,
	void* ctx
) {
	hey_llama_cpp_adapter_t* adapter = ctx;
	const struct llama_model* model = llama_get_model(adapter->context);

	hey_index_t num_chars_out = llama_token_to_piece(
		model, token,
		text, num_chars
	);

	return num_chars_out >= 0 ? num_chars_out : -num_chars_out;
}

HEY_LLAMA_CPP_PRIVATE void
hey_llama_cpp_eval(
	hey_token_t* tokens, hey_index_t num_tokens,
	hey_logit_t* logits,
	void* ctx
) {
	hey_llama_cpp_adapter_t* adapter = ctx;

	hey_index_t prefix_len;
	hey_index_t cmp_len = HEY_MIN(num_tokens, adapter->num_tokens);
	for (prefix_len = 0; prefix_len < cmp_len; ++prefix_len) {
		if (adapter->tokens[prefix_len] != tokens[prefix_len]) {
			break;
		}
	}

	llama_kv_cache_seq_rm(adapter->context, 0, prefix_len, -1);
	llama_decode(
		adapter->context,
		llama_batch_get_one(tokens + prefix_len, num_tokens - prefix_len, prefix_len, 0)
	);

	hey_index_t vocab_size = llama_n_vocab(llama_get_model(adapter->context));
	HEY_MEMCPY(
		logits,
		llama_get_logits(adapter->context),
		sizeof(hey_logit_t) * vocab_size
	);
}

size_t
hey_llama_cpp_adapter_size(hey_llama_context_t* context) {
	return sizeof(hey_llama_cpp_adapter_t) + sizeof(hey_token_t) * llama_n_ctx(context);
}

HEY_API hey_llm_t
hey_llama_cpp_adapter_init(hey_llama_context_t* context, void* mem) {
	const struct llama_model* model = llama_get_model(context);
	hey_llama_cpp_adapter_t* adapter = mem;
	*adapter = (hey_llama_cpp_adapter_t){
		.context = context,
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
