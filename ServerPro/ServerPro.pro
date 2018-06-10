TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

LIBS += -lwsock32
QT += sql
QTPLUGIN  += qsqlite

SOURCES += \
        main.cpp \
    server.cpp \
    CUtil.cpp \
    MyBuffer.cpp \
    json/json_reader.cpp \
    json/json_value.cpp \
    json/json_valueiterator.inl \
    json/json_writer.cpp

HEADERS += \
    server.h \
    CUtil.h \
    MyBuffer.h \
    global.h \
    json/allocator.h \
    json/assertions.h \
    json/autolink.h \
    json/config.h \
    json/features.h \
    json/forwards.h \
    json/json.h \
    json/json_tool.h \
    json/reader.h \
    json/value.h \
    json/version.h \
    json/version.h.in \
    json/writer.h

DISTFILES += \
    json/CMakeLists.txt
