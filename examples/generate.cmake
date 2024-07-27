add_executable(hey_generate "generate.c")
target_link_libraries(
	hey_generate PRIVATE
	argparse
	hey
	llama
	sokol
	${MATH_LIB}
)
set_target_properties(hey_generate PROPERTIES OUTPUT_NAME "generate")
