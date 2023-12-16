if (MSVC)
	# cublas_static is not available on Windows because reasons
	set(LLAMA_STATIC OFF CACHE BOOL "")
else()
	set(LLAMA_STATIC ON CACHE BOOL "")
endif()

set(LLAMA_CUBLAS ON CACHE BOOL "")
set(LLAMA_CUDA_F16 ON CACHE BOOL "")

add_subdirectory("llama.cpp" EXCLUDE_FROM_ALL)
