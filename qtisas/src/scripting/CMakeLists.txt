
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/FindReplaceDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScripting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MyParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Note.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/PythonScript.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/PythonScripting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PythonSyntaxHighlighter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Script.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptEdit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingEnv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingLangDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptWindow.cpp
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/customevents.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FindReplaceDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScript.h
    ${CMAKE_CURRENT_SOURCE_DIR}/muParserScripting.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MyParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Note.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/PythonScript.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/PythonScripting.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PythonSyntaxHighlighter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Script.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptEdit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingEnv.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptingLangDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ScriptWindow.h)

set(SCRIPTING_SOURCES ${SOURCES} PARENT_SCOPE)
set(SCRIPTING_HEADERS ${HEADERS} PARENT_SCOPE)
