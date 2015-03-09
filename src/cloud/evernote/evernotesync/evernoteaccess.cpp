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

#include "evernoteaccess.h"
#include "storagemanager.h"
#include "connectionmanager.h"
#include "evernotesync.h"
#include "edamutils.h"
#include "notekeeper_config.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QCryptographicHash>
#include <QStringBuilder>
#include <QVariantMap>
#include <QFile>
#include <QTimer>
#include <QtDebug>
#include <QDateTime>
#include <QApplication>

// c++ stl
#ifdef Q_OS_SYMBIAN
#include <stdapis/stlportv5/string>
#include <stdapis/stlportv5/list>
#include <arpa/inet.h>
#endif

// thrift
#include <Thrift.h>
#include <protocol/TBinaryProtocol.h>

// evernote
#include "UserStore.h"
#include "UserStore_constants.h"
#include "NoteStore.h"
#include "NoteStore_constants.h"

// QtThrift
#include "qthrifthttpclient.h"

using namespace apache;
using namespace evernote;

EvernoteAccess::EvernoteAccess(StorageManager *storageManager, ConnectionManager *connectionManager, QObject *parent)
    : QObject(parent)
    , m_storageManager(storageManager)
    , m_connectionManager(connectionManager)
    , m_userStoreHttpClient(0)
    , m_noteStoreHttpClient(0)
    , m_cancelled(false)
    , m_isFetchingWebResource(false)
    , m_isSslErrorFound(false)
{
}

void EvernoteAccess::setSslConfiguration(const QSslConfiguration &sslConf)
{
    m_sslConfig = sslConf;
}

bool EvernoteAccess::cancel()
{
    m_storageManager->log("EvernoteAccess::cancel()");
    if (m_userStoreHttpClient) {
        m_userStoreHttpClient->setCancelled(true);
    }
    if (m_noteStoreHttpClient) {
        m_noteStoreHttpClient->setCancelled(true);
    }
    m_cancelled = true;
    if (m_userStoreHttpClient || m_noteStoreHttpClient || m_isFetchingWebResource) {
        emit cancelled(); // caught by the current QThriftHttpClient object
        return true;
    }
    return false;
}

bool EvernoteAccess::fetchNote(const QString &noteId, QVariantMap *returnNoteData)
{
    QUrl noteUrl(m_storageManager->retrieveEvernoteAuthData("noteStoreUrl"));
    boost::shared_ptr<QThriftHttpClient> httpClient(new QThriftHttpClient(noteUrl, this));
    httpClient->setNetworkConfiguration(m_connectionManager->selectedConfiguration());
    m_noteStoreHttpClient = httpClient.get();
    boost::shared_ptr<thrift::protocol::TProtocol> protocol(new thrift::protocol::TBinaryProtocol(httpClient));
    httpClient->open();
    edam::NoteStoreClient noteStore(protocol);

    bool ok = fetchNote(&noteStore, noteId, httpClient->networkAccessManager(), returnNoteData);
    if (!ok) {
        m_noteStoreHttpClient = 0;
        return false;
    }

    emit finished(true, QString());
    m_noteStoreHttpClient = 0;
    return true;
}

bool EvernoteAccess::fetchNote(edam::NoteStoreClient *noteStore, const QString &noteId, QNetworkAccessManager *nwAccessManager, QVariantMap *returnNoteData)
{
    if (m_storageManager->isNoteContentUpToDate(noteId)) {
        return true;
    }

    const QString guid = m_storageManager->guidForNoteId(noteId);
    Q_ASSERT(!guid.isEmpty());
    if (guid.isEmpty()) {
        return false;
    }

    QString authToken = m_storageManager->retrieveEncryptedEvernoteAuthData("authToken");

    edam::Note note;
    if (!edamGetNote(noteStore, &note, authToken, guid, true, false, false, false)) {
        return false;
    }

    // store the note
    QString title = utf8StringFromStdString(note.title);
    QByteArray content = byteArrayFromStdString(note.content);
    QByteArray contentHash = byteArrayFromStdString(note.contentHash);
    Q_ASSERT(contentHash.size() == 16 /* MD5 hash length */);

    QByteArray computedHash = QCryptographicHash::hash(content, QCryptographicHash::Md5);
    Q_ASSERT(contentHash == computedHash);
    Q_UNUSED(computedHash);

    if (returnNoteData) {
        returnNoteData->insert("guid", guid);
        returnNoteData->insert("Title", title);
        returnNoteData->insert("Content", content);
        returnNoteData->insert("ContentHash", contentHash.toHex());
        returnNoteData->insert("USN", qint32(note.updateSequenceNum));
        returnNoteData->insert("CreatedTime", qint64(note.created));
        returnNoteData->insert("UpdatedTime", qint64(note.updated));
    }
    QVariantMap noteAttributes = getEdamNoteAttributes(note);

    // store metadata about the resources
    QVariantList attachmentsData;
    bool hasImages = false;
    for (std::vector<edam::Resource>::const_iterator resourcesIter = note.resources.begin();
         resourcesIter != note.resources.end();
         resourcesIter++) {
        const edam::Resource &resource = (*resourcesIter);
        if (latin1StringFromStdString(resource.mime) != "") {
            QString resourceGuid = latin1StringFromStdString(resource.guid);
            QByteArray resourceHash = byteArrayFromStdString(resource.data.bodyHash);
            Q_ASSERT(resourceHash.size() == 16 /* MD5 hash length */);
            QVariantMap attachmentDataMap;
            attachmentDataMap["guid"] = resourceGuid;
            attachmentDataMap["Hash"] = resourceHash.toHex();
            QString mimeType = latin1StringFromStdString(resource.mime);
            attachmentDataMap["MimeType"] = mimeType;
            if (mimeType.startsWith("image/")) {
                hasImages = true;
            }
            attachmentDataMap["Dimensions"] = QSize(resource.width, resource.height);
            attachmentDataMap["Duration"] = resource.duration;
            attachmentDataMap["Size"] = qint32(resource.data.size);
            QString fileName("");
            if (resource.__isset.attributes && resource.attributes.__isset.fileName) {
                fileName = utf8StringFromStdString(resource.attributes.fileName);
            }
            attachmentDataMap["FileName"] = fileName;
            attachmentsData << attachmentDataMap;
        }
    }
    int removedImagesCount;
    m_storageManager->setAttachmentsDataFromServer(noteId, attachmentsData, false /*isAfterPush*/, &removedImagesCount);
    QVariantList attachmentsWithResolvedPaths = m_storageManager->attachmentsData(noteId);
    if (returnNoteData) {
        returnNoteData->insert("AttachmentsData",  attachmentsWithResolvedPaths);
    }

    m_storageManager->log(QString("Fetched note [%1] Title: [%2]").arg(guid).arg(title));
    m_storageManager->setFetchedNoteContent(guid, title, content, contentHash.toHex(), qint32(note.updateSequenceNum), qint64(note.updated), noteAttributes, attachmentsWithResolvedPaths);
    updateNoteThumbnail(noteId, guid, hasImages, (removedImagesCount > 0) /* imagesRemoved */, nwAccessManager);
    return true;
}

bool EvernoteAccess::updateNoteThumbnail(const QString &noteId, const QString &noteGuid, bool hasImages, bool imagesRemoved, QNetworkAccessManager *nwAccessManager)
{
    if (hasImages) {
        bool noteThumbnailExists = m_storageManager->noteThumbnailExists(noteId);
        if (!noteThumbnailExists || imagesRemoved) {
            fetchNoteThumbnail(noteId, noteGuid, nwAccessManager);
        }
        // Creating a new QNAM inside fetchNoteThumbnail doesn't work
        // Possibly, we should have only one qnam per thread
    } else {
        m_storageManager->setNoteThumbnail(noteId, QImage());
    }
}

bool EvernoteAccess::fetchNoteThumbnail(const QString &noteId, const QString &noteGuid, QNetworkAccessManager *nwAccessManager)
{
    if (noteId.isEmpty() || noteGuid.isEmpty()) {
        return false;
    }
    m_cancelled = false;

    QString webApiUrlPrefix = m_storageManager->retrieveEvernoteAuthData("webApiUrlPrefix");
    QUrl noteThumbUrl(QString("%1thm/note/%2.jpg?size=75").arg(webApiUrlPrefix).arg(noteGuid));
    QTemporaryFile tempFile("notekeeper_tmp");
    bool ok = fetchEvernoteWebResource(noteThumbUrl, nwAccessManager, &tempFile);
    if (!ok || tempFile.fileName().isEmpty()) {
        return false;
    }

    QImage image(tempFile.fileName(), "JPG");
    if (m_cancelled || image.isNull()) {
        return false;
    }

    return m_storageManager->setNoteThumbnail(noteId, image);
}

bool EvernoteAccess::fetchOfflineNoteAttachment(const QString &noteId, const QString &attachmentGuid, const QByteArray &attachmentHash, QNetworkAccessManager *nwAccessManager)
{
    if (noteId.isEmpty() || attachmentGuid.isEmpty() || attachmentHash.isEmpty()) {
        return false;
    }
    QString webApiUrlPrefix = m_storageManager->retrieveEvernoteAuthData("webApiUrlPrefix");

    if (webApiUrlPrefix.isEmpty()) {
        return false;
    }

    m_cancelled = false;

    QUrl noteResourceUrl(QString("%1res/%2").arg(webApiUrlPrefix).arg(attachmentGuid));
    QTemporaryFile tempFile("notekeeper_tmp");
    bool ok = fetchEvernoteWebResource(noteResourceUrl, nwAccessManager, &tempFile);
    if (!ok) {
        return false;
    }

    return m_storageManager->setOfflineNoteAttachmentContent(noteId, attachmentHash, &tempFile);
}

bool EvernoteAccess::fetchEvernoteWebResource(const QUrl &url, QNetworkAccessManager *nwAccessManager, QTemporaryFile* webResourceFile, bool emitProgress)
{
    QString authToken = m_storageManager->retrieveEncryptedEvernoteAuthData("authToken");
    QUrl genericUrl(QString("https://%1").arg(EVERNOTE_API_HOST));
    QNetworkCookie cookie("auth", authToken.toLatin1());
    cookie.setDomain(genericUrl.host());
    if (genericUrl.scheme() == "https") {
        cookie.setSecure(true);
    }
    QNetworkCookieJar *cookieJar = new QNetworkCookieJar;
    cookieJar->setCookiesFromUrl(QList<QNetworkCookie>() << cookie, genericUrl);
    nwAccessManager->setCookieJar(cookieJar);

    if (!webResourceFile->isOpen()) {
        webResourceFile->open();
    }

    QNetworkReply *reply = nwAccessManager->get(QNetworkRequest(url));
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(60 * 1000); // 1 min timeout
    timer.setSingleShot(true);
    connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(this, SIGNAL(cancelled()), &loop, SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    m_cancelled = false;
    m_isFetchingWebResource = true;
    qint64 bytesDownloaded = 0;
    do {
        timer.start();
        loop.exec(); // blocking wait
        if (m_cancelled) { // cancelled
            return false;
        }
        if (!timer.isActive()) { // timed out
            return false;
        }
        while (reply->bytesAvailable() > 0) {
            QByteArray data = reply->read(1 << 18); // max 256 kb
            bytesDownloaded += webResourceFile->write(data);
        }
        if (emitProgress) {
            emit webResourceFetchProgressed(bytesDownloaded);
        }
    } while(!reply->isFinished());

    if (reply->bytesAvailable() > 0) {
        webResourceFile->write(reply->readAll());
    }
    webResourceFile->close();
    reply->deleteLater();
    m_isFetchingWebResource = false;

    return true;
}

bool EvernoteAccess::fetchNoteAndAttachmentsForOfflineAccess(edam::NoteStoreClient *noteStore, const QString &noteId, QNetworkAccessManager *nwAccessManager)
{
    bool noteNeedsToBeOffline = m_storageManager->noteNeedsToBeAvailableOffline(noteId);
    if (noteNeedsToBeOffline) {
        if (!m_storageManager->noteContentAvailableOffline(noteId)) {
            fetchNote(noteStore, noteId, nwAccessManager);
        }
        foreach (const QVariant &map, m_storageManager->attachmentsData(noteId)) {
            QVariantMap attachment = map.toMap();
            QString filePath = attachment.value("FilePath").toString();
            if (filePath.isEmpty() || !QFile::exists(filePath)) {
                QString attachmentGuid = attachment.value("guid").toString();
                QByteArray attachmentHash = attachment.value("Hash").toByteArray();
                fetchOfflineNoteAttachment(noteId, attachmentGuid, attachmentHash, nwAccessManager);
            }
        }
        return true;
    }
    return false;
}

static QString storeNoteGist(const edam::Note &note, StorageManager *storageManager, QString *noteGuid = 0, bool *isUsnChanged = 0)
{
    QString title = utf8StringFromStdString(note.title);
    QString guid = latin1StringFromStdString(note.guid);
    if (noteGuid) {
        (*noteGuid) = guid;
    }
    QByteArray contentHash = byteArrayFromStdString(note.contentHash);
    Q_ASSERT(contentHash.size() == 16 /* MD5 hash length */);
    QVariantMap noteAttributes = getEdamNoteAttributes(note);
    bool _isUsnChanged = false;
    QString noteId = storageManager->setSyncedNoteGist(guid, title, qint32(note.updateSequenceNum), contentHash.toHex(),
                                                       qint64(note.created), qint64(note.updated), noteAttributes, &_isUsnChanged);
    if (isUsnChanged) {
        (*isUsnChanged) = _isUsnChanged;
    }
    return noteId;
}

static void storeNoteNotebooksTagsTrashedness(const QString &noteId, const edam::Note &note, StorageManager *storageManager)
{
    // Set notebook for note
    QString notebookGuid = latin1StringFromStdString(note.notebookGuid);
    if (!notebookGuid.isEmpty()) {
        QString notebookId = storageManager->notebookIdForGuid(notebookGuid);
        if (notebookId.isEmpty()) {
            // we don't know about this notebook yet. we'll just write it down and move on.
            storageManager->addToEvernoteSyncIdsList("NoteIdsForUncreatedNotebook/" % notebookGuid, noteId);
        } else {
            storageManager->setNotebookForNote(noteId, notebookId);
        }
    }

    // Add tags to note
    QStringList tagIdsList;
    for (std::vector<edam::Guid>::const_iterator tagGuidIter = note.tagGuids.begin();
         tagGuidIter != note.tagGuids.end();
         tagGuidIter++) {
        const edam::Guid &tagGuidStdStr = (*tagGuidIter);
        QString tagGuid = latin1StringFromStdString(tagGuidStdStr);
        if (!tagGuid.isEmpty()) {
            QString tagId = storageManager->tagIdForGuid(tagGuid);
            if (tagId.isEmpty()) {
                // we don't know about this notebook yet. we'll just write it down and move on.
                storageManager->addToEvernoteSyncIdsList("NoteIdsForUncreatedTag/" % tagGuid, noteId);
            } else {
                tagIdsList.append(tagId);
            }
        }
    }
    storageManager->setTagsOnNote(noteId, tagIdsList);

    // Trashed notes
    if (!note.active) {
        storageManager->moveNoteToTrash(noteId);
    } else {
        storageManager->restoreNoteFromTrash(noteId);
    }
}

static void storeNoteAttachments(const QString &noteId, const edam::Note &note, StorageManager *storageManager, bool isAfterPush, bool *hasImages = 0, int *removedImagesCount = 0)
{
    if (hasImages) {
        (*hasImages) = false;
    }
    QVariantList attachmentsData;
    for (std::vector<edam::Resource>::const_iterator resourcesIter = note.resources.begin();
         resourcesIter != note.resources.end();
         resourcesIter++) {
        const edam::Resource &resource = (*resourcesIter);
        QString resourceGuid = latin1StringFromStdString(resource.guid);
        QByteArray resourceHash = byteArrayFromStdString(resource.data.bodyHash);
        Q_ASSERT(resourceHash.size() == 16 /* MD5 hash length */);
        QVariantMap attachmentDataMap;
        attachmentDataMap["guid"] = resourceGuid;
        attachmentDataMap["Hash"] = resourceHash.toHex();
        QString mimeType = latin1StringFromStdString(resource.mime);
        attachmentDataMap["MimeType"] = mimeType;
        attachmentDataMap["Dimensions"] = QSize(resource.width, resource.height);
        attachmentDataMap["Duration"] = resource.duration;
        attachmentDataMap["Size"] = qint32(resource.data.size);
        QString fileName("");
        if (resource.__isset.attributes && resource.attributes.__isset.fileName) {
            fileName = utf8StringFromStdString(resource.attributes.fileName);
        }
        attachmentDataMap["FileName"] = fileName;
        attachmentsData << attachmentDataMap;
        if (mimeType.startsWith("image/")) {
            if (hasImages) {
                (*hasImages) = true;
            }
        }
    }

    storageManager->setAttachmentsDataFromServer(noteId, attachmentsData, isAfterPush, removedImagesCount);
}

static QString syncCompletedMessage(int newNotesCount, int pushedObjectsCount)
{
    if (newNotesCount == 0 && pushedObjectsCount == 0) {
        return QLatin1String("No updates");
    }
    QStringList points;
    if (newNotesCount > 0) {
        points << QString("Updates received");
    }
    if (pushedObjectsCount > 0) {
        points << QString("Changes sent");
    }
    return (points.join(". ") + ".");
}

// definitions used in calculating the sync progress
#define FETCHING_OFFLINE_NOTES 0
#define RESOLVING_PENDING_CONFLICTS 1
#define FETCHING_OFFLINE_NOTEBOOKS 2
#define FETCHING_UPDATES 3
#define PREFETCHING_NOTES 4
#define PUSHING_UPDATES 5

static int syncProgress(int currentlyDoingWhat, int value, int total)
{
    int perStageProgress = (100 / (PUSHING_UPDATES + 1));
    int finishedStagesProgress = currentlyDoingWhat * perStageProgress;
    int thisStageProgress = (total <= 0? perStageProgress : (value * perStageProgress / total));
    return (finishedStagesProgress + thisStageProgress);
}

void EvernoteAccess::synchronize()
{
    m_storageManager->log("EvernoteAccess::synchronize()");
    m_cancelled = false;
    m_isSslErrorFound = false;

    QString authToken = m_storageManager->retrieveEncryptedEvernoteAuthData("authToken");
    fetchUserInfo(authToken); // update account information

    qint32 usn = m_storageManager->retrieveEvernoteSyncData("USN").toInt();
    bool fullSyncDone = m_storageManager->retrieveEvernoteSyncData("FullSyncDone").toBool();

    int maxEntries = 50; // Ref: http://forum.evernote.com/phpbb/viewtopic.php?f=43&t=26978
    bool fullSync = (!fullSyncDone || usn == 0);
    bool isFirstChunkDone = (!fullSync);

    m_storageManager->log(QString("synchronize() fullSyncDone=%1 usn=%2").arg(fullSyncDone).arg(usn));

    QUrl noteUrl(m_storageManager->retrieveEvernoteAuthData("noteStoreUrl"));
    boost::shared_ptr<QThriftHttpClient> httpClient(new QThriftHttpClient(noteUrl, this));
    httpClient->setNetworkConfiguration(m_connectionManager->selectedConfiguration());
    m_noteStoreHttpClient = httpClient.get();
    boost::shared_ptr<thrift::protocol::TProtocol> noteProtocol(new thrift::protocol::TBinaryProtocol(httpClient));
    httpClient->open();
    edam::NoteStoreClient noteStore(noteProtocol);

    // getSyncState
    if (!fullSync) {
        emit syncStatusMessage("Syncing: Looking for updates in the server");
    }
    edam::SyncState syncState;
    if (!edamGetSyncState(&noteStore, &syncState, authToken)) {
        m_noteStoreHttpClient = 0;
        return;
    }
    qint32 maxUsn = syncState.updateCount;

    m_storageManager->log(QString("synchronize() got sync state maxUsn=%1").arg(maxUsn));
    m_storageManager->saveEvernoteSyncData("Account/UploadedBytesInThisCycle", static_cast<qint64>(syncState.uploaded));

    // fetch pending offline notes

    {
        m_storageManager->tryResolveOfflineStatusChanges();
        QStringList offlineStatusChangeUnresolvedNotes = m_storageManager->offlineStatusChangeUnresolvedNotes();
        for (int i = 0; i < offlineStatusChangeUnresolvedNotes.count(); i++) {
            const QString &noteId = offlineStatusChangeUnresolvedNotes[i];
            fetchNoteAndAttachmentsForOfflineAccess(&noteStore, noteId, httpClient->networkAccessManager());
            m_storageManager->setOfflineStatusChangeUnresolvedNote(noteId, false);
            emit syncProgressChanged(syncProgress(FETCHING_OFFLINE_NOTES, (i + 1), offlineStatusChangeUnresolvedNotes.count()));
        }
    }

    // resolve pending conflicts

    QStringList notesPendingConflictResolution = m_storageManager->retrieveEvernoteSyncIdsList("NoteIdsWithDeferredConflictResolution");
    for (int i = 0; i < notesPendingConflictResolution.count(); i++) {
        const QString &noteId = notesPendingConflictResolution[i];
        fetchNote(&noteStore, noteId, httpClient->networkAccessManager());
        emit syncProgressChanged(syncProgress(RESOLVING_PENDING_CONFLICTS, (i + 1), notesPendingConflictResolution.count()));
    }
    m_storageManager->clearEvernoteSyncIdsList("NoteIdsWithDeferredConflictResolution");

    // fetch pending offline notebooks

    {
        QStringList offlineStatusUnresolvedNotebooks = m_storageManager->offlineStatusChangeUnresolvedNotebooks();
        bool isStatusMsgSetToDownloadingOfflineNotebooks = false;
        for (int i = 0; i < offlineStatusUnresolvedNotebooks.count(); i++) {
            const QString &notebookId = offlineStatusUnresolvedNotebooks[i];
            const QStringList noteIds = m_storageManager->listNoteIds(StorageConstants::NotesInNotebook, notebookId);
            foreach (const QString &noteId, noteIds) {
                bool requiredOffline, madeOffline;
                m_storageManager->tryResolveOfflineStatusChange(noteId, &requiredOffline, &madeOffline);
                if (requiredOffline && !madeOffline) {
                    if (!isStatusMsgSetToDownloadingOfflineNotebooks) {
                        emit syncStatusMessage("Syncing: Downloading offline notebooks");
                        isStatusMsgSetToDownloadingOfflineNotebooks = true;
                    }
                    fetchNoteAndAttachmentsForOfflineAccess(&noteStore, noteId, httpClient->networkAccessManager());
                }
            }
            m_storageManager->setOfflineStatusChangeUnresolvedNotebook(notebookId, false);
            emit syncProgressChanged(syncProgress(FETCHING_OFFLINE_NOTEBOOKS, (i + 1), offlineStatusUnresolvedNotebooks.count()));
        }
    }

    // pull changes
    int newNotesCount = 0;

    if (usn > 0) {
        if (usn < maxUsn) { // if we have updates to pull
            // check if the server recommends a full sync or incremental sync
            qint64 lastSyncServerTime = m_storageManager->retrieveEvernoteSyncData("LastSyncServerTime").toLongLong();
            if (qint64(syncState.fullSyncBefore) > lastSyncServerTime) {
                emit finished(false, QLatin1String("Server requires non-incremental sync"));
                m_noteStoreHttpClient = 0;
                return;
            }
            emit syncStatusMessage("Syncing: Receiving updates");
        }
    } else {
        emit syncStatusMessage("Syncing: Receiving notes data");
    }

    if (fullSync) {
        maxEntries = 10; // so that we make the progressbar move quicker
    }

    // getSyncChunk
    while (usn < maxUsn) {
        edam::SyncChunk syncChunk;
        if (!edamGetSyncChunk(&noteStore, &syncChunk, authToken, usn, maxEntries, fullSync)) {
            m_noteStoreHttpClient = 0;
            return;
        }

        usn = syncChunk.chunkHighUSN;
        maxUsn = syncChunk.updateCount;

        m_storageManager->log(QString("synchronize() got sync chunk usn=%1 maxUsn=%2").arg(usn).arg(maxUsn));

        // Process notebooks
        for (std::vector<edam::Notebook>::const_iterator notebooksIter = syncChunk.notebooks.begin();
             notebooksIter != syncChunk.notebooks.end();
             notebooksIter++) {
            const edam::Notebook &notebook = (*notebooksIter);
            QString notebookName = utf8StringFromStdString(notebook.name);
            QString notebookGuid = latin1StringFromStdString(notebook.guid);
            QString notebookId = m_storageManager->setSyncedNotebookData(notebookGuid, notebookName);
            if (notebook.defaultNotebook) {
                m_storageManager->setDefaultNotebookId(notebookId);
            }
            QStringList alreadySeenNotesInThisNotebook = m_storageManager->retrieveEvernoteSyncIdsList("NoteIdsForUncreatedNotebook/" % notebookGuid);
            foreach (const QString &noteId, alreadySeenNotesInThisNotebook) {
                m_storageManager->setNotebookForNote(noteId, notebookId);
            }
            m_storageManager->clearEvernoteSyncIdsList("NoteIdsForUncreatedNotebook/" % notebookGuid);
        }

        // Process tags
        for (std::vector<edam::Tag>::const_iterator tagsIter = syncChunk.tags.begin();
             tagsIter != syncChunk.tags.end();
             tagsIter++) {
            const edam::Tag &tag = (*tagsIter);
            QString tagName = utf8StringFromStdString(tag.name);
            QString tagGuid = latin1StringFromStdString(tag.guid);
            QString tagId = m_storageManager->setSyncedTagData(tagGuid, tagName);

            QStringList alreadySeenNotesWithThisTag = m_storageManager->retrieveEvernoteSyncIdsList("NoteIdsForUncreatedTag/" % tagGuid);
            foreach (const QString &noteId, alreadySeenNotesWithThisTag) {
                m_storageManager->addTagToNote(noteId, tagId);
            }
            m_storageManager->clearEvernoteSyncIdsList("NoteIdsForUncreatedTag/" % tagGuid);
        }

        // Process notes
        for (std::vector<edam::Note>::const_iterator notesIter = syncChunk.notes.begin();
             notesIter != syncChunk.notes.end();
             notesIter++) {

            const edam::Note &note = (*notesIter);

            bool hasImages = false;
            int removedImagesCount;

            QString guid;
            bool isUsnChanged = false;

            QString noteId = storeNoteGist(note, m_storageManager, &guid, &isUsnChanged);
            storeNoteAttachments(noteId, note, m_storageManager, false /*isAfterPush*/, &hasImages, &removedImagesCount);
            if (!m_storageManager->noteHasUnpushedChanges(noteId)) { // don't overwrite local changes
                storeNoteNotebooksTagsTrashedness(noteId, note, m_storageManager);
            }

            // Fetch note if applicable
            bool fetched = false;
            bool noteNeedsToBeOffline = m_storageManager->noteNeedsToBeAvailableOffline(noteId);
            if (noteNeedsToBeOffline && (!m_storageManager->noteContentAvailableOffline(noteId))) {
                fetchNoteAndAttachmentsForOfflineAccess(&noteStore, noteId, httpClient->networkAccessManager());
                fetched = true;
            } else if (m_storageManager->noteHasUnpushedContentChanges(noteId)) { // if note is locally modified, gotta resolve the conflict locally
                fetchNote(&noteStore, noteId, httpClient->networkAccessManager());
                fetched = true;
            }

            // Update note thumbnail if applicable
            if (!fetched) {
                updateNoteThumbnail(noteId, guid, hasImages, (removedImagesCount > 0) /* imagesRemoved */, httpClient->networkAccessManager());
            }

            if (isUsnChanged) {
                newNotesCount++;
            }
        } // end of foreach note

        // Expunged stuff
        for (std::vector<edam::Guid>::const_iterator expungedNotesGuidIter = syncChunk.expungedNotes.begin();
             expungedNotesGuidIter != syncChunk.expungedNotes.end();
             expungedNotesGuidIter++) {
            const edam::Guid &expungedNoteGuidStdStr = (*expungedNotesGuidIter);
            QString expungedNoteGuid = latin1StringFromStdString(expungedNoteGuidStdStr);
            QString expungedNoteId = m_storageManager->noteIdForGuid(expungedNoteGuid);
            if (!expungedNoteId.isEmpty()) {
                m_storageManager->expungeNote(expungedNoteId);
            }
        }
        for (std::vector<edam::Guid>::const_iterator expungedNotebooksGuidIter = syncChunk.expungedNotebooks.begin();
             expungedNotebooksGuidIter != syncChunk.expungedNotebooks.end();
             expungedNotebooksGuidIter++) {
            const edam::Guid &expungedNotebookGuidStdStr = (*expungedNotebooksGuidIter);
            QString expungedNotebookGuid = latin1StringFromStdString(expungedNotebookGuidStdStr);
            QString expungedNotebookId = m_storageManager->notebookIdForGuid(expungedNotebookGuid);
            if (!expungedNotebookId.isEmpty()) {
                m_storageManager->expungeNotebook(expungedNotebookId);
            }
        }
        for (std::vector<edam::Guid>::const_iterator expungedTagsGuidIter = syncChunk.expungedTags.begin();
             expungedTagsGuidIter != syncChunk.expungedTags.end();
             expungedTagsGuidIter++) {
            const edam::Guid &expungedTagGuidStdStr = (*expungedTagsGuidIter);
            QString expungedTagGuid = latin1StringFromStdString(expungedTagGuidStdStr);
            QString expungedTagId = m_storageManager->tagIdForGuid(expungedTagGuid);
            if (!expungedTagId.isEmpty()) {
                m_storageManager->expungeTag(expungedTagId);
            }
        }

        // checkpoint how far we've synced
        m_storageManager->saveEvernoteSyncData("USN", usn);
        m_storageManager->saveEvernoteSyncData("LastSyncServerTime", qint64(syncChunk.currentTime));

        if (fullSync) {
            m_storageManager->saveEvernoteSyncData("FullSyncDone", true);
            emit fullSyncDoneChanged();
            // fullSync = false; // Commented because there's no need to get expunged stuff for just this sync
            if (!isFirstChunkDone) {
                isFirstChunkDone = true;
                emit firstChunkDone();
            }
            // Write down current time as last-synced-time
            qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
            m_storageManager->saveEvernoteSyncData("LastSyncedTime", now);
        }
        emit syncProgressChanged(syncProgress(FETCHING_UPDATES, usn, maxUsn));
    } // end of while (usn < maxUsn)

    // Pre-fetch the latest notes
    QStringList allNoteIds = m_storageManager->listNoteIds(StorageConstants::AllNotes);
    int preFetchingNotesCount = qMin(10, allNoteIds.count());
    for (int i = 0; i < preFetchingNotesCount; i++) {
        fetchNote(&noteStore, allNoteIds[i], httpClient->networkAccessManager());
        emit syncProgressChanged(syncProgress(PREFETCHING_NOTES, (i + 1), preFetchingNotesCount));
    }

    // push changes

    const QStringList notesToPush = m_storageManager->notesWithUnpushedChanges();
    const QStringList notebooksToPush = m_storageManager->notebooksWithUnpushedChanges();
    const QStringList tagsToPush = m_storageManager->tagsWithUnpushedChanges();
    if ((!notesToPush.isEmpty()) || (!notebooksToPush.isEmpty()) || (!tagsToPush.isEmpty())) {
        emit syncStatusMessage("Syncing: Sending changes");
    }

    int pushedNotebooksCount = 0;
    foreach (const QString &notebookId, notebooksToPush) {
        const QString notebookName = m_storageManager->notebookName(notebookId);
        QString notebookGuid = m_storageManager->guidForNotebookId(notebookId);
        if (!notebookId.isEmpty() && !notebookName.isEmpty()) {
            if (notebookGuid.isEmpty()) {
                // Do nothing here; We'll create the notebook in the server while pushing contained notes.
            } else {
                // Update notebook
                edam::Notebook notebookData;
                setEdamNotebookGuid(&notebookData, notebookGuid);
                setEdamNotebookName(&notebookData, notebookName);
                qint32 resultUsn;
                if (!edamUpdateNotebook(&noteStore, &resultUsn, authToken, notebookData)) {
                    m_noteStoreHttpClient = 0;
                    return;
                }
                m_storageManager->setNotebookHasUnpushedChanges(notebookId, false);
                if (resultUsn == (usn + 1)) { // nothing has changed in the server between our last pull and this push
                    usn = usn + 1;
                    m_storageManager->saveEvernoteSyncData("USN", usn);
                }
                pushedNotebooksCount++;
            }
        }
    }

    int pushedTagsCount = 0;
    foreach (const QString &tagId, tagsToPush) {
        const QString tagName = m_storageManager->tagName(tagId);
        QString tagGuid = m_storageManager->guidForTagId(tagId);
        if (!tagId.isEmpty() && !tagName.isEmpty()) {
            if (tagGuid.isEmpty()) {
                // Do nothing here; We'll create the tag in the server while pushing notes with that tag.
            } else {
                // Update tag
                edam::Tag tagData;
                setEdamTagGuid(&tagData, tagGuid);
                setEdamTagName(&tagData, tagName);
                qint32 resultUsn;
                if (!edamUpdateTag(&noteStore, &resultUsn, authToken, tagData)) {
                    m_noteStoreHttpClient = 0;
                    return;
                }
                m_storageManager->setTagHasUnpushedChanges(tagId, false);
                if (resultUsn == (usn + 1)) { // nothing has changed in the server between our last pull and this push
                    usn = usn + 1;
                    m_storageManager->saveEvernoteSyncData("USN", usn);
                }
                pushedTagsCount++;
            }
        }
    }

    int pushedNotesCount = 0;
    int failedPushNotesCount = 0;
    int uploadLimitReachedNotesCount = 0;
    foreach (const QString& noteId, notesToPush) {
        const QVariantMap noteDataMap = m_storageManager->noteData(noteId);
        QString noteGuid = noteDataMap.value("guid").toString();

        m_storageManager->log(QString("synchronize() Trying to push note guid=%1").arg(noteGuid));

        // notebook for the note
        QString notebookId = noteDataMap.value("NotebookId").toString();
        if (notebookId.isEmpty()) {
            QString defaultNotebookId = m_storageManager->defaultNotebookId();
            if (defaultNotebookId.isEmpty()) { // if even default is not set, make a random guess
                QStringList candidateNotebookIds = m_storageManager->listNormalNotebookIds();
                if (!candidateNotebookIds.isEmpty()) {
                    notebookId = candidateNotebookIds.last();
                }
            }
            m_storageManager->setNotebookForNote(noteId, notebookId);
        }
        const QString notebookName = m_storageManager->notebookName(notebookId);
        QString notebookGuid = m_storageManager->guidForNotebookId(notebookId);
        if (!notebookId.isEmpty() && !notebookName.isEmpty()) {
            if (notebookGuid.isEmpty()) {
                // create notebook
                edam::Notebook notebookData;
                setEdamNotebookName(&notebookData, notebookName);
                edam::Notebook resultNotebook;
                if (!edamCreateNotebook(&noteStore, &resultNotebook, authToken, notebookData)) {
                    m_noteStoreHttpClient = 0;
                    return;
                }
                if (!resultNotebook.guid.empty()) {
                    notebookGuid = latin1StringFromStdString(resultNotebook.guid);
                    m_storageManager->setNotebookGuid(notebookId, notebookGuid);
                }
                qint32 resultUsn = resultNotebook.updateSequenceNum;
                if (resultUsn == (usn + 1)) { // nothing has changed in the server between our last pull and this push
                    usn = usn + 1;
                    m_storageManager->saveEvernoteSyncData("USN", usn);
                }
            }
        }

        // tags on the note
        const QStringList tagIdsList = noteDataMap.value("TagIds").toStringList();
        QStringList tagGuidsList;
        foreach (const QString &tagId, tagIdsList) {
            const QString tagName = m_storageManager->tagName(tagId);
            QString tagGuid = m_storageManager->guidForTagId(tagId);
            if (!tagId.isEmpty() && !tagName.isEmpty()) {
                if (tagGuid.isEmpty()) {
                    // create tag
                    edam::Tag tagData;
                    setEdamTagName(&tagData, tagName);
                    edam::Tag resultTag;
                    if (!edamCreateTag(&noteStore, &resultTag, authToken, tagData)) {
                        m_noteStoreHttpClient = 0;
                        return;
                    }
                    if (!resultTag.guid.empty()) {
                        tagGuid = latin1StringFromStdString(resultTag.guid);
                        m_storageManager->setTagGuid(tagId, tagGuid);
                    }
                    qint32 resultUsn = resultTag.updateSequenceNum;
                    if (resultUsn == (usn + 1)) { // nothing has changed in the server between our last push/pull and this push
                        usn = usn + 1;
                        m_storageManager->saveEvernoteSyncData("USN", usn);
                    }
                }
                if (!tagGuid.isEmpty()) {
                    tagGuidsList << tagGuid;
                }
            }
        }

        const bool contentValid = noteDataMap.value("ContentDataAvailable").toBool();
        const QByteArray contentHash = noteDataMap.value("ContentHash").toByteArray();
        if (!contentValid || contentHash.isEmpty()) {
            continue;
        }
        const QByteArray baseContentHash = noteDataMap.value("BaseContentHash").toByteArray();
        const QByteArray syncContentHash = noteDataMap.value("SyncContentHash").toByteArray();
        if (syncContentHash != baseContentHash) {
            // if we push, this will result in a conflict in the server
            // We anyway try to prevent this by pulling the note beforehand
            m_storageManager->log(QString("synchronize() Skipping push to avoid conflict"));
            continue;
        }
        bool isPushingContent = (noteGuid.isEmpty() || baseContentHash.isEmpty() || (contentHash != baseContentHash));
        QByteArray contentToPush;
        if (isPushingContent) {
            contentToPush = noteDataMap.value("Content").toByteArray();
        }

        edam::Note noteData;
        setEdamNoteTitle(&noteData, noteDataMap.value("Title").toString());
        setEdamNoteNotebookGuid(&noteData, notebookGuid);
        setEdamNoteTagGuids(&noteData, tagGuidsList);
        setEdamNoteUpdatedTime(&noteData, noteDataMap.value("UpdatedTime").toLongLong());
        setEdamNoteContentHash(&noteData, QByteArray::fromHex(contentHash));
        setEdamNoteIsActive(&noteData, (!noteDataMap.value("Trashed").toBool()));
        if (isPushingContent) {
            setEdamNoteContent(&noteData, contentToPush);
            bool ok = setEdamNoteResources(&noteData, m_storageManager->attachmentsData(noteId), noteGuid);
            if (!ok) {
                m_storageManager->log(QString("Could not set resources for push noteGuid=[%1]").arg(noteGuid));
                failedPushNotesCount++;
                continue; // skip this note
            }
        }
        setEdamNoteAttributes(&noteData, noteDataMap);

        edam::Note resultNote;
        if (noteGuid.isEmpty()) {
            NotePushResult result = edamCreateNote(&noteStore, &resultNote, authToken, noteData);
            m_storageManager->log(QString("synchronize() edamCreateNote completed result=%1").arg(static_cast<int>(result)));
            if (result != NotePushSuccess) {
                if (result == NotePushLoginError) {
                    emit finished(false, "Unable to login");
                    m_noteStoreHttpClient = 0;
                    return;
                } else if (result == NotePushCancelled) {
                    emit finished(false, "Cancelled");
                    m_noteStoreHttpClient = 0;
                    return;
                } else if (result == NotePushTimedOut) {
                    emit finished(false, "Request timed out. Please check your internet connection.");
                    m_noteStoreHttpClient = 0;
                    return;
                } else if (result == NotePushQuotaReachedError) {
                    uploadLimitReachedNotesCount++;
                    continue; // maybe the next note will be smaller and fit the remaining upload limit?
                } else {
                    m_storageManager->log(QString("synchronize() edamCreateNote failed content=[%1]").arg(QString::fromUtf8(contentToPush)));
                    failedPushNotesCount++;
                    continue; // just skip the bad note, we can atleast push the other notes
                }
            }
            if (!resultNote.guid.empty()) {
                m_storageManager->setNoteGuid(noteId, latin1StringFromStdString(resultNote.guid));
            }
        } else {
            setEdamNoteGuid(&noteData, noteGuid);
            NotePushResult result = edamUpdateNote(&noteStore, &resultNote, authToken, noteData);
            m_storageManager->log(QString("synchronize() edamUpdateNote completed result=%1").arg(static_cast<int>(result)));
            if (result != NotePushSuccess) {
                if (result == NotePushLoginError) {
                    emit finished(false, "Unable to login");
                    m_noteStoreHttpClient = 0;
                    return;
                } else if (result == NotePushCancelled) {
                    emit finished(false, "Cancelled");
                    m_noteStoreHttpClient = 0;
                    return;
                } else if (result == NotePushTimedOut) {
                    emit finished(false, "Request timed out. Please check your internet connection.");
                    m_noteStoreHttpClient = 0;
                    return;
                } else if (result == NotePushRateLimitReachedError) {
                    emit finished(false, "Temporarily exceeded Evernote server usage limits");
                    m_noteStoreHttpClient = 0;
                    return;
                } else if (result == NotePushQuotaReachedError) {
                    uploadLimitReachedNotesCount++;
                    continue; // maybe the next note will be smaller and fit the remaining upload limit?
                } else {
                    if (isPushingContent) {
                        m_storageManager->log(QString("synchronize() edamUpdateNote failed content=[%1]").arg(QString::fromUtf8(contentToPush)));
                    } else {
                        m_storageManager->log(QString("synchronize() edamUpdateNote failed"));
                    }
                    failedPushNotesCount++;
                    continue; // just skip the bad note, we can atleast push the other notes
                }
            }
            Q_ASSERT(noteGuid == latin1StringFromStdString(resultNote.guid));
        }
        if (isPushingContent) {
            m_storageManager->setPushedNote(noteId, utf8StringFromStdString(resultNote.title), contentToPush, contentHash, resultNote.updateSequenceNum, resultNote.updated);
            storeNoteAttachments(noteId, resultNote, m_storageManager, true /*isAfterPush*/);
        }
        m_storageManager->setNoteHasUnpushedChanges(noteId, false);
        qint32 resultUsn = resultNote.updateSequenceNum;
        if (resultUsn == (usn + 1)) { // nothing has changed in the server between our last push/pull and this push
            usn = usn + 1;
            m_storageManager->saveEvernoteSyncData("USN", usn);
        }
        pushedNotesCount++;
        emit syncProgressChanged(syncProgress(PUSHING_UPDATES, pushedNotesCount, notesToPush.count()));
    }

    { // update uploaded-bytes-in-this-cycle
        edam::SyncState syncState;
        if (!edamGetSyncState(&noteStore, &syncState, authToken)) {
            m_noteStoreHttpClient = 0;
            return;
        }
        m_storageManager->saveEvernoteSyncData("Account/UploadedBytesInThisCycle", static_cast<qint64>(syncState.uploaded));
    }

    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    m_storageManager->saveEvernoteSyncData("LastSyncedTime", now);
    if (failedPushNotesCount > 0) {
        emit finished(false, QString("Error sending changes for %1 %2").arg(failedPushNotesCount).arg(failedPushNotesCount == 1? "note" : "notes"));
    } else if (uploadLimitReachedNotesCount > 0){
        emit finished(false, "Some changes were not sent because you have reached the upload limit for this cycle.");
    } else {
        emit finished(true, syncCompletedMessage(newNotesCount, pushedNotesCount + pushedNotebooksCount + pushedTagsCount));
    }
    m_noteStoreHttpClient = 0;
}

QStringList EvernoteAccess::searchNotes(const QString &words, const QString &notebookId, const QString &tagId)
{
    QString authToken = m_storageManager->retrieveEncryptedEvernoteAuthData("authToken");

    QUrl noteUrl(m_storageManager->retrieveEvernoteAuthData("noteStoreUrl"));
    boost::shared_ptr<QThriftHttpClient> httpClient(new QThriftHttpClient(noteUrl, this));
    httpClient->setNetworkConfiguration(m_connectionManager->selectedConfiguration());
    m_noteStoreHttpClient = httpClient.get();
    boost::shared_ptr<thrift::protocol::TProtocol> noteProtocol(new thrift::protocol::TBinaryProtocol(httpClient));
    httpClient->open();
    edam::NoteStoreClient noteStore(noteProtocol);

    QStringList allMatchingNoteIds;
    edam::NoteFilter noteFilter;
    if (!words.isEmpty()) {
        noteFilter.words = stdStringFromUtf8String(words.simplified().toLower());
        noteFilter.__isset.words = true;
    }
    if (!notebookId.isEmpty()) {
        QString notebookGuid = m_storageManager->guidForNotebookId(notebookId);
        if (!notebookGuid.isEmpty()) {
            noteFilter.notebookGuid = stdStringFromLatin1String(notebookGuid);
            noteFilter.__isset.notebookGuid = true;
        }
    }
    if (!tagId.isEmpty()) {
        QString tagGuid = m_storageManager->guidForTagId(tagId);
        if (!tagGuid.isEmpty()) {
             noteFilter.tagGuids = std::vector<std::string>(1, stdStringFromLatin1String(tagGuid));
             noteFilter.__isset.tagGuids = true;
        }
    }
    noteFilter.order = edam::NoteSortOrder::UPDATED;
    noteFilter.ascending = false;
    noteFilter.inactive = false;
    noteFilter.__isset.order = true;
    noteFilter.__isset.ascending = true;
    noteFilter.__isset.inactive = true;

    const int maxNotes = 100;
    int offset = 0;
    forever {
        edam::NoteList noteList;
        if (!edamFindNotes(&noteStore, &noteList, authToken, noteFilter, offset, maxNotes)) {
            return QStringList();
        }
        QStringList pagedMatchingNotes;
        for (std::vector<edam::Note>::const_iterator notesIter = noteList.notes.begin();
             notesIter != noteList.notes.end();
             notesIter++) {
            const edam::Note &note = (*notesIter);
            QString guid = latin1StringFromStdString(note.guid);
            if (!guid.isEmpty()) {
                QString noteId = m_storageManager->noteIdForGuid(guid);
                if (noteId.isEmpty()) { // note was probably created another client after we last synced
                    noteId = storeNoteGist(note, m_storageManager);
                    storeNoteNotebooksTagsTrashedness(noteId, note, m_storageManager);
                    storeNoteAttachments(noteId, note, m_storageManager, false /*isAfterPush*/);
                }
                Q_ASSERT(!noteId.isEmpty());
                pagedMatchingNotes << noteId;
                allMatchingNoteIds << noteId;
            }
        }
        offset += noteList.notes.size();
        emit serverSearchMatchingNotes(pagedMatchingNotes, ((noteList.totalNotes > 0)? (offset * 100 / noteList.totalNotes) : 100));
        if (offset >= noteList.totalNotes) {
            break; // get out of the 'forever' loop
        }
    }
    return allMatchingNoteIds;
}

QString EvernoteAccess::fetchUserInfo(const QString &authToken)
{
    m_isSslErrorFound = false;
    QUrl userUrl(QString("https://%1/edam/user").arg(EVERNOTE_API_HOST));
    boost::shared_ptr<QThriftHttpClient> httpClient(new QThriftHttpClient(userUrl, this));
    httpClient->setNetworkConfiguration(m_connectionManager->selectedConfiguration());
    m_userStoreHttpClient = httpClient.get();
    boost::shared_ptr<thrift::protocol::TProtocol> protocol(new thrift::protocol::TBinaryProtocol(httpClient));
    httpClient->open();
    edam::UserStoreClient userStore(protocol);

    // check if our EDAM version is new enough to be supported by the server
    if (!edamCheckVersion(&userStore)) {
        m_userStoreHttpClient = 0;
        return QString();
    }

    // getUser
    edam::User user;
    if (!edamGetUser(&userStore, &user, authToken)) {
        m_userStoreHttpClient = 0;
        return QString();
    }

    // save user
    QString username = latin1StringFromStdString(user.username);
    m_storageManager->setActiveUser(username);
    m_storageManager->saveEvernoteAuthData("username", username);

    // save account info
    m_storageManager->saveEvernoteSyncData("Account/UploadLimitForThisCycle", static_cast<qint64>(user.accounting.uploadLimit));
    m_storageManager->saveEvernoteSyncData("Account/UploadLimitNextResetTimestamp", static_cast<qint64>(user.accounting.uploadLimitEnd));
    m_storageManager->saveEncryptedEvernoteAuthData("Account/IncomingEmail", utf8StringFromStdString(user.attributes.incomingEmailAddress));
    QString userType("Unknown");
    if (user.__isset.privilege) {
        switch (user.privilege) {
        case edam::PrivilegeLevel::NORMAL: userType = "Free"; break;
        case edam::PrivilegeLevel::PREMIUM: userType = "Premium"; break;
        case edam::PrivilegeLevel::MANAGER: userType = "Manager"; break;
        case edam::PrivilegeLevel::SUPPORT: userType = "Support"; break;
        case edam::PrivilegeLevel::ADMIN: userType = "Admin"; break;
        default: "Unknown"; break;
        }
    }
    m_storageManager->saveEvernoteSyncData("Account/UserType", userType);

    m_userStoreHttpClient = 0;
    return username;
}

bool EvernoteAccess::isSslErrorFound() const
{
    return m_isSslErrorFound;
}

// What follows are thin wrappers around EDAM function calls
// The main purpose of these wrappers is to handle EDAM exceptions within themselves

static void printUnknownExceptionError(const QString &during, const edam::EDAMUserException &e)
{
    qDebug() << "Error: EDAM user exception during " + during + ": " + QString::number(e.errorCode) + ": " + latin1StringFromStdString(e.parameter);
}

static void printUnknownExceptionError(const QString &during, const edam::EDAMSystemException &e)
{
    qDebug() << "Error: EDAM system exception during " + during + ": " + QString::number(e.errorCode) + ": " + latin1StringFromStdString(e.message);
}

static void printUnknownExceptionError(const QString &during, const edam::EDAMNotFoundException &e)
{
    qDebug() << "Error: EDAM not-found exception during " + during + ": " + latin1StringFromStdString(e.identifier) + ": " + latin1StringFromStdString(e.key);
}

static void printUnknownExceptionError(const QString &during, const thrift::transport::TTransportException &e)
{
    qDebug() << "Error: Thrift transport exception during " + during + ": Type " + QString::number(e.getType());
}

static void printUnknownExceptionError(const QString &during)
{
    qDebug() << "Error: General exception during " + during;
}

bool EvernoteAccess::edamCheckVersion(edam::UserStoreClient *userStore)
{
    bool versionOk = false;
    try {
        versionOk = userStore->checkVersion("Notekeeper", edam::g_UserStore_constants.EDAM_VERSION_MAJOR, edam::g_UserStore_constants.EDAM_VERSION_MINOR);
    } catch (const edam::EDAMUserException &e) {
        printUnknownExceptionError("checkVersion", e);
        emit loginError(QLatin1String("Unknown error"));
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const edam::EDAMSystemException &e) {
        printUnknownExceptionError("checkVersion", e);
        emit loginError(QLatin1String("Unknown error"));
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("checkVersion", e);
        emit loginError(QLatin1String("Unknown error"));
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            if (m_isSslErrorFound) {
                emit loginError(QLatin1String("There were SSL errors when talking to Evernote"));
                emit finished(false, "There were SSL errors when talking to Evernote");
            } else {
                m_storageManager->log("edamCheckVersion cancelled");
                emit loginError(QLatin1String("Login cancelled"));
                emit finished(false, QLatin1String("Cancelled"));
            }
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamCheckVersion timed out");
            emit loginError(QLatin1String("Request timed out. Please check your internet connection."));
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("checkVersion", e);
            emit loginError(QLatin1String("Unknown error"));
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("checkVersion");
        emit loginError(QLatin1String("Unknown error"));
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    if (!versionOk) {
        emit loginError(QLatin1String("Obsolete EDAM version"));
        emit finished(false, QLatin1String("Obsolete EDAM version"));
        return false;
    }
    return versionOk;
}

bool EvernoteAccess::edamGetUser(edam::UserStoreClient *userStore,
                 edam::User *user, const QString &authToken)
{
    try {
        userStore->getUser((*user), stdStringFromLatin1String(authToken));
    } catch (const edam::EDAMUserException &e) {
        if (e.errorCode == edam::EDAMErrorCode::INVALID_AUTH) {
            emit finished(false, QLatin1String("Access denied"));
            return false;
        }
        emit finished(false, QLatin1String("Unable to login"));
        return false;
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("User", e);
            emit loginError(QLatin1String("Unknown error"));
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("User", e);
        emit loginError(QLatin1String("Unknown error"));
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            if (m_isSslErrorFound) {
                emit loginError(QLatin1String("There were SSL errors when talking to Evernote"));
                emit finished(false, "There were SSL errors when talking to Evernote");
            } else {
                m_storageManager->log("edamUser cancelled");
                emit loginError(QLatin1String("Login cancelled"));
                emit finished(false, QLatin1String("Cancelled"));
            }
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamUser timed out");
            emit loginError(QLatin1String("Request timed out. Please check your internet connection."));
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("User", e);
            emit loginError(QLatin1String("Unknown error"));
            emit finished(false, QLatin1String("Unable to login"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("User");
        emit loginError(QLatin1String("Unknown error"));
        emit finished(false, QLatin1String("Unable to login"));
        return false;
    }
    return true;
}

bool EvernoteAccess::edamGetSyncState(edam::NoteStoreClient *noteStore,
                                    edam::SyncState *syncState, const QString &authToken)
{
    try {
        noteStore->getSyncState((*syncState), stdStringFromLatin1String(authToken));
    } catch (const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("GetSyncState", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("GetSyncState", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("GetSyncState", e);
            emit loginError(QLatin1String("Unknown error"));
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("GetSyncState", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            if (m_isSslErrorFound) {
                emit finished(false, "There were SSL errors when talking to Evernote");
            } else {
                m_storageManager->log("edamGetSyncState cancelled");
                emit finished(false, QLatin1String("Cancelled"));
            }
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamGetSyncState timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("GetSyncState", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("GetSyncState");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

bool EvernoteAccess::edamGetSyncChunk(edam::NoteStoreClient *noteStore,
                                    edam::SyncChunk *syncChunk,
                                    const QString &authToken, int usn, int maxEntries, bool fullSync)
{
    try {
        noteStore->getSyncChunk((*syncChunk), stdStringFromLatin1String(authToken), usn, maxEntries, fullSync);
    } catch (const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("GetSyncChunk", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("GetSyncChunk", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("GetSyncChunk", e);
            emit loginError(QLatin1String("Unknown error"));
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("GetSyncChunk", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamGetSyncChunk cancelled");
            emit finished(false, QLatin1String("Cancelled"));
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamGetSyncChunk timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("GetSyncChunk", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("GetSyncChunk");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

bool EvernoteAccess::edamGetNote(edam::NoteStoreClient *noteStore,
                               edam::Note *note,
                               const QString &authToken, const QString &guid,
                               bool withContent, bool withResources, bool withRecognition, bool withAlternateData)
{
    try {
        noteStore->getNote((*note), stdStringFromLatin1String(authToken), stdStringFromLatin1String(guid),
                           withContent, withResources, withRecognition, withAlternateData);
    } catch(const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("GetNote", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("GetNote", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("GetNote", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("GetNote", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamGetNote cancelled");
            emit finished(false, QLatin1String("Cancelled"));
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamGetNote timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("GetNote", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("GetNote");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

EvernoteAccess::NotePushResult EvernoteAccess::edamCreateNote(edam::NoteStoreClient *noteStore,
                                   edam::Note *note,
                                   const QString &authToken, const edam::Note &noteData)
{
    try {
        noteStore->createNote((*note), stdStringFromLatin1String(authToken), noteData);
    } catch(const edam::EDAMUserException &e) {
        if (e.errorCode == edam::EDAMErrorCode::QUOTA_REACHED) {
            m_storageManager->log(QString("edamCreateNote QUOTA_REACHED: %1").arg(latin1StringFromStdString(e.parameter)));
            return NotePushQuotaReachedError;
        } else if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                return NotePushLoginError;
            default:
                printUnknownExceptionError("CreateNote", e);
                return NotePushUnknownError;
            }
        } else {
            printUnknownExceptionError("CreateNote", e);
            return NotePushUnknownError;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return NotePushRateLimitReachedError;
        } else {
            printUnknownExceptionError("CreateNote", e);
            return NotePushUnknownError;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("CreateNote", e);
        return NotePushUnknownError;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamCreateNote cancelled");
            return NotePushCancelled;
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamCreateNote timed out");
            return NotePushTimedOut;
        } else {
            printUnknownExceptionError("CreateNote", e);
            return NotePushUnknownError;
        }
    } catch (...) {
        printUnknownExceptionError("CreateNote");
        return NotePushUnknownError;
    }
    return NotePushSuccess;
}

EvernoteAccess::NotePushResult EvernoteAccess::edamUpdateNote(edam::NoteStoreClient *noteStore,
                                  edam::Note *note,
                                  const QString &authToken, const edam::Note &noteData)
{
    try {
        noteStore->updateNote((*note), stdStringFromLatin1String(authToken), noteData);
    } catch(const edam::EDAMUserException &e) {
        if (e.errorCode == edam::EDAMErrorCode::QUOTA_REACHED) {
            m_storageManager->log(QString("edamCreateNote QUOTA_REACHED: %1").arg(latin1StringFromStdString(e.parameter)));
            return NotePushQuotaReachedError;
        } else if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                return NotePushLoginError;
            default:
                printUnknownExceptionError("UpdateNote", e);
                return NotePushUnknownError;
            }
        } else {
            printUnknownExceptionError("UpdateNote", e);
            return NotePushUnknownError;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return NotePushRateLimitReachedError;
        } else {
            printUnknownExceptionError("UpdateNote", e);
            return NotePushUnknownError;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("UpdateNote", e);
        return NotePushUnknownError;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamUpdateNote cancelled");
            return NotePushCancelled;
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamUpdateNote timed out");
            return NotePushTimedOut;
        } else {
            printUnknownExceptionError("UpdateNote", e);
            return NotePushUnknownError;
        }
    } catch (...) {
        printUnknownExceptionError("UpdateNote");
        return NotePushUnknownError;
    }
    return NotePushSuccess;
}

bool EvernoteAccess::edamFindNotes(edam::NoteStoreClient *noteStore,
                                   edam::NoteList *noteList,
                                   const QString &authToken, const edam::NoteFilter &noteFilter, qint32 offset, qint32 maxNotes)
{
    try {
        noteStore->findNotes((*noteList), stdStringFromLatin1String(authToken), noteFilter, offset, maxNotes);
    } catch(const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("FindNotes", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("FindNotes", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("FindNotes", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("FindNotes", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamFindNotes cancelled");
            emit finished(false, QLatin1String("Cancelled"));
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamFindNotes timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("FindNotes", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("FindNotes");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

bool EvernoteAccess::edamCreateNotebook(edam::NoteStoreClient *noteStore,
                                      edam::Notebook *notebook,
                                      const QString &authToken, const edam::Notebook &notebookData)
{
    try {
        noteStore->createNotebook((*notebook), stdStringFromLatin1String(authToken), notebookData);
    } catch(const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("CreateNotebook", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("CreateNotebook", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("CreateNotebook", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("CreateNotebook", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamCreateNotebook cancelled");
            emit finished(false, QLatin1String("Cancelled"));
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamCreateNotebook timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("CreateNotebook", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("CreateNotebook");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

bool EvernoteAccess::edamUpdateNotebook(edam::NoteStoreClient *noteStore,
                                        qint32 *usn,
                                        const QString &authToken, const edam::Notebook &notebookData)
{
    try {
        (*usn) = noteStore->updateNotebook(stdStringFromLatin1String(authToken), notebookData);
    } catch(const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("UpdateNotebook", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("UpdateNotebook", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("UpdateNotebook", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("UpdateNotebook", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamUpdateNotebook cancelled");
            emit finished(false, QLatin1String("Cancelled"));
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamUpdateNotebook timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("UpdateNotebook", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("UpdateNotebook");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

bool EvernoteAccess::edamCreateTag(edam::NoteStoreClient *noteStore,
                                 edam::Tag *tag,
                                 const QString &authToken, const edam::Tag &tagData)
{
    try {
        noteStore->createTag((*tag), stdStringFromLatin1String(authToken), tagData);
    } catch(const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("CreateTag", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("CreateTag", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("CreateTag", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("CreateTag", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamCreateTag cancelled");
            emit finished(false, QLatin1String("Cancelled"));
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamCreateTag timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("CreateTag", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("CreateTag");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

bool EvernoteAccess::edamUpdateTag(edam::NoteStoreClient *noteStore,
                                 qint32 *usn,
                                 const QString &authToken, const edam::Tag &tagData)
{
    try {
        (*usn) = noteStore->updateTag(stdStringFromLatin1String(authToken), tagData);
    } catch(const edam::EDAMUserException &e) {
        if (e.parameter == "authenticationToken") {
            switch (e.errorCode) {
            case edam::EDAMErrorCode::AUTH_EXPIRED:
            case edam::EDAMErrorCode::BAD_DATA_FORMAT:
            case edam::EDAMErrorCode::DATA_REQUIRED:
            case edam::EDAMErrorCode::INVALID_AUTH:
                // in all these cases, we should just try and get another auth token
                emit authTokenInvalid();
                emit finished(false, QLatin1String("Unable to login"));
                return false;
            default:
                printUnknownExceptionError("UpdateTag", e);
                emit finished(false, QLatin1String("Unknown error"));
                return false;
            }
        } else {
            printUnknownExceptionError("UpdateTag", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMSystemException &e) {
        if (e.errorCode == edam::EDAMErrorCode::RATE_LIMIT_REACHED) {
            emit rateLimitReached(e.rateLimitDuration);
            emit finished(false, QLatin1String("Temporarily exceeded Evernote server usage limits"));
            return false;
        } else {
            printUnknownExceptionError("UpdateTag", e);
            emit finished(false, QLatin1String("Unknown error"));
            return false;
        }
    } catch (const edam::EDAMNotFoundException &e) {
        printUnknownExceptionError("UpdateTag", e);
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    } catch (const thrift::transport::TTransportException &e) {
        if (e.getType() == thrift::transport::TTransportException::INTERRUPTED) {
            m_storageManager->log("edamUpdateTag cancelled");
            emit finished(false, QLatin1String("Cancelled"));
        } else if (e.getType() == thrift::transport::TTransportException::TIMED_OUT) {
            m_storageManager->log("edamUpdateTag timed out");
            emit finished(false, QLatin1String("Request timed out. Please check your internet connection."));
        } else {
            printUnknownExceptionError("UpdateTag", e);
            emit finished(false, QLatin1String("Unknown error"));
        }
        return false;
    } catch (...) {
        printUnknownExceptionError("UpdateTag");
        emit finished(false, QLatin1String("Unknown error"));
        return false;
    }
    return true;
}

void EvernoteAccess::handleSslErrors(const QList<QSslError> &errors)
{
    qDebug() << errors.count() << "SSL errors encountered:";
    foreach (const QSslError &se, errors) {
        qDebug() << QString("SSL Error: [%1] %2").arg(static_cast<int>(se.error())).arg(se.errorString());
    }
    qDebug() << "Peer certificate chain:";
    QList<QSslCertificate> peerCerts = m_sslConfig.peerCertificateChain();
    if (peerCerts.length() > 0) {
        foreach (const QSslCertificate &cert, peerCerts) {
            qDebug() << "CERT" << (cert.isValid()? "(valid)" : "(invalid)");
            qDebug() << "S" << cert.subjectInfo(QSslCertificate::CommonName) << "I" << cert.issuerInfo(QSslCertificate::CommonName);
            qDebug() << QString("Effective: %1; Expires: %2;").arg(cert.effectiveDate().toString("dd MMM yyyy")).arg(cert.expiryDate().toString("dd MMM yyyy"));
            qDebug() << "Serial number" << cert.serialNumber();
        }
    } else {
        qDebug() << "No peer certificates";
    }
    if (errors.count() > 0) {
        m_isSslErrorFound = true;
        emit sslErrorsFound();
    }
}
