
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    find_program(QMAKE NAMES qmake6 qmake-qt5 qmake REQUIRED)
else ()
    foreach(dir IN LISTS CMAKE_PREFIX_PATH)
        find_program(QMAKE NAMES qmake PATHS ${dir}/bin NO_DEFAULT_PATH REQUIRED)
    endforeach()
endif ()
message(STATUS "Found qmake: ${QMAKE}")
