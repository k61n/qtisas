
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

find_package(Qt4 COMPONENTS QtGui Qt3Support REQUIRED)
find_library(GSL_LIBRARY NAMES gsl PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/lib NO_DEFAULT_PATH)
find_library(MUPARSER_LIBRARY NAMES muparser PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/lib NO_DEFAULT_PATH)
find_library(QWT_LIBRARY NAMES qwt PATHS ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/lib NO_DEFAULT_PATH)

include_directories(
#   ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/gsl/include/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/muparser/include/muparser/
    ${CMAKE_SOURCE_DIR}/libs/${OS}-${ARCH}/qwt/include/qwt/

    ../analysis/
    ../core/
    ../lib/include/
    ../matrix/
    ../plot2D/
    ../plot2D/dialogs/
    ../table/

    ../../sans/compile/

    ${CMAKE_CURRENT_BINARY_DIR}/../../sans/compile/)

set(SOURCES
    FindReplaceDialog.cpp
    muParserScript.cpp
    muParserScripting.cpp
    MyParser.cpp
    Note.cpp
#    PythonScript.cpp
#    PythonScripting.cpp
    PythonSyntaxHighlighter.cpp
    Script.cpp
    ScriptEdit.cpp
    ScriptingEnv.cpp
    ScriptingLangDialog.cpp
    ScriptWindow.cpp
)

set(HEADERS
    customevents.h
    FindReplaceDialog.h
    muParserScript.h
    muParserScripting.h
    MyParser.h
    Note.h
#    PythonScript.h
#    PythonScripting.h
    PythonSyntaxHighlighter.h
    Script.h
    ScriptEdit.h
    ScriptingEnv.h
    ScriptingLangDialog.h
    ScriptWindow.h)

QT4_WRAP_CPP(MOCS ${HEADERS})

add_definitions(-DCOMPILE=1 -DSCRIPTING_MUPARSER=1)

add_library(scripting STATIC ${SOURCES} ${MOCS})
add_dependencies(scripting compile)
target_link_libraries(scripting PUBLIC
    ${GSL_LIBRARY} ${MUPARSER_LIBRARY} ${QWT_LIBRARY}
    Qt4::QtGui Qt4::Qt3Support)
