TEMPLATE = app
TARGET = dialogbox
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += dialogbox.hpp
SOURCES += dialogbox.cpp dialogparser.cpp dialogmain.cpp dialogsetoptions.cpp dialogslots.cpp dialogprivate.cpp

# install recipe options
target.path = /usr/bin
INSTALLS += target

# Added C/C++ compillers' options
# QMAKE_CXXFLAGS += -fpermissive -Wno-parentheses -Wno-switch -Wno-write-strings
