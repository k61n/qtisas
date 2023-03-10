
dan {
    FORMS 	+= $${QTISAS_FOLDER}/qtisas/sans/dan/dan.ui

    HEADERS += \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18.h \
        $${QTISAS_FOLDER}/qtisas/sans/dan/deadTimeRoot.h
        
    SOURCES += \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-settings.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-init.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-options.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-options-header.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-tools.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-addfiles.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-realtime.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-tof.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-mask.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-sens.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-process.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-process-dandan.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-process-matrix.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-extractor.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-merge1d.cpp \
        $${QTISAS_FOLDER}/qtisas/sans/dan/deadTimeRoot.cpp
}
