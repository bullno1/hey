add_library(sokol INTERFACE)
# SYSTEM is needed to suppress warning
target_include_directories(sokol SYSTEM INTERFACE sokol)
