
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ApplicationWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateBinMatrixDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomActionDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FindDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Folder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportASCIIDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MdiSubWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenProjectDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotWizard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QtiPlotApplication.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RenameWindowDialog.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/ApplicationWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/compat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CreateBinMatrixDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomActionDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FindDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Folder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/globals.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportASCIIDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExportPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MdiSubWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenProjectDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlotWizard.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QtiPlotApplication.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RenameWindowDialog.h)

set(core_SOURCES ${SOURCES} PARENT_SCOPE)
set(core_HEADERS ${HEADERS} PARENT_SCOPE)
