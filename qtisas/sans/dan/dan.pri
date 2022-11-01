###############################################################
################# Module: DAN ############################
###############################################################
dan {
    FORMS 	+= $${QTISAS_FOLDER}/qtisas/sans/dan/dan.ui
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18.h
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18.cpp
    
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-settings.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-init.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-options.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-options-header.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-tools.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-addfiles.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-realtime.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-tof.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-mask.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-sens.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-process.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-process-dandan.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-process-matrix.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-rawdata-extractor.cpp
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/dan18-merge1d.cpp
        
    HEADERS += $${QTISAS_FOLDER}/qtisas/sans/dan/deadTimeRoot.h
    SOURCES += $${QTISAS_FOLDER}/qtisas/sans/dan/deadTimeRoot.cpp
}
