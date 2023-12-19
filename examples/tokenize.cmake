add_executable(hey_tokenize "tokenize.c")
target_link_libraries(
	hey_tokenize PRIVATE
	argparse
	hey
	llama
)
set_target_properties(hey_tokenize PROPERTIES OUTPUT_NAME "tokenize")
