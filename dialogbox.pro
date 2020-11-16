TEMPLATE = app
CONFIG += qt thread release
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

TARGET = dialogbox
VPATH = src
DESTDIR = dist
OBJECTS_DIR = obj
MOC_DIR = $$OBJECTS_DIR

# Input
HEADERS += dialogbox.h
SOURCES += dialogbox.cc \
           dialog_parser.cc \
           dialog_main.cc \
           dialog_set_options.cc \
           dialog_slots.cc \
           dialog_private.cc

# install recipe options
target.path = /usr/bin
INSTALLS += target

# Added C/C++ compiler options
QMAKE_CXXFLAGS += --std=c++11
