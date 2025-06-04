
# Building 3rdparty qtisas libs
file(INSTALL FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.ps1
    PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/scripts/)
set(LIBRARIES
    "alglib" "minigzip" "qtexengine" "qwt" "qwtplot3d" "tamuanova")
foreach (lib ${LIBRARIES})
    if (APPLE)
        execute_process(
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${OS} ${ARCH} ${CORES} ${lib}
            ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/ ${CMAKE_GENERATOR} ${CMAKE_MAKE_PROGRAM}
            ${CMAKE_C_COMPILER} ${CMAKE_CXX_COMPILER}
            ${CMAKE_VERSION} ${GSL_ROOT} ${gl2ps_ROOT} "${CMAKE_PREFIX_PATH}" ${V} ${CMAKE_OSX_SYSROOT})
    elseif (UNIX)
        execute_process(
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${OS} ${ARCH} ${CORES} ${lib}
            ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/ ${CMAKE_GENERATOR} ${CMAKE_MAKE_PROGRAM}
            ${CMAKE_C_COMPILER} ${CMAKE_CXX_COMPILER}
            ${CMAKE_VERSION} ${GSL_ROOT} ${gl2ps_ROOT} "${CMAKE_PREFIX_PATH}" ${V})
    elseif (WIN32)
        execute_process(
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.ps1
            -os ${OS} -arch ${ARCH} -cores ${CORES} -name ${lib} -libdir ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/
            -cc ${CMAKE_C_COMPILER} -cxx ${CMAKE_CXX_COMPILER} -make ${CMAKE_MAKE_PROGRAM} -gen ${CMAKE_GENERATOR}
            -qt ${CMAKE_PREFIX_PATH} -zlib ${ZLIB_ROOT} -gsl ${GSL_ROOT} -gl2ps ${gl2ps_ROOT}
            RESULT_VARIABLE OUTPUT)
    endif ()
endforeach ()
# Importing 3rdparty libs
foreach (lib ${LIBRARIES})
    find_library(${lib}_LIBRARY NAMES ${lib} PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/lib NO_DEFAULT_PATH REQUIRED)
    message(STATUS "Found ${lib}: ${${lib}_LIBRARY}")
    set(LINKS ${LINKS} ${${lib}_LIBRARY})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/include/)
endforeach ()
