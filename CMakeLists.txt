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

project(qtisas
        VERSION 0.9.22
        LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
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
    endif ()
elseif (WIN32)
    execute_process(COMMAND powershell -Command "$env:NUMBER_OF_PROCESSORS" OUTPUT_VARIABLE CORES OUTPUT_STRIP_TRAILING_WHITESPACE)
else ()
    set(CORES 1)
endif ()

########################################################################################################################
# Building 3rdparty libs and adding them to the project
#

file(INSTALL FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.ps1
    PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/scripts/)

set(LIBRARIES
    "alglib" "gsl" "minigzip" "muparser" "qtexengine" "qwt" "qwtplot3d" "tamuanova")
foreach (lib ${LIBRARIES})
    if (UNIX)
        execute_process(
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${OS} ${ARCH} ${CORES} ${lib}
                ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/ ${CMAKE_C_COMPILER} ${CMAKE_CXX_COMPILER}
                ${CMAKE_PREFIX_PATH} ${CMAKE_VERSION})
    elseif (WIN32)
        execute_process(
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.ps1
            -os ${OS} -arch ${ARCH} -cores ${CORES} -name ${lib} -libdir ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/
            -cc ${CMAKE_C_COMPILER} -cxx ${CMAKE_CXX_COMPILER} -make ${CMAKE_MAKE_PROGRAM} -qt ${CMAKE_PREFIX_PATH}
            -zlib ${ZLIB_ROOT} -gsl ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl
            RESULT_VARIABLE OUTPUT)
    endif ()
endforeach ()

########################################################################################################################
# Project definitions
#

foreach (lib ${LIBRARIES})
    find_library(${lib}_LIBRARY NAMES ${lib} PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/lib NO_DEFAULT_PATH)
    set(LINKS ${LINKS} ${${lib}_LIBRARY})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/include/)
endforeach ()

find_package(HDF5 REQUIRED COMPONENTS CXX)
find_package(OpenGL REQUIRED)
find_package(Qt5 COMPONENTS Core OpenGL Network PrintSupport Svg Xml Widgets REQUIRED)
find_package(TIFF REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/ascii1d/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/common/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/compile/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/dan-configuration-widgets/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/dan-files-manager/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/dan-parser-header/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/dan-script-table-manager/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/dan/dan-header-devices/
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
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/parsers/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot2D/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot2D/dialogs/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/plot3D/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/scripting/
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/src/table/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/ascii1d/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/common/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/compile/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/dan/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/dan/dan-configuration-widgets/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/dan/dan-files-manager/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/dan/dan-parser-header/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/dan/dan-script-table-manager/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/dan/dan-header-devices/
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
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/parsers/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/plot2D/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/plot2D/dialogs/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/plot3D/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/scripting/
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/src/table/)

if (WITH_PYTHON)
    # troubles on macos sonoma, considers Xcode's python only
    if (${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL "23")
        set(Python3_EXECUTABLE ${Python3_ROOT_DIR}/bin/python3)
        execute_process(COMMAND ${Python3_EXECUTABLE} --version OUTPUT_VARIABLE Python3_VERSION)
        string(REPLACE " " "" Python3_VERSION "${Python3_VERSION}")
        string(REGEX MATCH "Python([0-9]+\\.[0-9]+)" Python3_VERSION "${Python3_VERSION}")
        string(REGEX REPLACE "Python" "" Python3_VERSION "${Python3_VERSION}")
        set(Python3_VERSION "python${Python3_VERSION}")
        set(Python3_INCLUDE_DIRS ${Python3_ROOT_DIR}/include/${Python3_VERSION})
        set(Python3_LIBRARIES ${Python3_ROOT_DIR}/Python3)
    # to avoid check with platform architecture when both Interpreter and
    # Development modules are specified
    elseif (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        find_package(Python3 REQUIRED COMPONENTS Development)
    else ()
        find_package(PythonInterp "3")
        find_package(PythonLibs "3")
        set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
        set(Python3_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
        set(Python3_LIBRARIES ${PYTHON_LIBRARIES})
    endif ()
    find_program(QMAKE NAMES qmake qmake-qt5)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildsip.py ${CMAKE_CURRENT_BINARY_DIR} ${QMAKE})
    include_directories(
        ${Python3_INCLUDE_DIRS}/
        ${CMAKE_CURRENT_BINARY_DIR}/sip
        ${CMAKE_CURRENT_BINARY_DIR}/sip/qti)
endif ()

########################################################################################################################
# Building and linking
#

set(SUBDIRS
    "src/analysis" "src/core" "src/lib" "src/matrix" "src/parsers" "src/plot2D" "src/plot3D" "src/scripting" "src/table"
    "sans/ascii1d" "sans/common" "sans/compile" "sans/dan" "sans/dan/dan-configuration-widgets" "sans/dan/dan-files-manager"
    "sans/dan/dan-parser-header" "sans/dan/dan-script-table-manager" "sans/dan/dan-header-devices" "sans/genmin"
    "sans/fittable" "sans/jnse" "sans/svd")
foreach (subdir ${SUBDIRS})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/${subdir}/)
    string(FIND "${subdir}" "/" slash REVERSE)
    string(SUBSTRING "${subdir}" ${slash} -1 subdir)
    string(SUBSTRING "${subdir}" 1 -1 subdir)
    set(SOURCES ${SOURCES} ${${subdir}_SOURCES})
    set(HEADERS ${HEADERS} ${${subdir}_HEADERS})
    set(UI_SOURCES ${UI_SOURCES} ${${subdir}_UIS})
endforeach ()
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/main.cpp)

qt5_add_resources(RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/icons/icons/icons.qrc)

if (APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION=1)
endif ()
add_definitions(-DQTISAS=1)
if (WITH_PYTHON)
    add_definitions(-DSCRIPTING_PYTHON=1 -DSCRIPTING_CONSOLE=1)
endif ()

add_executable(qtisas ${SOURCES} ${HEADERS} ${UI_SOURCES} ${RESOURCES})
set_target_properties(qtisas PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(qtisas PUBLIC ${LINKS}
    Qt5::Core Qt5::Network Qt5::OpenGL Qt5::PrintSupport Qt5::Svg Qt5::Widgets Qt5::Xml
    OpenGL::GLU TIFF::TIFF ZLIB::ZLIB)
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
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/gsl/)
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
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/gsl/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gsl115/gsl)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib/libgsl.a DESTINATION ${CMAKE_INSTALL_LIBDIR}/ RENAME libgsl115.a)
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_PACKAGE_NAME "qtisas")
    set(CPACK_PACKAGE_VERSION ${qtisas_VERSION})
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Forschungszentrum Jülich GmbH")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    if (WITH_PYTHON)
        set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase-gui, qt5-qtsvg, hdf5, mesa-libGLU, libtiff, yaml-cpp, python3-qt5")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5network5, libqt5opengl5, libqt5printsupport5, libqt5svg5, libqt5widgets5, libqt5xml5, libhdf5-cpp-100 | libhdf5-cpp-103, libtiff5, libyaml-cpp0.5v5 | libyaml-cpp0.6 | libyaml-cpp0.7, python3-pyqt5")
    else ()
        set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase-gui, qt5-qtsvg, hdf5, mesa-libGLU, libtiff, yaml-cpp")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5network5, libqt5opengl5, libqt5printsupport5, libqt5svg5, libqt5widgets5, libqt5xml5, libhdf5-cpp-100 | libhdf5-cpp-103, libtiff5, libyaml-cpp0.5v5 | libyaml-cpp0.6 | libyaml-cpp0.7")
    endif ()
    include(CPack)
endif ()
