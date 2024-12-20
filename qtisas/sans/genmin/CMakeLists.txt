
set(SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/genmin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/get_options.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/localMinGSL.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/multi_population.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/problem.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/program.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rlsprogram.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rule.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/symbol.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tolmin.cc)

set(HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/f2c.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/genmin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/get_options.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/multi_population.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/problem.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/program.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rlsprogram.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rule.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/symbol.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/tolmin.h)

set(INCLUDEDIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/)

set(genmin_SOURCES ${SOURCES} PARENT_SCOPE)
set(genmin_HEADERS ${HEADERS} PARENT_SCOPE)
set(genmin_INCLUDEDIRS ${INCLUDEDIRS} PARENT_SCOPE)
