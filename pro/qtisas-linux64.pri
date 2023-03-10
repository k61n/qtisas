
LIBS += -lGLU
LIBS += -lz
LIBS += -lpng

SYS_INCLUDEPATH += /usr/include

equals(ARCH, "aarch64") {
    SYS_LIBS += -L/usr/lib/aarch64-linux-gnu
} else {
    equals(ARCH, "amd64") {
        SYS_LIBS += -L/usr/lib/x86_64-linux-gnu
    }
}
