
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ColorMapDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixResamplingDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixSizeDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixValuesDialog.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/ColorMapDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Matrix.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixCommand.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixResamplingDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixSizeDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MatrixValuesDialog.h)

set(MATRIX_SOURCES ${SOURCES} PARENT_SCOPE)
set(MATRIX_HEADERS ${HEADERS} PARENT_SCOPE)
