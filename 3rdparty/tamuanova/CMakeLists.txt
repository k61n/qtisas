cmake_minimum_required(VERSION 3.0)

project(tamuanova C)

set(OS ${CMAKE_SYSTEM_NAME})
set(ARCH ${CMAKE_SYSTEM_PROCESSOR})

include_directories(${GSL_HEADERS})

set(SOURCES
    anova_1.c
    anova_2.c)

set(HEADERS
    tamu_anova.h)

add_library(tamuanova ${SOURCES} ${HEADERS})
target_include_directories(tamuanova PUBLIC include)

include(GNUInstallDirs)
install(TARGETS tamuanova
    EXPORT tamuanovaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tamuanova)
