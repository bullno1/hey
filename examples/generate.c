#define HEY_IMPLEMENTATION
#include <hey.h>
#include <hey_suffix.h>
#include <hey_llama_cpp.h>
#include <llama.h>
#include <argparse.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#define SOKOL_IMPL
#include <sokol_time.h>

#include "termcolor.h"

typedef struct watcher_state_s {
	double gpu_time;
	double cpu_time;
	uint64_t last_time;
	hey_index_t num_llm_tokens;
} watcher_state_t;

typedef struct state_s {
	hey_str_t input;
	bool allow_special;
} state_t;

static void
watcher(const hey_event_t* event, hey_exec_t* ctx, void* userdata) {
	watcher_state_t* watcher_state = userdata;
	switch(event->type) {
		case HEY_EVENT_NEW_TOKENS:
			watcher_state->num_llm_tokens +=
				event->new_tokens.source == HEY_SOURCE_LLM ? 1 : 0;

			if (event->new_tokens.source == HEY_SOURCE_USER) {
				hey_term_put(stdout, ANSI_CODE_RESET);
			} else {
				hey_term_put(stdout, ANSI_CODE_BLUE);
			}

			for (hey_index_t i = 0; i < event->new_tokens.num_tokens; ++i) {
				hey_token_t token = event->new_tokens.tokens[i];
				hey_str_t str = hey_detokenize(ctx, token);
				str.chars += event->new_tokens.healing_offset;
				str.length -= event->new_tokens.healing_offset;
				if (str.length > 0) {
					fprintf(stdout, "%.*s", str.length, str.chars);
					fflush(stdout);
				}
			}
			break;
		case HEY_EVENT_EVAL_BEGIN:
		case HEY_EVENT_SAMPLING_BEGIN:
			watcher_state->last_time = stm_now();;
			break;
		case HEY_EVENT_EVAL_END:
			watcher_state->gpu_time += stm_ms(stm_since(watcher_state->last_time));
			break;
		case HEY_EVENT_SAMPLING_END:
			watcher_state->cpu_time += stm_ms(stm_since(watcher_state->last_time));
			break;
		default:
			break;
	}
}

static void
generate(hey_exec_t* ctx, void* userdata) {
	state_t* state = userdata;
	struct watcher_state_s watcher_state = { 0 };
	hey_set_watcher(ctx, (hey_watcher_t){
		.fn = watcher,
		.userdata = &watcher_state
	});

	const hey_llm_t* llm = hey_get_llm(ctx);
	hey_push_str(ctx, state->input, state->allow_special);

	hey_var_t answer;
	hey_generate(ctx, (hey_generate_options_t){
		.controller = hey_ends_at_token(llm->eos),
		.capture_into = &answer,
	});

	hey_str_t capture = hey_get_var(ctx, answer);
	const hey_state_t* hey_state = hey_get_state(ctx);

	hey_term_put(stderr, ANSI_CODE_RESET);
	fprintf(stderr, "\n------------\n");
	fprintf(stderr, "Context: |%.*s|\n", hey_state->num_chars, hey_state->text);
	fprintf(stderr, "Capture span: [%d, %d)\n", answer.text.begin, answer.text.end);
	fprintf(stderr, "Capture: |%.*s|\n", capture.length, capture.chars);
	fprintf(stderr, "Total gpu time: %fms\n", watcher_state.gpu_time);
	fprintf(stderr, "Total cpu time: %fms\n", watcher_state.cpu_time);
	fprintf(stderr, "Generation speed: %ft/s\n", (double)watcher_state.num_llm_tokens / watcher_state.gpu_time * 1000.0);
}

int
main(int argc, const char* argv[]) {
	stm_setup();
	hey_term_enable_color(stdout);

	char* model_path = NULL;
	char* input_path = "-";
	int allow_special = false;
	int num_gpu_layers = 999;
	int exit_code = EXIT_SUCCESS;
	struct llama_context* llama_context = NULL;
	void* hey_llama_adapter = NULL;
	hey_t* hey = NULL;

	struct argparse_option options[] = {
		OPT_HELP(),
		{
			.type = ARGPARSE_OPT_STRING,
			.long_name = "model",
			.short_name = 'm',
			.help = "The model file",
			.value = &model_path,
		},
		{
			.type = ARGPARSE_OPT_STRING,
			.long_name = "input",
			.short_name = 'i',
			.help = "The input file. Use `-` for stdin.",
			.value = &input_path,
		},
		{
			.type = ARGPARSE_OPT_BOOLEAN,
			.long_name = "allow-special",
			.help = "Allow special tokens",
			.value = &allow_special,
		},
		{
			.type = ARGPARSE_OPT_INTEGER,
			.long_name = "num-gpu-layers",
			.help = "Numer of layers to offload to GPU",
			.value = &num_gpu_layers,
		},
		OPT_END(),
	};

	struct argparse argparse;
	argparse_init(&argparse, options, NULL, ARGPARSE_STOP_AT_NON_OPTION);
	argparse_describe(&argparse, "Tokenize a string", NULL);
	argparse_parse(&argparse, argc, argv);

	struct llama_model* model = NULL;
	char* str_buf = NULL;
	FILE* input_file = NULL;

	if (model_path == NULL) {
		fprintf(stderr, "Must specify model path\n");
		exit_code = EXIT_FAILURE;
		goto end;
	}

	struct llama_model_params model_params = llama_model_default_params();
	model_params.n_gpu_layers = num_gpu_layers;
	model = llama_load_model_from_file(model_path, model_params);
	if (model == NULL) {
		fprintf(stderr, "Could not load model\n");
		exit_code = EXIT_FAILURE;
		goto end;
	}

	struct llama_context_params ctx_params = llama_context_default_params();
	ctx_params.n_ctx = 0;
	llama_backend_init(false);
	llama_context = llama_new_context_with_model(model, ctx_params);

	size_t adapter_size = hey_llama_cpp_adapter_size(llama_context);
	hey_llama_adapter = malloc(adapter_size);
	hey_llm_t llm = hey_llama_cpp_adapter_init(llama_context, hey_llama_adapter);

	hey = hey_create((hey_options_t){
		.llm = llm,
	});

	if (strcmp(input_path, "-") == 0) {
		input_file = stdin;
	} else {
		input_file = fopen(input_path, "rb");
		if (input_file == NULL) {
			perror("Could not open input file");
			exit_code = EXIT_FAILURE;
			goto end;
		}
	}

	llama_token vocab_size = llm.vocab_size;
	int ctx_size = llm.context_size;
	int max_token_len = 0;
	for (llama_token i = 0; i < vocab_size; ++i) {
		int token_len = -llama_token_to_piece(model, i, NULL, 0);
		max_token_len = token_len > max_token_len ? token_len : max_token_len;
	}
	str_buf = malloc(max_token_len * ctx_size);

	hey_index_t num_chars = (hey_index_t)fread(str_buf, 1, ctx_size, input_file);
	if (ferror(input_file)) {
		fprintf(stderr, "Could not read input file\n");
		exit_code = EXIT_FAILURE;
		goto end;
	}

	hey_execute(hey, generate, &(state_t){
		.allow_special = allow_special,
		.input = {
			.chars = str_buf,
			.length = num_chars,
		},
	});

	hey_term_put(stdout, ANSI_CODE_RESET);
end:
	if (hey != NULL) {
		hey_destroy(hey);
	}

	free(str_buf);

	if (input_file != stdin && input_file != NULL) {
		fclose(input_file);
	}

	if (hey_llama_adapter != NULL) {
		free(hey_llama_adapter);
	}

	if (llama_context != NULL) {
		llama_free(llama_context);
		llama_backend_free();
	}

	if (model != NULL) {
		llama_free_model(model);
	}

	return exit_code;
}
