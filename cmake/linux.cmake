set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-fsanitize=address,undefined")
add_link_options("-fuse-ld=mold")
