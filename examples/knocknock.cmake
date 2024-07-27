add_executable(hey_knockknock "knockknock.c")
target_link_libraries(
	hey_knockknock PRIVATE
	argparse
	hey
	llama
	sokol
	${MATH_LIB}
)
set_target_properties(hey_knockknock PROPERTIES OUTPUT_NAME "knockknock")
