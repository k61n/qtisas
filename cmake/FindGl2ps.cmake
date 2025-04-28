
find_library(gl2ps_LIBRARY NAMES gl2ps
    HINTS ${gl2ps_ROOT}/lib /usr/lib /usr/lib/aarch64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/local/lib
    REQUIRED)

include_directories(BEFORE ${gl2ps_ROOT}/include/)

message(STATUS "Found gl2ps: ${gl2ps_LIBRARY}")
