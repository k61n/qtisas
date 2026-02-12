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

message(STATUS "Found qwt: ${qwt_LIBRARY}")