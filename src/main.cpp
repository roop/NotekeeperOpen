/*
  This file is part of Notekeeper Open and is licensed under the MIT License

  Copyright (C) 2011-2015 Roopesh Chander <roop@roopc.net>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject
  to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QFile>
#include <QDir>
#include <QLocale>
#include <QDir>
#include "declarativeruledpaper.h"
#include "clipboard.h"
#include "storage/storagemanager.h"
#include "storage/noteslistmodel.h"
#include "qmldataaccess.h"
#include "connectionmanager.h"
#include "evernoteoauth.h"
#include "evernotesync.h"
#include "qmlnetworkaccessmanagerfactory.h"
#include "qmlimageprovider/qmllocalimagethumbnailprovider.h"
#include "qmlimageprovider/qmlnoteimageprovider.h"
#include "loginstatustracker.h"
#include "notekeeper_config.h"
#include "qplatformdefs.h"

#ifdef HARMATTAN_BOOSTER
#include <MDeclarativeCache>
#endif

static QString versionToString(qint32 version)
{
    if ((version & 0xff) == 0) {
        return QString("%1.%2").arg(version >> 16 & 0xff).arg(version >> 8 & 0xff);
    }
    return QString("%1.%2.%3").arg(version >> 16 & 0xff).arg(version >> 8 & 0xff).arg(version & 0xff);
}

QString adjustPath(const QString &path)
{
// flicked from QmlApplicationViewerPrivate::adjustPath
#ifdef Q_OS_UNIX
#ifdef Q_OS_MAC
    if (!QDir::isAbsolutePath(path))
        return QString::fromLatin1("%1/../Resources/%2")
                .arg(QCoreApplication::applicationDirPath(), path);
#else
    const QString pathInInstallDir =
            QString::fromLatin1("%1/../%2").arg(QCoreApplication::applicationDirPath(), path);
    if (QFileInfo(pathInInstallDir).exists())
        return pathInInstallDir;
#endif
#endif
    return path;
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
#ifdef HARMATTAN_BOOSTER
    QScopedPointer<QApplication> app(MDeclarativeCache::qApplication(argc, argv));
#else
    QScopedPointer<QApplication> app(new QApplication(argc, argv));
#endif
    app->setApplicationName("Notekeeper");
    app->setApplicationVersion(versionToString(APP_VERSION));

#ifdef HARMATTAN_BOOSTER
    QScopedPointer<QDeclarativeView> viewer(MDeclarativeCache::qDeclarativeView());
#else
    QScopedPointer<QDeclarativeView> viewer(new QDeclarativeView);
#endif
    viewer->rootContext()->setContextProperty("appName", app->applicationName());
    viewer->rootContext()->setContextProperty("appVersion", app->applicationVersion());
    viewer->rootContext()->setContextProperty("appArguments", app->arguments());

    Clipboard clipboard;
    viewer->rootContext()->setContextProperty("clipboard", &clipboard);

    qmlRegisterType<DeclarativeRuledPaper>("TextHelper", 1, 0, "RuledPaper");
    qRegisterMetaType<StorageConstants::NotesListType>("StorageConstants::NotesListType");

    StorageManager storageManager;
    ConnectionManager connectionManager(&storageManager);
    viewer->rootContext()->setContextProperty("connectionManager", &connectionManager);

    QmlDataAccess qmlDataAccess(&storageManager, &connectionManager);
    viewer->rootContext()->setContextProperty("qmlDataAccess", &qmlDataAccess);
    viewer->rootContext()->setContextProperty("allNotesListModel", qmlDataAccess.allNotesListModel());
    viewer->rootContext()->setContextProperty("notebookNotesListModel", qmlDataAccess.notebookNotesListModel());
    viewer->rootContext()->setContextProperty("tagNotesListModel", qmlDataAccess.tagNotesListModel());
    viewer->rootContext()->setContextProperty("searchNotesListModel", qmlDataAccess.searchNotesListModel());
    qmlRegisterUncreatableType<StorageConstants>("StorageConstants", 1, 0, "StorageConstants", "Cannot create objects of enum-only class");

    EvernoteSync evernoteSync(&storageManager, &connectionManager);
    viewer->rootContext()->setContextProperty("evernoteSync", &evernoteSync);

    EvernoteOAuth evernoteOAuth(&storageManager, &connectionManager);
    viewer->rootContext()->setContextProperty("evernoteOAuth", &evernoteOAuth);

    // avoid two threads simultaneously trying to resolve offline notes with cached data
    QObject::connect(&evernoteSync, SIGNAL(syncStarted()), &qmlDataAccess, SLOT(unscheduleTryResolveOfflineStatusChanges()));

    LoginStatusTracker loginStatusTracker(&evernoteOAuth, &qmlDataAccess);
    viewer->rootContext()->setContextProperty("loginStatusTracker", &loginStatusTracker);

    // tell QML and sync when we're about to quit
    QObject::connect(app.data(), SIGNAL(aboutToQuit()), &qmlDataAccess, SIGNAL(aboutToQuit()));
    QObject::connect(app.data(), SIGNAL(aboutToQuit()), &evernoteSync, SLOT(cancel()));

    // when connection is lost, stop sync
    QObject::connect(&connectionManager, SIGNAL(disconnected()), &evernoteSync, SLOT(cancel()));

    QmlNetworkAccessManagerFactory nwAccessManagerFactory(&storageManager, &loginStatusTracker);
    viewer->engine()->setNetworkAccessManagerFactory(&nwAccessManagerFactory);
    viewer->engine()->addImageProvider("localimagethumbnail", new QmlLocalImageThumbnailProvider);

    QSslConfiguration sslConf = QSslConfiguration::defaultConfiguration();
    viewer->engine()->addImageProvider("noteimage", new QmlNoteImageProvider(&storageManager, &connectionManager, sslConf));

    QLocale systemLocale = QLocale::system();
    bool is24hourTimeFormat = (!systemLocale.timeFormat(QLocale::ShortFormat).contains("ap", Qt::CaseInsensitive));
    viewer->rootContext()->setContextProperty("is_24hour_time_format_symbian", qVariantFromValue(is24hourTimeFormat));
    viewer->rootContext()->setContextProperty("decimal_point_char", QString(systemLocale.decimalPoint()));
    viewer->rootContext()->setContextProperty("locale_lang_code", systemLocale.name().left(2));

#ifdef QT_SIMULATOR
    viewer->rootContext()->setContextProperty("is_simulator", qVariantFromValue(true));
#else
    viewer->rootContext()->setContextProperty("is_simulator", qVariantFromValue(false));
#endif

    viewer->rootContext()->setContextProperty("is_sandbox", QString(EVERNOTE_API_HOST).startsWith("sandbox"));

#if defined(Q_OS_SYMBIAN) || defined(USE_SYMBIAN_COMPONENTS)
    viewer->rootContext()->setContextProperty("is_symbian", qVariantFromValue(true));
#else
    viewer->rootContext()->setContextProperty("is_symbian", qVariantFromValue(false));
#endif

#if defined(MEEGO_EDITION_HARMATTAN) || defined(USE_MEEGO_COMPONENTS)
    viewer->rootContext()->setContextProperty("is_harmattan", qVariantFromValue(true));
#else
    viewer->rootContext()->setContextProperty("is_harmattan", qVariantFromValue(false));
#endif

    viewer->setAttribute(Qt::WA_AutoOrientation, true);

#ifdef QML_IN_RESOURCE_FILE
    viewer->rootContext()->setContextProperty("qml_root", ":/qml/");
    viewer->setSource(QUrl("qrc:/qml/notekeeper/main.qml"));
    QString qmlResourceBase = ":/qml";
#else
    viewer->rootContext()->setContextProperty("qml_root", "qml/");
    viewer->setSource(QUrl::fromLocalFile(adjustPath("qml/notekeeper/main.qml")));
    QString qmlResourceBase = adjustPath("qml/");
#endif

    // If user is on Meego PR1.0 or PR1.1, 'qml/notekeeper/main.qml' will not load.
    // Instead of showing a a blank screen, we ask the user to do a software update.
    if (!viewer->errors().isEmpty()) {
        bool meegoNotUpToDateError = false;
        foreach (const QDeclarativeError &error, viewer->errors()) {
            if (error.toString().contains("module \"com.nokia.meego\" version 1.1 is not installed")) {
                meegoNotUpToDateError = true;
                break;
            }
        }
        if (meegoNotUpToDateError) {
#ifdef QML_IN_RESOURCE_FILE
            viewer->setSource(QUrl("qrc:/qml/notekeeper/n9_with_old_meego/main.qml"));
#else
            viewer->setSource(QUrl::fromLocalFile(adjustPath("qml/notekeeper/n9_with_old_meego/main.qml")));
#endif
        }
    }

    storageManager.setProperty("qmlResourceBase", qmlResourceBase);

    viewer->showFullScreen();

    return app->exec();
}
