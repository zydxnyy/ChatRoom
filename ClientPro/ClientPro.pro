#-------------------------------------------------
#
# Project created by QtCreator 2018-06-09T10:33:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ClientPro
TEMPLATE = app
LIBS += -lwsock32

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    client.cpp \
    logindialog.cpp \
    registerdialog.cpp \
    MyBuffer.cpp \
    CUtil.cpp \
    json/json_reader.cpp \
    json/json_value.cpp \
    json/json_valueiterator.inl \
    json/json_writer.cpp \
    addfridlg.cpp \
    listwidgetfrirqst.cpp \
    notification.cpp

HEADERS += \
        mainwindow.h \
    client.h \
    global.h \
    logindialog.h \
    registerdialog.h \
    MyBuffer.h \
    CUtil.h \
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
    json/writer.h \
    addfridlg.h \
    listwidgetfrirqst.h \
    notification.h

FORMS += \
        mainwindow.ui \
    logindialog.ui \
    registerdialog.ui \
    addfridlg.ui \
    listwidgetfrirqst.ui \
    notification.ui

DISTFILES += \
    json/json.pri \
    json/CMakeLists.txt
