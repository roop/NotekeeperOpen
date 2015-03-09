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

#include "notekeepershareuimethod.h"
#include <QDesktopServices>
#include <QStringBuilder>
#include <QSettings>
#include <QFile>
#include <QDateTime>
#include <QProcess>
#include <ShareUI/ItemContainer>
#include <ShareUI/DataUriItem>
#include <ShareUI/FileItem>
#include <MDataUri>
#include <MTheme>

NotekeeperShareUIMethod::NotekeeperShareUIMethod(QObject *parent) :
    ShareUI::MethodBase(parent), m_isPixmapDirAdded(false)
{
}

NotekeeperShareUIMethod::~NotekeeperShareUIMethod()
{
}

QString NotekeeperShareUIMethod::id()
{
    return (QLatin1String("net.roopc.") % appExecName());
}

QString NotekeeperShareUIMethod::title()
{
    return QString();
}

QString NotekeeperShareUIMethod::subtitle()
{
    return currentlyLoggedInUser();
}

QString NotekeeperShareUIMethod::icon()
{
    if (!m_isPixmapDirAdded) {
        QString shareIconPath = "/opt/" + appExecName() + "/images/" + appExecName() + "-icons/";
        MTheme::addPixmapDirectory(shareIconPath);
        m_isPixmapDirAdded = true;
    }
    return (QLatin1String("icon-") % appExecName() % "-share-plugin");
}

// Meego calls currentItems() to check if this plugin handles these items
// If we do handle it, we emit visible(true), else we say visible(false)
void NotekeeperShareUIMethod::currentItems(const ShareUI::ItemContainer * items)
{
    bool show = showInShareMenu(items);
    emit visible(show);
}

// Meego calls selected() when the user taps the menu item
// We should invoke Notekeeper and exit
void NotekeeperShareUIMethod::selected(const ShareUI::ItemContainer * items)
{
    // Write create note request inputs to a file
    QString createNoteRequestIniPath = notekeeperSessionDirPath() % "/createNoteRequest.ini";
    if (QFile::exists(createNoteRequestIniPath)) {
        return;
    }
    QSettings *createNoteRequestIni = new QSettings(createNoteRequestIniPath, QSettings::IniFormat);

    ShareUI::ItemIterator itemsIter = items->itemIterator();
    bool urlAdded = false;
    bool attachmentsAdded = false;
    QStringList attachments = QStringList();
    while (itemsIter.hasNext()) {
        ShareUI::SharedItem item = itemsIter.next();
        ShareUI::DataUriItem *dataUriItem = ShareUI::DataUriItem::toDataUriItem(item);
        if (dataUriItem) {
            QString uriString = dataUriItem->dataUri().textData();
            if (uriString.startsWith("http://m.ovi.me/?c=") || uriString.startsWith("https://m.ovi.me/?c=")) {
                // it's actually a location being shared from the Maps app; skip it
                continue;
            }
            if (!urlAdded) {
                createNoteRequestIni->setValue("Title", dataUriItem->title());
                createNoteRequestIni->setValue("Url", uriString);
                urlAdded = true;
            }
            continue;
        }
        ShareUI::FileItem *fileItem = ShareUI::FileItem::toFileItem(item);
        if (fileItem) {
            attachments << fileItem->filePath();
            attachments << fileItem->mimeType();
        }
    }
    if (!attachments.isEmpty()) {
        createNoteRequestIni->setValue("Attachments", attachments);
        attachmentsAdded = true;
    }

    if (urlAdded || attachmentsAdded) {
        createNoteRequestIni->setValue("RequestSource", "meego-share-ui-plugin");
        createNoteRequestIni->setValue("RequestFormatVersion", 1);
        createNoteRequestIni->setValue("RequestedAt", QDateTime::currentDateTime().toMSecsSinceEpoch());
        createNoteRequestIni->sync();

        delete createNoteRequestIni; // to make sure there are no race conditions when notekeeper tries to open the ini for reading
        createNoteRequestIni = 0;

        QString appExecFullPath = "/opt/" + appExecName() + "/bin/" + appExecName();
        QString splashArgs = "--splash=/opt/" + appExecName() + "/images/splash/splash.jpg --splash-landscape=/opt/" + appExecName() + "/images/splash/splash-l.jpg";
        QString cmd = "/usr/bin/invoker --type=d -s " + splashArgs + " " + appExecFullPath + " -create-note-request";
        bool started = QProcess::startDetached(cmd);
        if (started) {
            emit done();
        } else {
            emit selectedFailed(QString("Could not activate %1").arg(appExecName()));
        }
        return;
    }

    emit selectedFailed(QString("Nothing to share"));
    delete createNoteRequestIni;
}

QString NotekeeperShareUIMethod::notekeeperSessionDirPath() const
{
    QString homeDir = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
    return (homeDir % "/.local/share/data/Notekeeper/" % appDataDirName() % "/Store/Session");
}

QString NotekeeperShareUIMethod::currentlyLoggedInUser()
{
    QString sessionIniPath = notekeeperSessionDirPath() % "/session.ini";
    if (!QFile::exists(sessionIniPath)) {
        return QString();
    }
    QSettings sessionIni(sessionIniPath, QSettings::IniFormat);
    QString username = sessionIni.value("Username").toString();
    QString userDirName = sessionIni.value("UserDirName").toString();
    bool isReadyForCreateNoteRequests = sessionIni.value("ReadyForCreateNoteRequests").toBool();
    if ((!userDirName.isEmpty()) && isReadyForCreateNoteRequests) {
        return username;
    }
    return QString();
}

bool NotekeeperShareUIMethod::showInShareMenu(const ShareUI::ItemContainer *items)
{
    if (items == 0 || items->count() <= 0) {
        return false;
    }

    QString username = currentlyLoggedInUser();
    if (username.isEmpty()) {
        return false;
    }

    QString createNoteRequestIniPath = notekeeperSessionDirPath() % "/createNoteRequest.ini";
    if (QFile::exists(createNoteRequestIniPath)) { // some other create note request is being processed
        return false;
    }

    ShareUI::ItemIterator itemsIter = items->itemIterator();
    int dataUriItemCount = 0;
    int itemCount = 0;
    while (itemsIter.hasNext()) {
        ShareUI::SharedItem item = itemsIter.next();
        ShareUI::DataUriItem *dataUriItem = ShareUI::DataUriItem::toDataUriItem(item);
        if (dataUriItem) {
            QString uriString = dataUriItem->dataUri().textData();
            if (uriString.startsWith("http://m.ovi.me/?c=") || uriString.startsWith("https://m.ovi.me/?c=")) {
                // it's actually a location being shared from the Maps app; skip it
                continue;
            }
            dataUriItemCount++;
        }
        itemCount++;
    }

    if (itemCount == 0) {
        return false;
    }

    // If we have multiple URLs, we cannot add it to a single note
    if (dataUriItemCount >= 2) {
        return false;
    }

    return true;
}
