QT += widgets

HEADERS       = window.h \
    findworker.h
SOURCES       = main.cpp \
                window.cpp \
    findworker.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/findfiles
INSTALLS += target
