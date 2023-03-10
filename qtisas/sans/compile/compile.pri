
compile {
    FORMS += $${QTISAS_FOLDER}/qtisas/sans/compile/compile.ui

    HEADERS += \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile18.h \
        $${QTISAS_FOLDER}/qtisas/sans/compile/highlighter.h

    SOURCES += \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile18.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile-init.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile-settings.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile-explorer.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile-parameters.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile-info.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/compile/compile-code-menu.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/compile/highlighter.cpp
}
