###############################################################
################# Module: COMPILE #############################
###############################################################
compile {
    FORMS 	+= $${QTISAS_FOLDER}/qtisas/sans/compile/compile.ui
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/compile/compile18.h
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/compile18.cpp

    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/compile-init.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/compile-settings.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/compile-explorer.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/compile-parameters.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/compile-info.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/compile-code-menu.cpp
    
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/compile/highlighter.h
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/compile/highlighter.cpp
}
