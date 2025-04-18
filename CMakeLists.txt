cmake_minimum_required(VERSION 3.10)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.11")
    cmake_policy(SET CMP0072 NEW)
endif ()
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
    cmake_policy(SET CMP0074 NEW)
endif ()
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.27")
    cmake_policy(SET CMP0144 NEW)
endif ()

project(qtisas VERSION 0.12.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

########################################################################################################################
# Setting project variables
#

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

if (UNIX)
    if (APPLE)
        execute_process(COMMAND "sysctl" "-n" "hw.ncpu" OUTPUT_VARIABLE CORES)
    else ()
        execute_process(COMMAND "nproc" OUTPUT_VARIABLE CORES)
        if("$ENV{CI}" STREQUAL "true")
            set(CORES 4)
        endif()
    endif ()
elseif (WIN32)
    execute_process(COMMAND powershell -Command "$env:NUMBER_OF_PROCESSORS" OUTPUT_VARIABLE CORES OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
    set(CORES 1)
endif ()
message(STATUS "Detecting system: OS ${OS}; arch ${ARCH}; cores ${CORES}")

if (NOT CMAKE_C_COMPILER)
    if (UNIX)
        set(CMAKE_C_COMPILER "/usr/bin/gcc")
    elseif (WIN32)
        message(FATAL_ERROR "Set the c compiler passing -DCMAKE_C_COMPILER argument")
    endif ()
endif ()
if (NOT CMAKE_CXX_COMPILER)
    if (UNIX)
        set(CMAKE_CXX_COMPILER "/usr/bin/g++")
    elseif (WIN32)
        message(FATAL_ERROR "Set the c++ compiler passing -DCMAKE_CXX_COMPILER argument")
    endif ()
endif ()
message(STATUS "Detecting generator ${CMAKE_GENERATOR}: ${CMAKE_MAKE_PROGRAM}")

########################################################################################################################
# Lib imports
#

if (${PREFER_QT})
    if (${PREFER_QT} EQUAL 6)
        find_package(Qt6 COMPONENTS Core DBus Network OpenGLWidgets PrintSupport Svg Xml Widgets REQUIRED)
        set(V 6)
    elseif (${PREFER_QT} EQUAL 5)
        find_package(Qt5 COMPONENTS Core Network OpenGL PrintSupport Svg Xml Widgets REQUIRED)
        set(V 5)
    endif ()
else ()
    find_package(Qt6 COMPONENTS Core DBus Network OpenGLWidgets PrintSupport Svg Xml Widgets QUIET)
    if (${Qt6_FOUND})
        set(V 6)
    else ()
        find_package(Qt5 COMPONENTS Core Network OpenGL PrintSupport Svg Xml Widgets REQUIRED)
        set(V 5)
    endif ()
endif ()
get_target_property(QT_LIB_PATH Qt${V}::Core LOCATION)
message(STATUS "Found Qt${V}Core: ${QT_LIB_PATH}")

find_library(gl2ps_LIBRARY NAMES gl2ps
    HINTS ${gl2ps_ROOT}/lib /usr/lib /usr/lib/aarch64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/local/lib)
include_directories(${gl2ps_ROOT}/include/)
message(STATUS "Found gl2ps: ${gl2ps_LIBRARY}")
find_package(GSL REQUIRED)
find_package(HDF5 REQUIRED COMPONENTS CXX)
find_package(muparser QUIET)
# compatibility with muparser < 2.3
if (NOT muparser_FOUND)
    find_library(muparser_LIBRARY NAMES muparser)
    add_library(muparser::muparser SHARED IMPORTED)
    set_target_properties(muparser::muparser PROPERTIES IMPORTED_LOCATION ${muparser_LIBRARY})
endif ()
get_target_property(MU_LIB_PATH muparser::muparser LOCATION)
message(STATUS "Found muparser: ${MU_LIB_PATH}")
find_package(OpenGL REQUIRED)
find_package(TIFF REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(ZLIB REQUIRED)

# Building 3rdparty libs
file(INSTALL FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.ps1
        PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/scripts/)
set(LIBRARIES
        "alglib" "minigzip" "qtexengine" "qwt" "qwtplot3d" "tamuanova")
foreach (lib ${LIBRARIES})
    if (UNIX)
        execute_process(
                COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${OS} ${ARCH} ${CORES} ${lib}
                ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/ ${CMAKE_GENERATOR} ${CMAKE_MAKE_PROGRAM}
                ${CMAKE_C_COMPILER} ${CMAKE_CXX_COMPILER}
                ${CMAKE_VERSION} ${GSL_ROOT} ${gl2ps_ROOT} "${CMAKE_PREFIX_PATH}" ${PREFER_QT})
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
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/lib NO_DEFAULT_PATH)
    message(STATUS "Found ${lib}: ${${lib}_LIBRARY}")
    set(LINKS ${LINKS} ${${lib}_LIBRARY})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/include/)
endforeach ()

########################################################################################################################
# Project definitions
#

if (WITH_PYTHON)
    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
        # to avoid check with platform architecture when both Interpreter and
        # Development modules are specified
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        find_package(Python3 REQUIRED COMPONENTS Development)
    else ()
        find_package(PythonInterp "3")
        find_package(PythonLibs "3")
        set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
        set(Python3_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
        set(Python3_LIBRARIES ${PYTHON_LIBRARIES})
    endif ()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        find_program(QMAKE NAMES qmake6 qmake-qt5 qmake)
    elseif (APPLE)
        # need to climb up directories 
        set(CONTINUE_LOOP ON)
        while (CONTINUE_LOOP)
            get_filename_component (PARENT_DIR "${QT_LIB_PATH}" DIRECTORY)
            if (PARENT_DIR STREQUAL QT_LIB_PATH)
                break ()
            endif ()
            set (QT_LIB_PATH "${PARENT_DIR}")
            get_filename_component (DIRNAME "${QT_LIB_PATH}" NAME)
            if (DIRNAME STREQUAL "qt" OR DIRNAME STREQUAL "qt@6" OR DIRNAME STREQUAL "qt@5")
                set(CONTINUE_LOOP OFF)
            endif ()
        endwhile ()
        find_program(QMAKE NAMES qmake HINTS ${QT_LIB_PATH}/bin NO_DEFAULT_PATH)
    else (WIN32)
        get_filename_component (QT_LIB_PATH "${QT_LIB_PATH}" DIRECTORY)
        find_program(QMAKE NAMES qmake HINTS ${QT_LIB_PATH} NO_DEFAULT_PATH)
    endif ()
    message(STATUS "Found qmake: ${QMAKE}")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildsip.py ${CMAKE_CURRENT_BINARY_DIR} ${QMAKE} ${V})
    include_directories(
        ${Python3_INCLUDE_DIRS}/
        ${CMAKE_CURRENT_BINARY_DIR}/sip
        ${CMAKE_CURRENT_BINARY_DIR}/sip/qti)
endif ()

set(SUBDIRS
    "src/analysis" "src/core" "src/lib" "src/matrix" "src/parsers" "src/plot2D" "src/plot3D" "src/scripting" "src/table"
    "sans/ascii1d" "sans/common" "sans/compile" "sans/dan" "sans/genmin" "sans/fittable" "sans/jnse" "sans/svd")
foreach (subdir ${SUBDIRS})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/${subdir}/)
    # reduces subdir from relative path to submodule name
    string(FIND "${subdir}" "/" slash REVERSE)
    string(SUBSTRING "${subdir}" ${slash} -1 subdir)
    string(SUBSTRING "${subdir}" 1 -1 subdir)
    set(SOURCES ${SOURCES} ${${subdir}_SOURCES})
    set(HEADERS ${HEADERS} ${${subdir}_HEADERS})
    set(UI_SOURCES ${UI_SOURCES} ${${subdir}_UIS})
    include_directories(${${subdir}_INCLUDEDIRS})
endforeach ()
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/main.cpp)

if (V EQUAL 6)
    qt6_add_resources(RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/icons/icons/icons.qrc)
else ()
    qt5_add_resources(RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/icons/icons/icons.qrc)
endif ()

add_definitions(-DQTISAS=1)

if (APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION=1)
endif ()

if (WITH_PYTHON)
    add_definitions(-DSCRIPTING_PYTHON=1 -DSCRIPTING_CONSOLE=1)
endif ()

if (MSVC)
    # for muparser
    add_compile_definitions(NOMINMAX)
    # to properly import GSL
    add_compile_definitions(GSL_DLL)
    # to enable parallel build
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
endif ()

########################################################################################################################
# Linking
#

add_executable(qtisas ${SOURCES} ${HEADERS} ${UI_SOURCES} ${RESOURCES})
set_target_properties(qtisas PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(qtisas PUBLIC ${LINKS}
    Qt${V}::Core Qt${V}::Network Qt${V}::PrintSupport Qt${V}::Svg Qt${V}::Widgets Qt${V}::Xml
    ${gl2ps_LIBRARY} GSL::gsl GSL::gslcblas muparser::muparser OpenGL::GLU TIFF::TIFF ZLIB::ZLIB)
if (V EQUAL 6)
    target_link_libraries(qtisas PUBLIC Qt6::DBus Qt6::OpenGLWidgets)
else ()
    target_link_libraries(qtisas PUBLIC Qt5::OpenGL)
endif ()
if (TARGET HDF5::HDF5)
    target_link_libraries(qtisas PUBLIC HDF5::HDF5)
else ()
    include_directories(${HDF5_C_INCLUDE_DIRS} ${HDF5_CXX_INCLUDE_DIRS})
    target_link_libraries(qtisas PUBLIC ${HDF5_LIBRARIES})
endif ()
if (TARGET yaml-cpp::yaml-cpp)
    target_link_libraries(qtisas PUBLIC yaml-cpp::yaml-cpp)
else ()
    target_link_libraries(qtisas PUBLIC yaml-cpp)
endif ()
if(WITH_PYTHON)
    target_link_libraries(qtisas PUBLIC ${Python3_LIBRARIES})
endif()
# In Qt6 in win32 UNICODE is set, that messes up with muparser
if (WIN32 AND V EQUAL 6)
    qt_disable_unicode_defines(qtisas)
endif ()
# Disable cmd line in Windows when running the app
if (WIN32)
    target_link_options(qtisas PRIVATE -mwindows)
endif ()

########################################################################################################################
# Bundling
#

if (APPLE)
    set_target_properties(qtisas PROPERTIES MACOSX_BUNDLE TRUE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(MACOSX_BUNDLE_ICON_FILE "qtisas.icns")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/osx/qtisas.icns"
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/qtisas.icns COPYONLY)
    set_target_properties(qtisas PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/osx/Info.plist")
    if (WITH_PYTHON)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/python-qtiUtil.py"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python/python-qtiUtil.py COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qtisasrc.py"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python/qtisasrc.py COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qti_wordlist.txt"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python/qti_wordlist.txt COPYONLY)
        file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python-scripts)
    endif ()
    file(COPY ${GSL_INCLUDE_DIRS}/gsl/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/gsl/)
    foreach(SYMLINK ${GSL_LIBRARIES})
        get_filename_component(LIBRARY "${SYMLINK}" REALPATH)
        file(COPY ${LIBRARY} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Frameworks/)
    endforeach()
endif ()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    include(GNUInstallDirs)
    install(TARGETS qtisas
        EXPORT qtisasTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qtisasrc.py
        ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/python-qtiUtil.py
        ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qti_wordlist.txt
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/qtisas/python)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/icons/qtisas_logo.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/qtisas)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/linux/qtisas.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
endif ()
