INCLUDEPATH += $$PWD

HEADERS += $$PWD/evernotesync.h $$PWD/evernotemarkup.h $$PWD/edamutils.h \
    $$PWD/richtextnotecss.h \
    $$PWD/evernoteoauth.h \
    cloud/evernote/evernotesync/html_named_entities.h
SOURCES += $$PWD/evernotesync.cpp $$PWD/evernotemarkup.cpp $$PWD/edamutils.cpp \
    $$PWD/evernoteoauth.cpp

!simulator {  # Thrift doesn't compile for the Simulator
    include(evernote_edam_3rdparty.pri)
    include(thrift_3rdparty.pri)
    include(boost_3rdparty.pri)
    HEADERS += $$PWD/qthrifthttpclient.h $$PWD/evernoteaccess.h
    SOURCES += $$PWD/qthrifthttpclient.cpp $$PWD/evernoteaccess.cpp
}
