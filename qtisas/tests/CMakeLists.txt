# QtiSAS tests, enabled with -DBUILD_TESTING=ON (see the top-level CMakeLists.txt)

# Python tests run by qtisas itself (-x --quit: exit code = result).
# A script that cannot run here (e.g. a missing fit function) prints QTISAS_TEST_SKIPPED and is reported as skipped.
# RESOURCE_LOCK: parallel qtisas instances share settings and the fit-function library, so ctest -j runs them one at
# a time.
function(qtisas_python_test name script)
    add_test(NAME ${name} COMMAND $<TARGET_FILE:qtisas> -x --quit ${script})
    set(environment ${ARGN})
    if (UNIX)
        list(APPEND environment "QT_QPA_PLATFORM=offscreen")
    endif ()
    set_tests_properties(${name} PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} TIMEOUT 120
                         SKIP_REGULAR_EXPRESSION "QTISAS_TEST_SKIPPED" RESOURCE_LOCK qtisas
                         ENVIRONMENT "${environment}")
endfunction()

if (WITH_PYTHON)
    # every *_test.py below this folder
    file(GLOB_RECURSE PYTHON_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*_test.py)
    foreach (script ${PYTHON_TESTS})
        get_filename_component(name ${script} NAME_WE)
        qtisas_python_test(${name} ${script})
    endforeach ()

    # NIST StRD nonlinear regression: one test per function of the library's NIST/ folder (nist_fit.py)
    set(NIST_FUNCTIONS
        Bennett5 BoxBOD Chwirut1 Chwirut2 DanWood ENSO Eckerle4 Gauss1 Gauss2 Gauss3 Hahn1 Kirby2 Lanczos1 Lanczos2
        Lanczos3 MGH09 MGH10 MGH17 Misra1a Misra1b Misra1c Misra1d Rat42 Rat43 Roszman1 Thurber)
    foreach (function ${NIST_FUNCTIONS})
        string(TOLOWER ${function} lower)
        qtisas_python_test(nist_${lower} ${CMAKE_CURRENT_SOURCE_DIR}/sans/fittable/nist_fit.py NIST_FUNCTION=${function})
    endforeach ()
endif ()

# C++ unit tests need Qt's Test component; without it only they are skipped, qtisas still builds
find_package(Qt${V} COMPONENTS Test QUIET)
if (NOT Qt${V}Test_FOUND)
    message(STATUS "Qt${V}::Test not found — skipping C++ unit tests")
    return()
endif ()

if (WITH_PYTHON)
    add_executable(ai_content_tests src/ai/ai_content_tests.cpp src/ai/ai_content_tests.h
        ${CMAKE_SOURCE_DIR}/qtisas/src/ai/AiAssistant.cpp
        ${CMAKE_SOURCE_DIR}/qtisas/src/ai/QtiSasAiContent.cpp)
    target_include_directories(ai_content_tests PRIVATE ${CMAKE_SOURCE_DIR}/qtisas/src/ai)
    target_link_libraries(ai_content_tests PRIVATE Qt${V}::Core Qt${V}::Network Qt${V}::Test)
    set_target_properties(ai_content_tests PROPERTIES AUTOMOC ON)
    add_test(NAME ai_content_tests COMMAND ai_content_tests)
endif ()

add_executable(table_numeric_locale_tests src/table/table_numeric_locale_tests.cpp src/table/table_numeric_locale_tests.h
    ${CMAKE_SOURCE_DIR}/qtisas/src/table/numericLocale.cpp)
target_include_directories(table_numeric_locale_tests PRIVATE ${CMAKE_SOURCE_DIR}/qtisas/src/table)
target_link_libraries(table_numeric_locale_tests PRIVATE Qt${V}::Core Qt${V}::Test)
set_target_properties(table_numeric_locale_tests PROPERTIES AUTOMOC ON)
add_test(NAME table_numeric_locale_tests COMMAND table_numeric_locale_tests)

add_executable(graph3d_log_scale_tests src/plot3D/graph3d_log_scale_tests.cpp src/plot3D/graph3d_log_scale_tests.h
    ${CMAKE_SOURCE_DIR}/qtisas/src/plot3D/Graph3DLogScale.h)
target_include_directories(graph3d_log_scale_tests PRIVATE ${CMAKE_SOURCE_DIR}/qtisas/src/plot3D)
target_link_libraries(graph3d_log_scale_tests PRIVATE Qt${V}::Core Qt${V}::Test)
set_target_properties(graph3d_log_scale_tests PROPERTIES AUTOMOC ON)
add_test(NAME graph3d_log_scale_tests COMMAND graph3d_log_scale_tests)

add_executable(smart_completer_tests src/scripting/smart_completer_tests.cpp
    src/scripting/smart_completer_tests.h
    ${CMAKE_SOURCE_DIR}/qtisas/src/scripting/SmartCompleter.cpp
    ${CMAKE_SOURCE_DIR}/qtisas/src/scripting/SmartCompleter.h)
target_include_directories(smart_completer_tests PRIVATE ${CMAKE_SOURCE_DIR}/qtisas/src/scripting)
target_link_libraries(smart_completer_tests PRIVATE Qt${V}::Widgets Qt${V}::Test)
set_target_properties(smart_completer_tests PROPERTIES AUTOMOC ON)
add_test(NAME smart_completer_tests COMMAND smart_completer_tests)
if (UNIX)
    set_tests_properties(smart_completer_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif ()

add_executable(dan_polarization_corrections_tests sans/dan/dan_polarization_corrections_tests.cpp
    sans/dan/dan_polarization_corrections_tests.h
    ${CMAKE_SOURCE_DIR}/qtisas/sans/dan/dan-polarization/dan-polarization-corrections.cpp)
target_include_directories(dan_polarization_corrections_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/qtisas/sans/dan/dan-polarization)
target_link_libraries(dan_polarization_corrections_tests PRIVATE Qt${V}::Core Qt${V}::Test GSL::gsl GSL::gslcblas)
set_target_properties(dan_polarization_corrections_tests PROPERTIES AUTOMOC ON)
add_test(NAME dan_polarization_corrections_tests COMMAND dan_polarization_corrections_tests)
