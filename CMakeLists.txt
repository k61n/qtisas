cmake_minimum_required(VERSION 3.18)

cmake_policy(SET CMP0072 NEW)
cmake_policy(SET CMP0074 NEW)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.27")
    cmake_policy(SET CMP0144 NEW)
endif ()

project(qtisas VERSION 0.14.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

########################################################################################################################
# Setting build environment
#

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

if (APPLE)
    if (NOT CMAKE_OSX_SYSROOT)
        set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
    endif ()
    message(STATUS "Detecting macOS SDK: ${CMAKE_OSX_SYSROOT}")
endif ()

########################################################################################################################
# Lib imports
#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(GSL REQUIRED)      # used by qtisas and tamuanova
find_package(Hdf REQUIRED )
find_package(Muparser REQUIRED)
find_package(Qt REQUIRED)
find_package(TIFF REQUIRED)
find_package(YamlCpp REQUIRED)
find_package(ZLIB REQUIRED)     # used by dan module and minigzip

find_package(BuiltinLibs REQUIRED)

if (WITH_PYTHON)
    find_package(Qmake REQUIRED)
    find_package(Python REQUIRED)
endif ()

########################################################################################################################
# Project definitions
#

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
foreach (module IN LISTS QT_MODULES)
    target_link_libraries(qtisas PUBLIC Qt${V}::${module})
endforeach ()
target_link_libraries(qtisas PUBLIC
    ${BUILTIN_LIBS}
    GSL::gsl GSL::gslcblas
    ${HDF5_LIBRARIES}
    ${muparser_LIBRARY}
    TIFF::TIFF
    ${yaml-cpp_LIBRARY}
    ZLIB::ZLIB)

if(WITH_PYTHON)
    target_link_libraries(qtisas PUBLIC ${Python3_LIBRARIES})
endif()
# In Qt6 in win32 UNICODE is set, that messes up with muparser
if (WIN32 AND V EQUAL 6)
    qt_disable_unicode_defines(qtisas)
endif ()
# Disable cmd line in Windows when running the app
if (WIN32)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(qtisas PRIVATE -mwindows)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC|Clang")
        target_link_options(qtisas PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    endif()
endif ()

########################################################################################################################
# Bundling
#

if (APPLE)
    set_target_properties(qtisas PROPERTIES MACOSX_BUNDLE TRUE)
    set(MACOSX_BUNDLE_ICON_FILE "qtisas.icns")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/install/osx/qtisas.icns"
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/qtisas.icns COPYONLY)
    set_target_properties(qtisas PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/install/osx/Info.plist")
    if (WITH_PYTHON)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/python-qtiUtil.py"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python/python-qtiUtil.py COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qtisasrc.py"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python/qtisasrc.py COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qti_wordlist.txt"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python/qti_wordlist.txt COPYONLY)
        file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python-scripts)
    endif ()
    get_filename_component(GSL_INCLUDE_DIR "${GSL_INCLUDE_DIRS}/gsl" REALPATH)
    file(COPY ${GSL_INCLUDE_DIR} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/)
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
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/install/linux/qtisas.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
endif ()
