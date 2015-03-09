EVERNOTE_API_PATH = $$PWD/../3rdparty/evernote-api/cpp/src

INCLUDEPATH += $$EVERNOTE_API_PATH

SOURCES +=  \
           $$EVERNOTE_API_PATH/Errors_constants.cpp \
           $$EVERNOTE_API_PATH/Errors_types.cpp \
           $$EVERNOTE_API_PATH/Limits_constants.cpp \
           $$EVERNOTE_API_PATH/Limits_types.cpp \
           $$EVERNOTE_API_PATH/NoteStore.cpp \
           $$EVERNOTE_API_PATH/NoteStore_constants.cpp \
           $$EVERNOTE_API_PATH/NoteStore_types.cpp \
           $$EVERNOTE_API_PATH/Types_constants.cpp \
           $$EVERNOTE_API_PATH/Types_types.cpp \
           $$EVERNOTE_API_PATH/UserStore.cpp \
           $$EVERNOTE_API_PATH/UserStore_constants.cpp \
           $$EVERNOTE_API_PATH/UserStore_types.cpp
