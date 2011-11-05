TARGET = DOMScalp
QT += core gui network xml sql
CONFIG += warn_on console

LEVEL = ..

!include($$LEVEL/ADScalp.pri):error("Can't load ADScalp.pri")
!include($$LEVEL/AlfaDirectAPI/AlfaDirectAPI.deps):error("Can't load AlfaDirectAPI.deps")


TEMPLATE = app

HEADERS += \
           ADMainWindow.h \
           ADMainWindow.h \
           ADTableView.h

SOURCES += \
           Main.cpp \
           ADMainWindow.cpp \
           ADTableView.cpp

FORMS += \
          ADMainWindow.ui

