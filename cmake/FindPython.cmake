
# on macOS multiple version of python are available and we need to pick
# the one user pointed to
if (APPLE)
    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.15")
        cmake_policy(SET CMP0094 NEW)
    endif ()
endif ()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
else ()
    find_package(PythonInterp "3" REQUIRED)
    find_package(PythonLibs "3" REQUIRED)
    set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
    set(Python3_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
    set(Python3_LIBRARIES ${PYTHON_LIBRARIES})
endif ()

include_directories(${Python3_INCLUDE_DIRS}/)
