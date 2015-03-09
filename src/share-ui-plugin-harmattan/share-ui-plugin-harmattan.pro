TEMPLATE = lib

OBJECTS_DIR = .obj
MOC_DIR = .moc

TARGET = notekeeper-open-share-plugin

CONFIG += mdatauri share-ui-plugin meegotouch

# CONFIG += mdatauri shareui share-ui-plugin share-ui-common qt plugin link_pkgconfig

HEADERS += \
    notekeepershareuimethod.h \
    notekeepershareuiplugin.h

SOURCES += \
    notekeepershareuimethod.cpp \
    notekeepershareuiplugin.cpp

contains(MEEGO_EDITION,harmattan) {
# To enable Build > Deploy to work in Qt Creator
contains(CONFIG, debug) {
    target.path = /usr/lib/share-ui/plugins
    INSTALLS += target
}
}
