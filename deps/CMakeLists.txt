if (NOT MSVC)
	add_compile_options(-fPIC)
endif ()

include(llama.cpp.cmake)
include(argparse.cmake)
include(termcolor-c.cmake)
include(sokol.cmake)
