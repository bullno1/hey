set(LLAMA_STATIC ON CACHE BOOL "")
#set(LLAMA_LTO ON CACHE BOOL "")
set(LLAMA_CUBLAS ON CACHE BOOL "")
set(LLAMA_NATIVE OFF CACHE BOOL "")
set(LLAMA_CUDA_F16 ON CACHE BOOL "")

add_subdirectory("llama.cpp" EXCLUDE_FROM_ALL)
