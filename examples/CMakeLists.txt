SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "\${ORIGIN}")

include(tokenize.cmake)
include(generate.cmake)
include(knocknock.cmake)
include(calculator.cmake)
