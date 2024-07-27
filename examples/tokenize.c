#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <argparse.h>
#include <stddef.h>
#include <llama.h>
#include <hey.h>

int
main(int argc, const char* argv[]) {
	char* model_path = NULL;
	char* input_path = "-";
	int allow_special = false;
	int exit_code = EXIT_SUCCESS;

	llama_backend_init();

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
		OPT_END(),
	};

	struct argparse argparse;
	argparse_init(&argparse, options, NULL, ARGPARSE_STOP_AT_NON_OPTION);
	argparse_describe(&argparse, "Tokenize a string", NULL);
	argparse_parse(&argparse, argc, argv);

	struct llama_model* model = NULL;
	char* str_buf = NULL;
	char* tmp_buf = NULL;
	llama_token* token_buf = NULL;
	FILE* input_file = NULL;

	if (model_path == NULL) {
		fprintf(stderr, "Must specify model path\n");
		exit_code = EXIT_FAILURE;
		goto end;
	}

	struct llama_model_params model_params = llama_model_default_params();
	model = llama_load_model_from_file(model_path, model_params);
	if (model == NULL) {
		fprintf(stderr, "Could not load model\n");
		exit_code = EXIT_FAILURE;
		goto end;
	}

	if (strcmp(input_path, "-") == 0) {
		input_file = stdin;
	} else {
		input_file = fopen(input_path, "rb");
		if (input_file == NULL) {
			perror("Could not open file");
			exit_code = EXIT_FAILURE;
			goto end;
		}
	}

	llama_token vocab_size = llama_n_vocab(model);
	int ctx_size = llama_n_ctx_train(model);
	int max_token_len = 0;
	for (llama_token i = 0; i < vocab_size; ++i) {
		int token_len = -llama_token_to_piece(model, i, NULL, 0, 0, false);
		max_token_len = token_len > max_token_len ? token_len : max_token_len;
	}
	str_buf = malloc(max_token_len * ctx_size);
	tmp_buf = malloc(max_token_len);

	hey_index_t num_chars = (hey_index_t)fread(str_buf, 1, ctx_size, input_file);
	if (ferror(input_file)) {
		fprintf(stderr, "Could not read input file\n");
		exit_code = EXIT_FAILURE;
		goto end;
	}

	token_buf = calloc(sizeof(llama_token), ctx_size);
	int num_tokens = llama_tokenize(
		model,
		str_buf, num_chars,
		token_buf, ctx_size,
		false,
		allow_special
	);

	for (int i = 0; i < num_tokens; ++i) {
		int str_len = llama_token_to_piece(model, token_buf[i], tmp_buf, max_token_len, 0, false);
		printf("%d: |%.*s|\n", token_buf[i], str_len, tmp_buf);
	}

end:
	free(tmp_buf);
	free(str_buf);
	free(token_buf);

	if (input_file != stdin && input_file != NULL) {
		fclose(input_file);
	}

	if (model != NULL) {
		llama_free_model(model);
	}

	llama_backend_free();
	return exit_code;
}
