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

set(3RD_PARTY_LIST
    "alglib" "minigzip" "muparser" "qtexengine" "qwt" "qwtplot3d" "tamuanova" "tiff" "yaml-cpp")
foreach(dep ${3RD_PARTY_LIST})
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${OS} ${ARCH} ${CORES} ${dep} ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${dep}/)
endforeach()
execute_process(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-gsl.sh ${OS} ${ARCH} ${CORES} ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/gsl/)

################################################################################
# Project definitions
#

find_library(ALGLIB_LIBRARY NAMES alglib PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/alglib/lib NO_DEFAULT_PATH)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(GSLCBLAS_LIBRARY NAMES gslcblas PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MINIGZIP_LIBRARY NAMES minigzip PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/minigzip/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QTEXENGINE_LIBRARY NAMES qtexengine PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qtexengine/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)
find_library(QWTPLOT3D_LIBRARY NAMES qwtplot3d PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/lib NO_DEFAULT_PATH)
find_library(TAMUANOVA_LIBRARY NAMES tamuanova PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/tamuanova/lib NO_DEFAULT_PATH)
find_library(TIFF_LIBRARY NAMES tiff PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/tiff/lib NO_DEFAULT_PATH)
find_library(YAML-CPP_LIBRARY NAMES yaml-cpp PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/yaml-cpp/lib NO_DEFAULT_PATH)

find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL QtNetwork QtSvg QtXml Qt3Support REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(
    ${OPENGL_INCLUDE_DIRS}
#    ${PNG_INCLUDE_DIR}
    ${QtCore_INCLUDE_DIRS}
    ${QtGui_INCLUDE_DIRS}
    ${QtOpenGL_INCLUDE_DIRS}
    ${QtSvg_INCLUDE_DIRS}
    ${QtXml_INCLUDE_DIRS}
    ${Qt3Support_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/alglib/include/alglib/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qtexengine/include/qtexengine/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/qwtplot3d/include/qwtplot3d/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/tamuanova/include/tamuanova/
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/tiff/include/
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
    find_package(PythonInterp 2.7 REQUIRED)
    find_package(PythonLibs 2.7 REQUIRED)
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/python-sipcmd.py
        OUTPUT_VARIABLE sipcommand)
    string(STRIP ${sipcommand} sipcommand)
    execute_process(
        COMMAND bash -c "${sipcommand} -c ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/scripting/qti.sip")
    include_directories(
        ${PYTHON_INCLUDE_DIRS}/
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

SET(SOURCES ${ANALYSIS_SOURCES} ${CORE_SOURCES} ${LIB_SOURCES} ${MATRIX_SOURCES}
        ${PLOT2D_SOURCES} ${PLOT3D_SOURCES} ${SCRIPTING_SOURCES} ${TABLE_SOURCES}
        ${ASCII1D_SOURCES} ${COMMON_SOURCES} ${COMPILE_SOURCES} ${DAN_SOURCES}
        ${FITTABLE_SOURCES} ${JNSE_SOURCES} ${SVD_SOURCES})

SET(HEADERS ${ANALYSIS_HEADERS} ${CORE_HEADERS} ${LIB_HEADERS} ${MATRIX_HEADERS}
        ${PLOT2D_HEADERS} ${PLOT3D_HEADERS} ${SCRIPTING_HEADERS} ${TABLE_HEADERS}
        ${ASCII1D_HEADERS} ${COMMON_HEADERS} ${COMPILE_HEADERS} ${DAN_HEADERS}
        ${FITTABLE_HEADERS} ${JNSE_HEADERS} ${SVD_HEADERS})

SET(UI_SOURCES ${ANALYSIS_UIS} ${CORE_UIS} ${LIB_UIS} ${MATRIX_UIS}
        ${PLOT2D_UIS} ${PLOT3D_UIS} ${SCRIPTING_UIS} ${TABLE_UIS}
        ${ASCII1D_UIS} ${COMMON_UIS} ${COMPILE_UIS} ${DAN_UIS}
        ${FITTABLE_UIS} ${JNSE_UIS} ${SVD_UIS})

qt4_add_resources(RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/icons/icons/icons.qrc)

add_definitions(-DASCII1D=1 -DCOMPILE=1 -DDAN=1 -DFITTABLE=1 -DJNSE=1
    -DHAVE_ALGLIB=1 -DHAVE_TAMUANOVA=1 -DSCRIPTING_MUPARSER=1 -DSVD=1
    -DTEX_OUTPUT=1 -DTIFFTIFF=1 -DYAMLYAML=1)
if(WITH_PYTHON)
    add_definitions(-DSCRIPTING_PYTHON=1 -DSCRIPTING_CONSOLE=1)
endif()

add_executable(qtisas ${SOURCES} ${HEADERS} ${UI_SOURCES} ${RESOURCES})
set_target_properties(qtisas PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(qtisas PUBLIC
    ${ALGLIB_LIBRARY} ${GSL_LIBRARY} ${GSLCBLAS_LIBRARY} ${MINIGZIP_LIBRARY} ${MUPARSER_LIBRARY} ${QTEXENGINE_LIBRARY}
    ${QWT_LIBRARY} ${QWTPLOT3D_LIBRARY} ${TAMUANOVA_LIBRARY} ${TIFF_LIBRARY} ${YAML-CPP_LIBRARY}
    Qt4::QtCore Qt4::QtGui Qt4::QtNetwork Qt4::QtOpenGL Qt4::QtSvg Qt4::QtXml Qt4::Qt3Support
    OpenGL::GLU PNG::PNG ZLIB::ZLIB)
if(WITH_PYTHON)
    target_link_libraries(qtisas PUBLIC ${PYTHON_LIBRARIES})
endif()