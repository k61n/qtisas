
set(LIBRARIES
    "alglib" "minigzip" "qtexengine" "qwt" "qwtplot3d" "tamuanova")
set(QTISAS_LIBS "")
foreach (lib ${LIBRARIES})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib})
    if (BUILD_SHARED_LIBS)
        list(APPEND QTISAS_LIBS ${lib}_shared)
    else (TARGET ${lib}_shared)
        list(APPEND QTISAS_LIBS ${lib}_static)
    endif ()

    file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/include/*.h")
    if(HEADERS)
        file(COPY ${HEADERS} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/include/${lib}")
    endif()
endforeach ()
include_directories(${CMAKE_CURRENT_BINARY_DIR}/3rdparty/include/)
