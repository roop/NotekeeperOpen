TEMPLATE = app
VERSION = 3.0.0

# Let code know of the app version
DEFINES += APP_VERSION=0x030000

TARGET = notekeeper-open         # for N9
symbian:TARGET = Notekeeper_Open # for Symbian
symbian:TARGET.UID3 = 0xE0605588 # Assuming self-signed
symbian:DEPLOYMENT.display_name = "Notekeeper Open"

QT += network
QT += webkit
CONFIG += qt-components
CONFIG += mobility
MOBILITY += gallery
MOBILITY += multimedia
MOBILITY += systeminfo # Needed to access OS and firmware versions

### Build-time defines ###

# Using Qt Components for Symbian or Meego
symbian {
DEFINES += USE_SYMBIAN_COMPONENTS
}
contains(MEEGO_EDITION, harmattan) {
DEFINES += USE_MEEGO_COMPONENTS
}
simulator {
DEFINES += USE_SYMBIAN_COMPONENTS # Manually specify for Simulator
}

# When building for a release, pick up QML files through qrc files
# (Accessing files through the qrc will be slightly faster.)
contains(CONFIG, release) {
DEFINES += QML_IN_RESOURCE_FILE
}

# Make QML files available to the code
contains(DEFINES, QML_IN_RESOURCE_FILE) {
  # QML files will be included in the exe as resources
  RESOURCES += \
      qml/qml.qrc
  contains(DEFINES, USE_SYMBIAN_COMPONENTS) {
      RESOURCES += qml/symbian/native.qrc
  }
  contains(DEFINES, USE_MEEGO_COMPONENTS) {
      RESOURCES += qml/meego/native.qrc
  }
} else {
  # QML files will be bundled with the installer
  # and will be extracted to the installation
  # location in the phone.
  folder_01.source = qml/notekeeper
  folder_01.target = qml
  DEPLOYMENTFOLDERS += folder_01
  contains(DEFINES, USE_SYMBIAN_COMPONENTS) {
      folder_02.source = qml/symbian/native
      folder_02.target = qml
      DEPLOYMENTFOLDERS += folder_02
  }
  contains(DEFINES, USE_MEEGO_COMPONENTS) {
      folder_03.source = qml/meego/native
      folder_03.target = qml
      DEPLOYMENTFOLDERS += folder_03
  }
}

### Symbian-specific stuff ###

symbian {
  # For network access
  TARGET.CAPABILITY += NetworkServices

  # Min 10 MB (Don't start app unless this much RAM is available)
  # Max 496 MB (My Nokia X7 has only 256 MB RAM, while the Nokia 808 has 512 MB)
  TARGET.EPOCHEAPSIZE =  0xA00000 0x1F000000

  supported_platforms = \
    "[0x2003A678],0,0,0,{\"S60ProductID\"}" \ # Symbian Belle
    "[0x20022E6D],0,0,0,{\"S60ProductID\"}"   # and Symbian^3
  # remove default platforms
  default_deployment.pkg_prerules -= pkg_platform_dependencies
  # add our platforms
  platform_deploy.pkg_prerules += supported_platforms
  DEPLOYMENT += platform_deploy

  # Uncomment if you want to use the Nokia Smart Installer for Symbian,
  # and if you are not creating a self-signed sis file.
  # symbian:DEPLOYMENT.installer_header = 0x2002CCCF

  # Uncomment and add vendor details if applicable
  # vendorinfo = "%{\"App Maintainer Name\"}" \
  #                     ":\"App Maintainer Name\""
  # vendor_deploy.pkg_prerules += vendorinfo
  # DEPLOYMENT += vendor_deploy
}

### Meego-Harmattan-specific stuff ###

contains(MEEGO_EDITION, harmattan) {
  # Speed up launching on MeeGo/Harmattan when using applauncherd daemon
  CONFIG += qdeclarative-boostable

  # Deploy splash screen for Meego
  contains(CONFIG, release) {
    folder_splash.source = splash_screen_harmattan/splash
    folder_splash.target = images
    DEPLOYMENTFOLDERS += folder_splash
  }

  # Deploy share-ui plugin for Meego
  contains(CONFIG, release) {
    # The share-ui plugin should have been built before
    # building this project in release mode
    # (See ./scripts/release_build_for_n9.pl)
    share_ui_plugin.files = share-ui-plugin-harmattan/libnotekeeper-open-share-plugin.so
    share_ui_plugin.path = /usr/lib/share-ui/plugins
    INSTALLS += share_ui_plugin
  }

  # Deploy share-ui icons for Meego
  contains(CONFIG, release) {
    extra_icons.source = extra_icons_harmattan/notekeeper-open-icons
    extra_icons.target = images
    DEPLOYMENTFOLDERS += extra_icons
  }

  OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/notekeeper-open.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog
}

### Source files ###

SOURCES += main.cpp \
    declarativeruledpaper.cpp \
    clipboard.cpp \
    storage/storagemanager.cpp \
    storage/noteslistmodel.cpp \
    storage/crypto/crypto.cpp \
    qmlimageprovider/qmllocalimagethumbnailprovider.cpp \
    qmlimageprovider/qmlnoteimageprovider.cpp \
    connectionmanager.cpp \
    qmldataaccess.cpp \
    qmlnetworkaccessmanagerfactory.cpp \
    storage/diskcache/shareddiskcache.cpp \
    qmlnetworkdiskcache.cpp \
    searchlocalnotesthread.cpp \
    logger.cpp \
    qmlnetworkcookiejar.cpp \
    qmlnetworkaccessmanager.cpp \
    loginstatustracker.cpp

HEADERS += \
    declarativeruledpaper.h \
    clipboard.h \
    storage/storagemanager.h \
    storage/noteslistmodel.h \
    storage/crypto/crypto.h \
    qmlimageprovider/qmllocalimagethumbnailprovider.h \
    qmlimageprovider/qmlnoteimageprovider.h \
    connectionmanager.h \
    qmldataaccess.h \
    qmlnetworkaccessmanagerfactory.h \
    storage/diskcache/shareddiskcache.h \
    qmlnetworkdiskcache.h \
    searchlocalnotesthread.h \
    logger.h \
    qmlnetworkcookiejar.h \
    qmlnetworkaccessmanager.h \
    loginstatustracker.h

INCLUDEPATH += $$PWD $$PWD/storage

include(cloud/evernote/evernotesync/evernotesync.pri)
include(3rdparty/qblowfish/qblowfish.pri)
include(3rdparty/qexifreader/qexifreader.pri)

# Please do not modify the following two lines. Required for deployment.
include(qmlapplicationviewer/qmlapplicationviewer.pri)
qtcAddDeployment()
