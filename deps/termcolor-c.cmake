add_library(termcolor-c INTERFACE)
# SYSTEM is needed to suppress warning
target_include_directories(termcolor-c SYSTEM INTERFACE termcolor-c/include)
