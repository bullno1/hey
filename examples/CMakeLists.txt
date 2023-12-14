SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "\${ORIGIN}")

add_executable(hey_tokenize "tokenize.c")
target_link_libraries(
	hey_tokenize PRIVATE
	argparse
	hey
	llama
)
set_target_properties(hey_tokenize PROPERTIES OUTPUT_NAME "tokenize")
