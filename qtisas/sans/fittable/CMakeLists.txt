
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/genmin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/get_options.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/localMinGSL.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/multi_population.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/problem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/program.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/rlsprogram.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/rule.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/symbol.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/src/tolmin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable18.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-explorer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-fitting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-fitting-fitting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-simulate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable-simulate-simulate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fitting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/glaguerre.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/glegendre.cpp)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/f2c.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/genmin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/get_options.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/multi_population.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/problem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/program.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/rlsprogram.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/rule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/symbol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../genmin/include/tolmin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable18.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fitting.h)

set(UI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/fittable.ui)

set(FITTABLE_SOURCES ${SOURCES} PARENT_SCOPE)
set(FITTABLE_HEADERS ${HEADERS} PARENT_SCOPE)
set(FITTABLE_UIS ${UI_SOURCES} PARENT_SCOPE)
