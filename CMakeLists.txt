cmake_minimum_required(VERSION 3.24)

project(qtisas
        VERSION 0.9.14
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
# Setting project variables
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
    execute_process(COMMAND powershell -Command "$env:NUMBER_OF_PROCESSORS"
        OUTPUT_VARIABLE CORES OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(CORES 1)
endif()

################################################################################
# Building 3rdparty libs and adding them to the project
#

file(INSTALL FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.ps1
    PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/scripts/)

set(LIBRARIES
    "alglib" "gsl" "minigzip" "muparser" "qtexengine" "qwt" "qwtplot3d"
    "tamuanova" "yaml-cpp")
foreach(lib ${LIBRARIES})
    if(UNIX)
        execute_process(
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.sh ${OS} ${ARCH}
                ${CORES} ${lib} ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/
                ${CMAKE_C_COMPILER} ${CMAKE_CXX_COMPILER} ${CMAKE_PREFIX_PATH})
    elseif(WIN32)
        execute_process(
            COMMAND powershell -ExecutionPolicy Bypass -File
            ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildlib.ps1
            -os ${OS} -arch ${ARCH} -cores ${CORES} -name ${lib}
            -libdir ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/${lib}/
            -cc ${CMAKE_C_COMPILER} -cxx ${CMAKE_CXX_COMPILER}
            -make ${CMAKE_MAKE_PROGRAM} -qt ${CMAKE_PREFIX_PATH}
            -zlib ${ZLIB_ROOT} -png ${PNG_ROOT}
            -gsl ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl
            RESULT_VARIABLE OUTPUT)
    endif()
endforeach()

################################################################################
# Project definitions
#

foreach(lib ${LIBRARIES})
    find_library(${lib}_LIBRARY NAMES ${lib} PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}-${ARCH}/${lib}/lib NO_DEFAULT_PATH)
    set(LINKS ${LINKS} ${${lib}_LIBRARY})
endforeach()

find_package(HDF5 REQUIRED COMPONENTS CXX)
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
    ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/sans/parsers/
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
    ${CMAKE_CURRENT_BINARY_DIR}/qtisas/sans/parsers/
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

if(WITH_PYTHON)
    # to avoid check with platform architecture when both Interpreter and
    # Development modules are specified
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    find_package(Python3 REQUIRED COMPONENTS Development)
    find_program(QMAKE NAMES qmake qmake-qt5)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/buildsip.py
            ${CMAKE_CURRENT_BINARY_DIR} ${QMAKE})
    include_directories(
        ${Python3_INCLUDE_DIRS}/
        ${CMAKE_CURRENT_BINARY_DIR}/sip
        ${CMAKE_CURRENT_BINARY_DIR}/sip/qti)
endif()

################################################################################
# Project definitions
#

set(SUBDIRS "src/analysis" "src/core" "src/lib" "src/matrix" "src/plot2D"
    "src/plot3D" "src/scripting" "src/table" "sans/ascii1d" "sans/common"
    "sans/compile" "sans/dan" "sans/fittable" "sans/jnse" "sans/svd")
foreach(subdir ${SUBDIRS})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtisas/${subdir}/)
    string(FIND "${subdir}" "/" slash REVERSE)
    string(SUBSTRING "${subdir}" ${slash} -1 subdir)
    string(SUBSTRING "${subdir}" 1 -1 subdir)
    set(SOURCES ${SOURCES} ${${subdir}_SOURCES})
    set(HEADERS ${HEADERS} ${${subdir}_HEADERS})
    set(UI_SOURCES ${UI_SOURCES} ${${subdir}_UIS})
endforeach()

qt5_add_resources(RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/icons/icons/icons.qrc)

add_definitions(-DASCII1D=1 -DCOMPILE=1 -DDAN=1 -DFITTABLE=1 -DJNSE=1
    -DHAVE_ALGLIB=1 -DHAVE_TAMUANOVA=1 -DSVD=1 -DTEX_OUTPUT=1
    -DSCRIPTING_MUPARSER=1 -DTIFFTIFF=1 -DYAMLYAML=1)
if(WITH_PYTHON)
    add_definitions(-DSCRIPTING_PYTHON=1 -DSCRIPTING_CONSOLE=1 -DSIP_VER=${sipversion})
endif()

add_executable(qtisas ${SOURCES} ${HEADERS} ${UI_SOURCES} ${RESOURCES})
set_target_properties(qtisas PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(qtisas PUBLIC ${LINKS}
    Qt5::Core Qt5::Network Qt5::OpenGL Qt5::PrintSupport Qt5::Svg Qt5::Widgets Qt5::Xml
    OpenGL::GLU HDF5::HDF5 PNG::PNG TIFF::TIFF ZLIB::ZLIB)
if(WITH_PYTHON)
    target_link_libraries(qtisas PUBLIC ${Python3_LIBRARIES})
endif()

if (APPLE)
    set_target_properties(qtisas PROPERTIES MACOSX_BUNDLE TRUE)
    set(MACOSX_BUNDLE_ICON_FILE "qtisas.icns")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/osx/qtisas.icns"
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/qtisas.icns COPYONLY)
    set_target_properties(qtisas PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/osx/Info.plist")
    if(WITH_PYTHON)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/python-qtiUtil.py"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/python-qtiUtil.py COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qtisasrc.py"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/qtisasrc.py COPYONLY)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qti_wordlist.txt"
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qtisas.app/Contents/Resources/qti_wordlist.txt COPYONLY)
    endif()
endif()

if (LINUX)
    include(GNUInstallDirs)
    install(TARGETS qtisas
        EXPORT qtisasTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    execute_process(COMMAND echo $ENV{HOME} OUTPUT_VARIABLE HOME_DIRECTORY
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qtisasrc.py
        ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/python-qtiUtil.py
        ${CMAKE_CURRENT_SOURCE_DIR}/qtisas/python/qti_wordlist.txt
        DESTINATION
        /usr/share/qtisas/python)
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_PACKAGE_NAME "qtisas")
    set(CPACK_PACKAGE_VERSION ${qtisas_VERSION})
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Forschungszentrum Jullich Gmbh")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5network5, libqt5opengl5, libqt5printsupport5, libqt5svg5, libqt5widgets5, libqt5xml5, libglu1-mesa, libpng16-16, libtiff5, zlib1g, libopengl0, python3-pyqt5")
    if(WITH_PYTHON)
        set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase-gui, qt5-qtsvg, mesa-libGLU, libpng, libtiff, zlib, python3-qt5")
    else()
        set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase-gui, qt5-qtsvg, mesa-libGLU, libpng, libtiff, zlib")
    endif()
    include(CPack)
endif()
