TEMPLATE = subdirs

LEVEL = .

!include($$LEVEL/ADScalp.pri):error("Can't load ADOption.pri")

SUBDIRS += \
           AlfaDirectAPI \
           src

# build must be last:
CONFIG += ordered
