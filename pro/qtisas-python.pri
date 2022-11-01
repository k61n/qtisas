
###############################################################
## Scripting: PYTHON + SIP + PyQT #############################
###############################################################

###############################################################
## python - only used if python is needed #####################
###############################################################

# the python interpreter to use
# (unix only, windows will use what ever is configured to execute .py files!)
PYTHON = python

INSTALLS += pythonconfig
pythonconfig.files    += qtisasrc.py \
python-qtiUtil.py \
qti_wordlist.txt \

unix: pythonconfig.path = $ORIGIN/../python
win32: pythonconfig.path = $ORIGIN/../python
#   DEFINES += PYTHON_CONFIG_PATH="\\\"$$replace(pythonconfig.path," ","\ ")\\\"

unix {
    isEmpty(PYTHON): PYTHON = python
    INCLUDEPATH += $$system($$PYTHON python-includepath.py)
    LIBS        += $$system($$PYTHON -c "\"from distutils import sysconfig; print '-lpython'+sysconfig.get_config_var('VERSION')\"")
    LIBS        += -lm
    system(mkdir -p $${SIP_DIR})
    system($$system($$PYTHON python-sipcmd.py) -c $${SIP_DIR} ../qtisas/src/scripting/qti.sip)
}

win32 {
    INCLUDEPATH += $$system(call python-includepath.py)
    LIBS        += $$system(call python-libs-win.py)
    system($$system(call python-sipcmd.py) -c $${SIP_DIR} ../qtisas/src/scripting/qti.sip)
}
