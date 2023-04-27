cmake_minimum_required(VERSION 3.24)

project(qtisas
        VERSION 0.0.1
        LANGUAGES CXX)

add_compile_options(-w)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

################################################################################
# Building 3rdparty libs and adding them to the project
#

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

if (UNIX)
    if (APPLE)
        execute_process(COMMAND "sysctl" "-n" "hw.ncpu" OUTPUT_VARIABLE CORES)
    else()
        execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
    endif()
elseif(WIN32)
    set(CORES 1)
else()
    set(CORES 1)
endif()

file(INSTALL FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-gsl.sh
    PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/scripts/)

execute_process(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-gsl.sh ${OS} ${ARCH}
    ${CORES} ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/gsl/)
set(LIBRARIES
    "alglib" "minigzip" "muparser" "qtexengine" "qwt" "qwtplot3d" "tamuanova"
    "yaml-cpp")
foreach(lib ${LIBRARIES})
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${OS} ${ARCH}
            ${CORES} ${lib} ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/)
endforeach()

################################################################################
# Project definitions
#

set(LIBRARIES ${LIBRARIES} gsl)
foreach(lib ${LIBRARIES})
    find_library(${lib}_LIBRARY NAMES ${lib} PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/lib NO_DEFAULT_PATH)
    set(LINKS ${LINKS} ${${lib}_LIBRARY})
endforeach()
find_library(gslcblas_LIBRARY NAMES gslcblas PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
set(LINKS ${LINKS} ${gslcblas_LIBRARY})

find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(Qt5 COMPONENTS Core OpenGL Network PrintSupport Svg Xml Widgets REQUIRED)
find_package(TIFF REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/alglib/include/alglib/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qtexengine/include/qtexengine/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/include/qwtplot3d/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/tamuanova/include/tamuanova/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/yaml-cpp/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/ascii1d/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/common/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/compile/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/fittable/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/genmin/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/jnse/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/svd/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/analysis/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/analysis/dialogs/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/core/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/lib/3rdparty/qtcolorpicker/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/lib/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/matrix/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot2D/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot2D/dialogs/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot3D/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/scripting/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/table/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/ascii1d/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/common/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/compile/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/dan/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/fittable/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/genmin/include/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/jnse/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/svd/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/analysis/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/analysis/dialogs/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/core/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/lib/3rdparty/qtcolorpicker/src/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/lib/include/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/matrix/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/plot2D/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/plot2D/dialogs/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/plot3D/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/scripting/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/table/)

if (WITH_PYTHON)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/python-sipcmd.py
        OUTPUT_VARIABLE sipcommand)
    string(STRIP ${sipcommand} sipcommand)
    execute_process(
        COMMAND bash -c "${sipcommand} -c ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qti.sip")
    include_directories(
        ${Python3_INCLUDE_DIRS}/
        ${CMAKE_CURRENT_BINARY_DIR}/)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/analysis/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/core/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/lib/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/matrix/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot2D/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot3D/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/scripting/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/table/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/ascii1d/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/common/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/compile/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/fittable/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/jnse/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/svd/)

################################################################################
# Project definitions
#

SET(SOURCES
    ${ANALYSIS_SOURCES} ${CORE_SOURCES} ${LIB_SOURCES} ${MATRIX_SOURCES}
    ${PLOT2D_SOURCES} ${PLOT3D_SOURCES} ${SCRIPTING_SOURCES} ${TABLE_SOURCES}
    ${ASCII1D_SOURCES} ${COMMON_SOURCES} ${COMPILE_SOURCES} ${DAN_SOURCES}
    ${FITTABLE_SOURCES} ${JNSE_SOURCES} ${SVD_SOURCES})

SET(HEADERS
    ${ANALYSIS_HEADERS} ${CORE_HEADERS} ${LIB_HEADERS} ${MATRIX_HEADERS}
    ${PLOT2D_HEADERS} ${PLOT3D_HEADERS} ${SCRIPTING_HEADERS} ${TABLE_HEADERS}
    ${ASCII1D_HEADERS} ${COMMON_HEADERS} ${COMPILE_HEADERS} ${DAN_HEADERS}
    ${FITTABLE_HEADERS} ${JNSE_HEADERS} ${SVD_HEADERS})

SET(UI_SOURCES
    ${ANALYSIS_UIS} ${CORE_UIS} ${LIB_UIS} ${MATRIX_UIS}
    ${PLOT2D_UIS} ${PLOT3D_UIS} ${SCRIPTING_UIS} ${TABLE_UIS}
    ${ASCII1D_UIS} ${COMMON_UIS} ${COMPILE_UIS} ${DAN_UIS}
    ${FITTABLE_UIS} ${JNSE_UIS} ${SVD_UIS})

qt5_add_resources(RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/icons/icons/icons.qrc)

add_definitions(-DASCII1D=1 -DCOMPILE=1 -DDAN=1 -DFITTABLE=1 -DJNSE=1
    -DHAVE_ALGLIB=1 -DHAVE_TAMUANOVA=1 -DSVD=1 -DTEX_OUTPUT=1
    -DSCRIPTING_MUPARSER=1 -DTIFFTIFF=1 -DYAMLYAML=1)
if(WITH_PYTHON)
    add_definitions(-DSCRIPTING_PYTHON=1 -DSCRIPTING_CONSOLE=1)
endif()

add_executable(qtisas ${SOURCES} ${HEADERS} ${UI_SOURCES} ${RESOURCES})
set_target_properties(qtisas PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(qtisas PUBLIC ${LINKS}
        Qt5::Core Qt5::Network Qt5::OpenGL Qt5::PrintSupport Qt5::Svg Qt5::Widgets Qt5::Xml
        OpenGL::GLU PNG::PNG TIFF::TIFF ZLIB::ZLIB)
if(WITH_PYTHON)
    target_link_libraries(qtisas PUBLIC ${Python3_LIBRARIES})
endif()