
find_library(yaml-cpp_LIBRARY NAMES yaml-cpp HINTS "${yaml-cpp_ROOT}/lib" REQUIRED)

include_directories(BEFORE ${yaml-cpp_ROOT}/include/)

message(STATUS "Found yaml-cpp: ${yaml-cpp_LIBRARY}")
