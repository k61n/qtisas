if(APPLE)
    find_library(qwt_LIBRARY NAMES Qwt qwt
            HINTS ${qwt_ROOT}/lib /opt/homebrew/lib /usr/local/lib
            REQUIRED)
    if(qwt_LIBRARY MATCHES "\\.framework$")
        # Framework headers are at Qwt.framework/Headers/
        # Code uses #include <qwt/...>, so create a symlink wrapper
        set(qwt_INCLUDE_WRAPPER "${CMAKE_BINARY_DIR}/qwt_include")
        file(MAKE_DIRECTORY "${qwt_INCLUDE_WRAPPER}")
        # Create symlink: qwt_include/qwt -> Qwt.framework/Headers
        if(NOT EXISTS "${qwt_INCLUDE_WRAPPER}/qwt")
            execute_process(
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "${qwt_LIBRARY}/Headers"
                    "${qwt_INCLUDE_WRAPPER}/qwt"
            )
        endif()
        include_directories(BEFORE "${qwt_INCLUDE_WRAPPER}")
    else()
        include_directories(BEFORE ${qwt_ROOT}/include/)
    endif()
else()
    find_library(qwt_LIBRARY NAMES qwt qwt-qt6 qwt5 qwt-qt5
            HINTS ${qwt_ROOT}/lib /usr/lib /usr/lib/aarch64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/local/lib
            REQUIRED)
    include_directories(BEFORE ${qwt_ROOT}/include/)
endif()

# Check Qwt version >= 6.3
find_file(qwt_GLOBAL_HEADER qwt_global.h
        PATH_SUFFIXES qwt
        HINTS ${qwt_ROOT}/include /opt/homebrew/include /usr/include /usr/local/include
              "${qwt_INCLUDE_WRAPPER}")
if(qwt_GLOBAL_HEADER)
    file(STRINGS "${qwt_GLOBAL_HEADER}" qwt_VERSION_LINE REGEX "^#define[ \t]+QWT_VERSION_STR")
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" qwt_VERSION "${qwt_VERSION_LINE}")
    message(STATUS "Found qwt: ${qwt_LIBRARY} (found version: ${qwt_VERSION})")
    if(qwt_VERSION VERSION_LESS "6.3.0")
        message(FATAL_ERROR "Qwt >= 6.3 is required, but found ${qwt_VERSION}")
    endif()
else()
    message(WARNING "Could not find qwt_global.h to verify Qwt version")
endif()
