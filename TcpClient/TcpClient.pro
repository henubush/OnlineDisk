#-------------------------------------------------
#
# Project created by QtCreator 2017-08-07T14:44:58
#
#-------------------------------------------------

QT       += core gui
QT 	+= network
#CONFIG  += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TcpClient
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
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
    md5.cpp \
    upload_dialog.cpp \
    upload.cpp \
    download_dialog.cpp \
    download.cpp \
    QRoundProgressBar.cpp

HEADERS += \
        mainwindow.h \
    md5.h \
    upload_dialog.h \
    upload.h \
    download_dialog.h \
    download.h \
    QRoundProgressBar.h

FORMS += \
        mainwindow.ui \
    upload_dialog.ui \
    download_dialog.ui
