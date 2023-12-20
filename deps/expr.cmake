add_library(expr INTERFACE)
# Suppress MSVC warning with SYSTEM
target_include_directories(expr SYSTEM INTERFACE expr)
