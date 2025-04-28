
find_library(muparser_LIBRARY NAMES muparser HINTS "${muparser_ROOT}/lib"  REQUIRED)

include_directories(BEFORE ${muparser_ROOT}/include/)

message(STATUS "Found muparser: ${muparser_LIBRARY}")
