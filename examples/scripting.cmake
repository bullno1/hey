add_executable(hey_scripting "scripting.c")
target_link_libraries(
	hey_scripting PRIVATE
	argparse
	hey
	llama
	sokol
)
set_target_properties(hey_scripting PROPERTIES OUTPUT_NAME "scripting")
