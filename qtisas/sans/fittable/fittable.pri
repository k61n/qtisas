
fittable {

    INCLUDEPATH += $${QTISAS_FOLDER}/qtisas/sans/genmin/include

    FORMS += $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable.ui
      
    HEADERS += \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable18.h \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fitting.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/f2c.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/genmin.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/get_options.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/multi_population.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/problem.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/program.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/rule.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/rlsprogram.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/symbol.h \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/include/tolmin.h
    
    SOURCES += \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable18.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-init.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-explorer.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-data.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-fitting.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-fitting-fitting.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-simulate.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fittable-simulate-simulate.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/fitting.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/glaguerre.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/fittable/glegendre.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/genmin.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/get_options.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/multi_population.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/problem.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/program.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/rlsprogram.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/rule.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/symbol.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/tolmin.cc \
        $${QTISAS_FOLDER}/qtisas/sans/genmin/src/localMinGSL.cc
}
