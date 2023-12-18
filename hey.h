#ifndef HEY_H
#define HEY_H

#ifndef HEY_API
#	ifdef __cplusplus
#		define HEY_API extern "C"
#	else
#		define HEY_API
#	endif
#endif

// Define this to add unicode support
// Recommended: https://github.com/sheredom/utf8.h
#ifndef HEY_STRNICMP

#ifdef _MSC_VER
#	include <string.h>
#	define HEY_STRNICMP(LHS, RHS, LEN) _strnicmp(LHS, RHS, LEN)
#else
#	ifndef _DEFAULT_SOURCE
#		define _DEFAULT_SOURCE
#	endif
#	include <string.h>
#	define HEY_STRNICMP(LHS, RHS, LEN) strncasecmp(LHS, RHS, LEN)
#endif

#endif

#ifndef HEY_STRNCMP
#include <string.h>
#define HEY_STRNCMP(LHS, RHS, LEN) strncmp(LHS, RHS, LEN)
#endif

#ifndef HEY_MEMCPY
#include <string.h>
#define HEY_MEMCPY(DST, SRC, LEN) memcpy(DST, SRC, LEN)
#endif

#ifndef HEY_MALLOC
#include <stdlib.h>
#define HEY_MALLOC(CTX, SIZE) malloc(SIZE)
#define HEY_REALLOC(CTX, PTR, NEW_SIZE) realloc(PTR, NEW_SIZE)
#define HEY_FREE(CTX, SIZE) free(SIZE)
#endif

#ifndef HEY_ASSERT
#include <assert.h>
#define HEY_ASSERT(COND, MSG) assert((COND) && (MSG))
#endif

#ifndef HEY_TOKEN_TYPE
#include <stdint.h>
#define HEY_TOKEN_TYPE int32_t
#endif

#ifndef HEY_INDEX_TYPE
#include <stdint.h>
#define HEY_INDEX_TYPE int32_t
#endif

#ifndef HEY_VSNPRINTF
#include <stdio.h>
#define HEY_VSNPRINTF(OUTPUT, SIZE, FORMAT, ARGS) vsnprintf(OUTPUT, SIZE, FORMAT, ARGS)
#endif

#ifndef HEY_LOGIT_TYPE
#define HEY_LOGIT_TYPE float
#endif

#ifndef HEY_ALIGN_TYPE
#include <stddef.h>
#define HEY_ALIGN_TYPE max_align_t
#endif

// https://stackoverflow.com/a/6849629
#if _MSC_VER >= 1400
#	include <sal.h>
#	if _MSC_VER > 1400
#		define HEY_FMT_STR(FMT) _Printf_format_string_ FMT
#	else
#		define HEY_FMT_STR(FMT) __format_string FMT
#	endif
#	define HEY_PRINTF_LIKE(N, M)
#else
#	define HEY_FMT_STR(FMT) FMT
#	define HEY_PRINTF_LIKE(N, M) __attribute__((format(printf,N,M)))
#endif

#include <stdbool.h>
#include <math.h>

#ifndef HEY_ARENA_CHUNK_SIZE
#define HEY_ARENA_CHUNK_SIZE (2 * 1024 * 1024)
#endif

#define HEY_MAX(A, B) ((A) > (B) ? (A) : (B))
#define HEY_MIN(A, B) ((A) < (B) ? (A) : (B))
#define HEY_LOGIT_IGNORE (-INFINITY)
// It doesn't zero so it can't be called calloc
#define HEY_ARRAY_ALLOC(CTX, TYPE, LEN) HEY_MALLOC(CTX, sizeof(TYPE) * LEN)

#define HEY_STR(LITERAL) \
	(hey_str_t){ .chars = LITERAL, .length = sizeof(LITERAL) - 1}

#define HEY_ARRAY(TYPE, ...) (TYPE[]){ __VA_ARGS__, { 0 } }

typedef struct hey_s hey_t;
typedef struct hey_exec_s hey_exec_t;

typedef HEY_TOKEN_TYPE hey_token_t;
typedef HEY_INDEX_TYPE hey_index_t;
typedef HEY_LOGIT_TYPE hey_logit_t;

typedef enum hey_control_decision_e {
	HEY_CONTINUE,
	HEY_STOP,
	HEY_STOP_AND_RETRACT_TOKENS = HEY_STOP,
	HEY_STOP_AND_RETRACT_CHARACTERS,
} hey_control_decision_t;

typedef enum hey_event_type_e {
	HEY_EVENT_NEW_TOKENS,
	HEY_EVENT_EVAL_BEGIN,
	HEY_EVENT_EVAL_END,
	HEY_EVENT_SAMPLING_BEGIN,
	HEY_EVENT_SAMPLING_END,
	HEY_EVENT_REWIND,
} hey_event_type_t;

typedef enum hey_text_source_e {
	HEY_SOURCE_USER,
	HEY_SOURCE_LLM,
} hey_text_source_t;

typedef void(*hey_fn_t)(hey_exec_t* ctx, void* userdata);

typedef struct hey_llm_s {
	void* ctx;
	hey_token_t vocab_size;
	hey_index_t context_size;

	hey_token_t bos;
	hey_token_t eos;
	hey_token_t nl;

	hey_index_t (*tokenize)(
		const char* text, hey_index_t num_chars,
		hey_token_t* tokens, hey_index_t num_tokens,
		bool allow_special,
		void* ctx
	);

	hey_index_t (*detokenize)(
		hey_token_t token,
		char* text, hey_index_t num_chars,
		void* ctx
	);

	void (*eval)(
		hey_token_t* tokens, hey_index_t num_tokens,
		hey_logit_t* logits,
		void* ctx
	);
} hey_llm_t;

typedef struct hey_options_s {
	void* memctx;
	hey_llm_t llm;
} hey_options_t;

typedef struct hey_span_s {
	hey_index_t begin;
	hey_index_t end;
} hey_span_t;

typedef struct hey_str_t {
	hey_index_t length;
	const char* chars;
} hey_str_t;

typedef struct hey_state_s {
	hey_str_t healing_prefix;

	hey_index_t num_tokens;
	const hey_token_t* tokens;
	const hey_span_t* token_spans;

	hey_index_t num_chars;
	const char* text;
} hey_state_t;

typedef struct hey_var_s {
	const char* name;
	hey_span_t tokens;
	hey_span_t text;
} hey_var_t;

typedef struct hey_event_s {
	hey_event_type_t type;

	union {
		struct {
			const hey_token_t* tokens;
			hey_index_t num_tokens;
			hey_index_t healing_offset;
			hey_text_source_t source;
		} new_tokens;

		struct {
			hey_var_t* capture;
		} eval;

		struct {
			hey_index_t pos;
		} rewind;
	};
} hey_event_t;

typedef struct hey_logit_processor_s {
	void (*fn)(
		hey_logit_t* logits, hey_token_t num_logits,
		hey_exec_t* ctx,
		void* userdata
	);

	void* userdata;
} hey_logit_processor_t;

typedef struct hey_sampler_s {
	hey_token_t (*fn)(
		const hey_logit_t* logits, hey_token_t num_logits,
		hey_exec_t* ctx,
		void* userdata
	);

	void* userdata;
} hey_sampler_t;

typedef struct hey_controller_s {
	hey_control_decision_t (*fn)(hey_index_t* count, hey_exec_t* ctx, void* userdata);

	void* userdata;
} hey_controller_t;

typedef struct hey_watcher_s {
	void (*fn)(const hey_event_t* event, hey_exec_t* ctx, void* userdata);
	void* userdata;
} hey_watcher_t;

typedef struct hey_generate_options_s {
	hey_logit_processor_t logit_processor;
	hey_controller_t controller;
	hey_var_t* capture_into;
} hey_generate_options_t;

HEY_API hey_t*
hey_create(hey_options_t options);

HEY_API void
hey_destroy(hey_t* hey);

HEY_API void
hey_execute(hey_t* hey, hey_fn_t fn, void* userdata);

HEY_API const hey_llm_t*
hey_get_llm(hey_exec_t* ctx);

HEY_API const hey_state_t*
hey_get_state(hey_exec_t* ctx);

HEY_API hey_index_t
hey_get_pos(hey_exec_t* ctx);

HEY_API void
hey_rewind(hey_exec_t* ctx, hey_index_t pos);

HEY_API void*
hey_malloc(hey_exec_t* ctx, size_t size);

HEY_API hey_watcher_t
hey_set_watcher(hey_exec_t* ctx, hey_watcher_t watcher);

HEY_API hey_sampler_t
hey_set_sampler(hey_exec_t* ctx, hey_sampler_t sampler);

HEY_API hey_logit_processor_t
hey_set_logit_processor(hey_exec_t* ctx, hey_logit_processor_t processor);

HEY_API void
hey_generate(hey_exec_t* ctx, hey_generate_options_t options);

HEY_API void
hey_push_str(hey_exec_t* ctx, hey_str_t string, bool allow_special);

HEY_API void
hey_push_str_fmt(
	hey_exec_t* ctx,
	bool allow_special,
	HEY_FMT_STR(const char* fmt), ...
) HEY_PRINTF_LIKE(3, 4);

HEY_API void
hey_push_var(hey_exec_t* ctx, hey_var_t var);

HEY_API void
hey_push_tokens(hey_exec_t* ctx, const hey_token_t* tokens, hey_index_t num_tokens);

HEY_API hey_str_t
hey_get_var(hey_exec_t* ctx, hey_var_t var);

HEY_API hey_str_t
hey_str_from_cstr(const char* str);

HEY_API hey_str_t
hey_detokenize(hey_exec_t* ctx, hey_token_t token);

#endif

#if defined(HEY_IMPLEMENTATION) && !defined(HEY_MAIN_IMPLEMENTATION)
#define HEY_MAIN_IMPLEMENTATION

#include <tgmath.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define HEY_PRIVATE static inline

typedef struct hey_arena_chunk_s {
	struct hey_arena_chunk_s* next;
	char* current;
	char* end;
	char begin[];
} hey_arena_chunk_t;

typedef struct hey_arena_s {
	size_t chunk_size;
	hey_arena_chunk_t* current_chunk;
	hey_arena_chunk_t* free_chunks;
} hey_arena_t;

struct hey_exec_s {
	hey_t* owner;
	hey_sampler_t sampler;
	hey_logit_processor_t logit_processor;
	hey_watcher_t watcher;
	hey_index_t sync_index;
	hey_state_t state;
};

struct hey_s {
	hey_options_t options;
	hey_index_t max_token_len;

	hey_token_t* tokens;
	hey_logit_t* logits;
	char* text;
	hey_span_t* token_spans;
	char* tmp_str_buf;

	hey_arena_t arena;
	hey_exec_t exec;
};

HEY_PRIVATE hey_arena_chunk_t*
hey_arena_alloc_chunk(hey_arena_t* arena, size_t min_size) {
	hey_arena_chunk_t* chunk;

	if (
		arena->free_chunks != NULL
		&& arena->free_chunks->begin + min_size <= arena->free_chunks->end
	) {
		chunk = arena->free_chunks;
		arena->free_chunks = chunk->next;
	} else {
		size_t alloc_size = HEY_MAX(
			arena->chunk_size,
			sizeof(hey_arena_chunk_t) + min_size
		);
		chunk = HEY_MALLOC(hey->options.memctx, alloc_size);
		HEY_ASSERT(chunk, "Out of memory");
		chunk->current = chunk->begin;
		chunk->end = (char*)chunk + alloc_size;
	}

	chunk->next = arena->current_chunk;
	arena->current_chunk = chunk;
	return chunk;
}

HEY_PRIVATE void
hey_arena_reset(hey_arena_t* arena) {
	hey_arena_chunk_t* itr = arena->current_chunk;

	while (itr->next != NULL) {
		hey_arena_chunk_t* next = itr->next;

		if (itr->begin + arena->chunk_size < itr->end) {
			// Oversized alloc
			HEY_FREE(arena->ctx, itr);
		} else {
			itr->current = itr->begin;
			itr->next = arena->free_chunks;
			arena->free_chunks = itr;
		}

		itr = next;
	}

	itr->current = itr->begin;
	arena->current_chunk = itr;
}

HEY_PRIVATE intptr_t
hey_align_ptr(intptr_t ptr, size_t alignment) {
	return (((intptr_t)ptr + (intptr_t)alignment - 1) & -(intptr_t)alignment);
}

HEY_PRIVATE void*
hey_arena_alloc_from_chunk(hey_arena_chunk_t* chunk, size_t size, size_t align) {
	char* ptr = chunk->current;
	ptr = (char*)hey_align_ptr((intptr_t)ptr, align);
	char* new_pos = ptr + size;
	if (new_pos > chunk->end) { return NULL; }

	chunk->current = new_pos;
	return ptr;
}

HEY_PRIVATE void
hey_arena_init(hey_arena_t* arena, size_t chunk_size) {
	*arena = (hey_arena_t){
		.chunk_size = chunk_size,
	};
	hey_arena_alloc_chunk(arena, 0);
}

HEY_PRIVATE void
hey_arena_cleanup(hey_arena_t* arena) {
	for (
		hey_arena_chunk_t* itr = arena->current_chunk;
		itr != NULL;
	) {
		hey_arena_chunk_t* next = itr->next;
		HEY_FREE(hey->options.memctx, itr);
		itr = next;
	}

	for (
		hey_arena_chunk_t* itr = arena->free_chunks;
		itr != NULL;
	) {
		hey_arena_chunk_t* next = itr->next;
		HEY_FREE(hey->options.memctx, itr);
		itr = next;
	}
}

HEY_PRIVATE void*
hey_arena_alloc(hey_arena_t* arena, size_t size, size_t align) {
	hey_arena_chunk_t* current_chunk = arena->current_chunk;

	void* ptr = hey_arena_alloc_from_chunk(current_chunk, size, align);
	if (ptr != NULL) { return ptr; }

	current_chunk = hey_arena_alloc_chunk(arena, size + align - 1);
	HEY_ASSERT(current_chunk, "Out of memory");

	ptr = hey_arena_alloc_from_chunk(current_chunk, size, align);
	HEY_ASSERT(ptr, "Out of memory");

	return ptr;
}

HEY_PRIVATE hey_token_t
hey_sample_argmax(const hey_logit_t* logits, hey_token_t num_logits, hey_exec_t* ctx, void* userdata) {
	(void)ctx;
	(void)userdata;

	hey_logit_t max_score = HEY_LOGIT_IGNORE;
	for (hey_token_t i = 0; i < num_logits; ++i) {
		max_score = HEY_MAX(logits[i], max_score);
	}

	hey_logit_t sum = 0;
	for (hey_token_t i = 0; i < num_logits; ++i) {
		hey_logit_t p = exp(logits[i] - max_score);
		sum += p;
	}

	hey_token_t chosen_token = -1;
	hey_logit_t chosen_score = HEY_LOGIT_IGNORE;
	for (hey_token_t token = 0; token < num_logits; ++token) {
		hey_logit_t token_score = exp(logits[token] - max_score) / sum;

		if (token_score > chosen_score) {
			chosen_score = token_score;
			chosen_token = token;
		}
	}

	HEY_ASSERT(chosen_token != -1, "Invalid grammar");
	return chosen_token;
}

HEY_PRIVATE void
hey_sync_state(hey_exec_t* ctx) {
	hey_t* hey = ctx->owner;
	const hey_llm_t* llm = &hey->options.llm;

	for (hey_index_t i = ctx->sync_index; i < ctx->state.num_tokens; ++i) {
		hey_token_t token = hey->tokens[i];
		hey_index_t num_chars_left =
			llm->context_size * hey->max_token_len - ctx->state.num_chars;
		hey_index_t num_chars = llm->detokenize(
			token,
			hey->text + ctx->state.num_chars,
			num_chars_left,
			llm->ctx
		);
		HEY_ASSERT(num_chars <= num_chars_left, "Text buffer overflow");

		hey->token_spans[i].begin = ctx->state.num_chars;
		hey->token_spans[i].end = ctx->state.num_chars + num_chars;
		ctx->state.num_chars += num_chars;
	}

	ctx->sync_index = ctx->state.num_tokens;
}

HEY_PRIVATE void
hey_logit_processor_noop(hey_logit_t* logits, hey_token_t num_logits, hey_exec_t* ctx, void* userdata) {
	(void)logits;
	(void)num_logits;
	(void)ctx;
	(void)userdata;
}

HEY_PRIVATE void
hey_watcher_noop(const hey_event_t* event, hey_exec_t* ctx, void* userdata) {
	(void)event;
	(void)ctx;
	(void)userdata;
}

HEY_PRIVATE hey_control_decision_t
hey_controller_fill_context(hey_index_t* count, hey_exec_t* ctx, void* userdata) {
	(void)count;
	(void)userdata;

	const hey_state_t* state = hey_get_state(ctx);
	const hey_llm_t* llm = hey_get_llm(ctx);
	return state->num_tokens >= llm->context_size ? HEY_STOP : HEY_CONTINUE;
}

hey_t*
hey_create(hey_options_t options) {
	// Find the longest token
	hey_index_t max_token_len = 0;
	for (hey_index_t i = 0; i < options.llm.vocab_size; ++i) {
		hey_index_t len = options.llm.detokenize(i, NULL, 0, options.llm.ctx);
		if (len > max_token_len) {
			max_token_len = len;
		}
	}

	hey_t* hey = HEY_MALLOC(options->memctx, sizeof(hey_t));
	*hey = (hey_t){
		.options = options,
		.max_token_len = max_token_len,
		.tokens = HEY_ARRAY_ALLOC(
			options->memctx, hey_token_t, options.llm.context_size
		),
		.logits = HEY_ARRAY_ALLOC(options->memctx, hey_logit_t, options.llm.vocab_size),
		.text = HEY_MALLOC(
			options->memctx, max_token_len * options.llm.context_size
		),
		.token_spans = HEY_ARRAY_ALLOC(
			options->memctx, hey_span_t, options.llm.context_size
		),
		.tmp_str_buf = HEY_MALLOC(
			options->memctx, max_token_len * options.llm.context_size
		),
	};

	hey_arena_init(&hey->arena, HEY_ARENA_CHUNK_SIZE);

	return hey;
}

void
hey_destroy(hey_t* hey) {
	hey_arena_cleanup(&hey->arena);

	HEY_FREE(hey->options.memctx, hey->tmp_str_buf);
	HEY_FREE(hey->options.memctx, hey->token_spans);
	HEY_FREE(hey->options.memctx, hey->text);
	HEY_FREE(hey->options.memctx, hey->logits);
	HEY_FREE(hey->options.memctx, hey->tokens);
	HEY_FREE(hey->options.memctx, hey);
}

void
hey_execute(hey_t* hey, hey_fn_t fn, void* userdata) {
	hey_exec_t ctx = {
		.owner = hey,
		.sampler = {
			.fn = &hey_sample_argmax,
		},
		.logit_processor = {
			.fn = &hey_logit_processor_noop,
		},
		.watcher = {
			.fn = &hey_watcher_noop,
		},
		.state = {
			.tokens = hey->tokens,
			.token_spans = hey->token_spans,
			.text = hey->text,
		},
	};

	hey_arena_reset(&hey->arena);
	fn(&ctx, userdata);
}

const hey_llm_t*
hey_get_llm(hey_exec_t* ctx) {
	return &ctx->owner->options.llm;
}

void*
hey_malloc(hey_exec_t* ctx, size_t size) {
	return hey_arena_alloc(&ctx->owner->arena, size, _Alignof(HEY_ALIGN_TYPE));
}

const hey_state_t*
hey_get_state(hey_exec_t* ctx) {
	hey_sync_state(ctx);
	return &ctx->state;
}

void
hey_push_str_fmt(hey_exec_t* ctx, bool allow_special, const char* fmt, ...) {
	hey_t* hey = ctx->owner;

	char* tmp_str_buf = hey->tmp_str_buf;
	size_t max_len = hey->max_token_len * hey->options.llm.context_size;
	va_list args;
	va_start(args, fmt);
	hey_index_t len = HEY_VSNPRINTF(tmp_str_buf, max_len, fmt, args);
	va_end(args);
	HEY_ASSERT(len >= 0, "Encoding error");
	HEY_ASSERT(len < (hey_index_t)max_len, "Context overflow");

	hey_push_str(
		ctx,
		(hey_str_t){
			.chars = tmp_str_buf,
			.length = len
		},
		allow_special
	);
}

void
hey_push_str(hey_exec_t* ctx, hey_str_t string, bool allow_special) {
	hey_t* hey = ctx->owner;

	hey_index_t num_tokens_left = hey->options.llm.context_size - ctx->state.num_tokens;
	hey_token_t* new_tokens = hey->tokens + ctx->state.num_tokens;
	hey_index_t num_tokens = hey->options.llm.tokenize(
		string.chars, string.length,
		new_tokens, num_tokens_left,
		allow_special,
		hey->options.llm.ctx
	);

	HEY_ASSERT(num_tokens <= num_tokens_left, "Context overflow");
	ctx->state.num_tokens += num_tokens;
	ctx->state.healing_prefix.length = 0;

	ctx->watcher.fn(
		&(hey_event_t){
			.type = HEY_EVENT_NEW_TOKENS,
			.new_tokens = {
				.tokens = new_tokens,
				.num_tokens = num_tokens,
				.source = HEY_SOURCE_USER,
			},
		},
		ctx,
		ctx->watcher.userdata
	);
}

void
hey_push_tokens(hey_exec_t* ctx, const hey_token_t* tokens, hey_index_t num_tokens) {
	hey_t* hey = ctx->owner;
	HEY_ASSERT(
		ctx->state.num_tokens + num_tokens <= hey->options.llm.context_size,
		"Context overflow"
	);

	HEY_MEMCPY(
		hey->tokens + ctx->state.num_tokens,
		tokens,
		num_tokens * sizeof(hey_token_t)
	);
	ctx->state.num_tokens += num_tokens;
	ctx->state.healing_prefix.length = 0;

	ctx->watcher.fn(
		&(hey_event_t){
			.type = HEY_EVENT_NEW_TOKENS,
			.new_tokens = {
				.tokens = tokens,
				.num_tokens = num_tokens,
				.source = HEY_SOURCE_USER,
			},
		},
		ctx,
		ctx->watcher.userdata
	);
}

void
hey_push_var(hey_exec_t* ctx, hey_var_t var) {
	hey_push_str(
		ctx,
		(hey_str_t) {
			.chars = ctx->state.text + var.text.begin,
			.length = var.text.end - var.text.begin,
		},
		false
	);
}

hey_index_t
hey_get_pos(hey_exec_t* ctx) {
	return ctx->state.num_tokens;
}

void
hey_rewind(hey_exec_t* ctx, hey_index_t pos) {
	HEY_ASSERT(pos <= ctx->state.num_tokens, "Invalid rewind position");
	if (pos == ctx->state.num_tokens) { return; }

	hey_sync_state(ctx);
	ctx->state.num_chars = ctx->state.token_spans[pos].begin;
	ctx->state.num_tokens = pos;
	ctx->sync_index = pos;

	ctx->watcher.fn(
		&(hey_event_t){
			.type = HEY_EVENT_REWIND,
			.rewind = { .pos = pos, },
		},
		ctx,
		ctx->watcher.userdata
	);
}

hey_sampler_t
hey_set_sampler(hey_exec_t* ctx, hey_sampler_t sampler) {
	hey_sampler_t old_sampler = ctx->sampler;
	ctx->sampler = sampler;
	return old_sampler;
}

hey_logit_processor_t
hey_set_logit_processor(hey_exec_t* ctx, hey_logit_processor_t processor) {
	hey_logit_processor_t old_processor = ctx->logit_processor;
	ctx->logit_processor = processor;
	return old_processor;
}

hey_watcher_t
hey_set_watcher(hey_exec_t* ctx, hey_watcher_t watcher) {
	hey_watcher_t old_watcher = ctx->watcher;
	ctx->watcher = watcher;
	return old_watcher;
}

hey_str_t
hey_str_from_cstr(const char* str) {
	return (hey_str_t){
		.chars = str,
		.length = (hey_index_t)strlen(str),
	};
}

hey_str_t
hey_get_var(hey_exec_t* ctx, hey_var_t var) {
	const hey_state_t* state = hey_get_state(ctx);

	return (hey_str_t){
		.chars = state->text + var.text.begin,
		.length = var.text.end - var.text.begin,
	};
}

hey_str_t
hey_detokenize(hey_exec_t* ctx, hey_token_t token) {
	hey_t* hey = ctx->owner;
	const hey_llm_t* llm = &hey->options.llm;
	hey_index_t num_chars = llm->detokenize(
		token,
		hey->tmp_str_buf, hey->max_token_len,
		llm->ctx
	);
	HEY_ASSERT(num_chars <= hey->max_token_len, "Temp string buffer overflow");
	return (hey_str_t){
		.chars = hey->tmp_str_buf,
		.length = num_chars,
	};
}

void
hey_generate(hey_exec_t* ctx, hey_generate_options_t options) {
	hey_sync_state(ctx);

	if (options.logit_processor.fn == NULL) {
		options.logit_processor.fn = hey_logit_processor_noop;
	}

	if (options.controller.fn == NULL) {
		options.controller.fn = hey_controller_fill_context;
	}

	hey_t* hey = ctx->owner;
	const hey_llm_t* llm = &hey->options.llm;
	hey_var_t* capture = options.capture_into;

	if (capture != NULL) {
		capture->text.begin = ctx->state.num_chars + ctx->state.healing_prefix.length;
		capture->tokens.begin = ctx->state.num_tokens;
	}

	// Token healing
	if (ctx->state.healing_prefix.length == 0 && ctx->state.num_tokens > 0) {
		// Pop the last token and make that the healing prefix
		hey_span_t last_token_span = ctx->state.token_spans[ctx->state.num_tokens - 1];
		ctx->state.healing_prefix.chars = hey->text + last_token_span.begin;
		ctx->state.healing_prefix.length = last_token_span.end - last_token_span.begin;
		ctx->state.num_tokens -= 1;
		ctx->sync_index = ctx->state.num_tokens;
		ctx->state.num_chars = last_token_span.begin;

		if (capture != NULL) {
			capture->tokens.begin = ctx->state.num_tokens;
		}
	}

	hey_logit_t* logits = hey->logits;
	hey_token_t* tokens = hey->tokens;
	hey_sampler_t sampler = ctx->sampler;
	hey_watcher_t watcher = ctx->watcher;
	hey_token_t vocab_size = llm->vocab_size;
	char* tmp_str_buf = hey->tmp_str_buf;

	hey_control_decision_t decision;
	do {
		watcher.fn(
			&(hey_event_t){
				.type = HEY_EVENT_EVAL_BEGIN,
				.eval = { .capture = options.capture_into },
			},
			ctx,
			watcher.userdata
		);
		llm->eval(tokens, ctx->state.num_tokens, logits, llm->ctx);
		watcher.fn(
			&(hey_event_t){
				.type = HEY_EVENT_EVAL_END,
				.eval = { .capture = options.capture_into },
			},
			ctx,
			watcher.userdata
		);

		watcher.fn(&(hey_event_t){ .type = HEY_EVENT_SAMPLING_BEGIN }, ctx, watcher.userdata);

		// Filter tokens for healing
		if (ctx->state.healing_prefix.length > 0) {
			for (hey_token_t token = 0; token < vocab_size; ++token) {
				hey_index_t num_chars = llm->detokenize(
					token, tmp_str_buf, hey->max_token_len, llm->ctx
				);

				hey_index_t cmp_len = HEY_MIN(ctx->state.healing_prefix.length, num_chars);
				if (HEY_STRNCMP(ctx->state.healing_prefix.chars, tmp_str_buf, cmp_len) != 0) {
					logits[token] = HEY_LOGIT_IGNORE;
				}
			}
		}
		hey_index_t healing_offset = ctx->state.healing_prefix.length;

		// Generation processor
		options.logit_processor.fn(logits, vocab_size, ctx, options.logit_processor.userdata);

		// Global processor
		ctx->logit_processor.fn(logits, vocab_size, ctx, ctx->logit_processor.userdata);

		hey_token_t chosen_token = sampler.fn(logits, vocab_size, ctx, sampler.userdata);
		watcher.fn(&(hey_event_t){ .type = HEY_EVENT_SAMPLING_END }, ctx, watcher.userdata);

		HEY_ASSERT(ctx->state.num_tokens < llm->context_size, "Context overflow");
		tokens[ctx->state.num_tokens] = chosen_token;
		ctx->state.num_tokens += 1;
		watcher.fn(
			&(hey_event_t){
				.type = HEY_EVENT_NEW_TOKENS,
				.new_tokens = {
					.tokens = &chosen_token,
					.num_tokens = 1,
					.source = HEY_SOURCE_LLM,
					.healing_offset = healing_offset,
				},
			},
			ctx,
			watcher.userdata
		);

		// Reduce the healing prefix by the new token
		if (ctx->state.healing_prefix.length > 0) {
			hey_index_t num_chars = llm->detokenize(
				chosen_token, tmp_str_buf, hey->max_token_len, llm->ctx
			);
			ctx->state.healing_prefix.chars += num_chars;
			ctx->state.healing_prefix.length = HEY_MAX(
				ctx->state.healing_prefix.length - num_chars, 0
			);
		}

		hey_index_t count = 0;
		decision = options.controller.fn(&count, ctx, options.controller.userdata);

		switch (decision) {
			case HEY_CONTINUE:
				break;
			case HEY_STOP_AND_RETRACT_TOKENS:
				HEY_ASSERT(count >= 0, "Cannot retract negative number of tokens");
				HEY_ASSERT(count <= ctx->state.num_tokens, "Cannot retract more tokens than available");
				hey_rewind(ctx, ctx->state.num_tokens - count);
				if (capture != NULL) {
					capture->text.end = ctx->state.num_chars;
					capture->tokens.end = ctx->state.num_tokens;
				}
				break;
			case HEY_STOP_AND_RETRACT_CHARACTERS: {
				hey_sync_state(ctx);
				HEY_ASSERT(count >= 0, "Cannot retract negative number of characters");
				HEY_ASSERT(count <= ctx->state.num_chars, "Cannot retract more characters than available");

				hey_index_t num_chars = ctx->state.num_chars - count;
				for (hey_index_t i = ctx->state.num_tokens - 1; i >= 0; --i) {
					hey_span_t token_span = ctx->state.token_spans[i];
					if (
						token_span.begin <= num_chars
						&& num_chars < token_span.end
					) {
						ctx->state.healing_prefix.chars = hey->text + token_span.begin;
						ctx->state.healing_prefix.length = num_chars - token_span.begin;
						hey_rewind(ctx, i);

						if (capture != NULL) {
							capture->text.end = num_chars;
							capture->tokens.end = i;
						}

						break;
					}
				}
			} break;
		}
	} while(decision == HEY_CONTINUE);
}

#endif
