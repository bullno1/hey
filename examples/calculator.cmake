add_executable(hey_calculator "calculator.c")
target_link_libraries(
	hey_calculator PRIVATE
	argparse
	hey
	llama
	sokol
	expr
	${MATH_LIB}
)
set_target_properties(hey_calculator PROPERTIES OUTPUT_NAME "calculator")
