if (NOT MSVC)
	add_compile_options(-fPIC)
endif ()

include(llama_cpp.cmake)
include(argparse.cmake)
include(termcolor-c.cmake)
