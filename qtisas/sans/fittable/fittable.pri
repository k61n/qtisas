

###############################################################
################# Module: FITTABLE ############################
###############################################################
fittable {
    
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/fittable/fitting.h
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fitting.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/glaguerre.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/glegendre.cpp
    
    ##########################
    #### GenMin           ####
    ##########################
    INCLUDEPATH += $${QTISAS_FOLDER}/qtisas/sans/genmin/include
    
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/f2c.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/genmin.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/get_options.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/multi_population.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/problem.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/program.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/rule.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/rlsprogram.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/symbol.h
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/genmin/include/tolmin.h
    
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/genmin.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/get_options.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/multi_population.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/problem.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/program.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/rlsprogram.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/rule.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/symbol.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/tolmin.cc
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/genmin/src/localMinGSL.cc
    
    
    FORMS     += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable.ui
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable18.h
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable18.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-init.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-explorer.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-data.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-fitting.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-fitting-fitting.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-simulate.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-simulate-simulate.cpp
}



