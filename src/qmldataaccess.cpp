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

#include "qmldataaccess.h"
#include "storage/storagemanager.h"
#include "connectionmanager.h"
#include "evernoteaccess.h"
#include "evernotemarkup.h"
#include "notekeeper_config.h"
#include "storage/diskcache/shareddiskcache.h"
#include "storage/noteslistmodel.h"
#include "searchlocalnotesthread.h"
#include "qmlnetworkdiskcache.h"
#include "qmlnetworkcookiejar.h"
#include "qexifreader.h"

#include <QDateTime>
#include <QStringRef>
#include <QStringBuilder>
#include <QNetworkDiskCache>
#include <QUrl>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QInputContext>
#include <QDesktopServices>
#include <QTransform>
#include "qplatformdefs.h"

#ifndef QT_SIMULATOR
#include "Limits_constants.h"
#endif

QmlDataAccess::QmlDataAccess(StorageManager *storageManager, ConnectionManager *connectionManager, QObject *parent)
    : QObject(parent)
    , m_storageManager(storageManager)
#ifndef QT_SIMULATOR
    , m_evernoteAccess(new EvernoteAccess(storageManager, connectionManager, this))
#endif
    , m_allNotesListModel(new NotesListModel(storageManager, this))
    , m_notebookNotesListModel(new NotesListModel(storageManager, this))
    , m_tagNotesListModel(new NotesListModel(storageManager, this))
    , m_searchNotesListModel(new NotesListModel(storageManager, this))
    , m_searchLocalNotesThread(0)
    , m_searchResultsCount(0)
    , m_unsearchedNotesCount(0)
    , m_searchProgressPercentage(0)
    , m_attachmentDownloadedBytes(0)
    , m_addingAttachmentStatus(QVariantMap())
{
    connect(m_storageManager, SIGNAL(notesListChanged(StorageConstants::NotesListType,QString)), SLOT(storageManagerNotesListChanged(StorageConstants::NotesListType,QString)));
    connect(m_storageManager, SIGNAL(notebooksListChanged()), SIGNAL(notebooksListChanged()));
    connect(m_storageManager, SIGNAL(tagsListChanged()), SIGNAL(tagsListChanged()));

    connect(m_storageManager, SIGNAL(textAddedToLog(QString)), SIGNAL(textAddedToLog(QString)));
    connect(m_storageManager, SIGNAL(logCleared()), SIGNAL(logCleared()));

    m_offlineStatusChangedTimer.setSingleShot(true);
    m_offlineStatusChangedTimer.setInterval(5 * 1000); // triggered 5 seconds after a note has its offline status changed
    connect(&m_offlineStatusChangedTimer, SIGNAL(timeout()), SLOT(tryResolveOfflineStatusChanges()));
    tryResolveOfflineStatusChanges(); // in case the app quit earlier when the timer was running

    bool diskCacheSizeOk = false;
    qint64 diskCacheSize = retrieveStringSetting("DiskCache/MaxSize").toLongLong(&diskCacheSizeOk);
    if (diskCacheSizeOk && diskCacheSize > 0) {
        SharedDiskCache::setSharedDiskCacheSize(diskCacheSize);
    }

    m_isDarkColorSchemeEnabled = m_storageManager->retrieveSetting("ColorScheme/dark");
}

void QmlDataAccess::setSslConfiguration(const QSslConfiguration &sslConf)
{
#ifndef QT_SIMULATOR
    m_evernoteAccess->setSslConfiguration(sslConf);
#endif
}

NotesListModel *QmlDataAccess::allNotesListModel() const
{
    return m_allNotesListModel;
}

NotesListModel *QmlDataAccess::notebookNotesListModel() const
{
    return m_notebookNotesListModel;
}

NotesListModel *QmlDataAccess::tagNotesListModel() const
{
    return m_tagNotesListModel;
}

NotesListModel *QmlDataAccess::searchNotesListModel() const
{
    return m_searchNotesListModel;
}

void QmlDataAccess::storageManagerNotesListChanged(StorageConstants::NotesListType whichNotes, const QString &objectId)
{
    emit notesListChanged(static_cast<int>(whichNotes), objectId);
}


static void separateTitleAndContent(const QString *titleAndContent, QStringRef *title, QStringRef *content)
{
    int firstNL = titleAndContent->indexOf('\n');
    if (firstNL < 0) { // if there's no newline in the text
        (*title) = QStringRef(titleAndContent); // the whole thing is the title
        (*content) = QStringRef(titleAndContent, 0, 0); // and the content is empty
        return;
    }
    (*title) = QStringRef(titleAndContent, 0, firstNL);
    (*content) = QStringRef(titleAndContent, firstNL + 1, titleAndContent->length() - (firstNL + 1));
}

QVariant QmlDataAccess::saveNote(const QString &noteId, const QString &titleAndContent, const QVariantList &attachmentsData,
                                const QString &previousTitle, const QString &previousPlainTextContent, const QByteArray &previousBaseContentHash)
{
    QVariantMap returnMap;
    if (noteId.isEmpty()) { // note yet to be created
        QString title;
        QByteArray enmlContent;
        EvernoteMarkup::enmlFromPlainText(titleAndContent, attachmentsData, &title, &enmlContent);
        QString createdNoteId = m_storageManager->createNote(title, enmlContent);
        returnMap["NoteId"] = createdNoteId;
        returnMap["Created"] = true;
        returnMap["NoChange"] = false;
        m_storageManager->setNoteHasUnpushedChanges(createdNoteId);
    } else { // note exists
        returnMap["NoteId"] = noteId;
        returnMap["Created"] = false;
        returnMap["NoChange"] = true;
        QStringRef titleRef, plainTextContentRef;
        separateTitleAndContent(&titleAndContent, &titleRef, &plainTextContentRef);
        if (plainTextContentRef.compare(previousPlainTextContent) != 0) { // if content has changed
            QString title;
            QByteArray enmlContent;
            EvernoteMarkup::enmlFromPlainText(titleAndContent, attachmentsData, &title, &enmlContent);
            m_storageManager->updateNoteTitleAndContent(noteId, title, enmlContent, previousBaseContentHash);
            returnMap["NoChange"] = false;
            m_storageManager->setNoteHasUnpushedChanges(noteId);
        } else if (titleRef.compare(previousTitle) != 0) { // if only title has changed
            m_storageManager->updateNoteTitle(noteId, titleRef.toString());
            returnMap["NoChange"] = false;
            m_storageManager->setNoteHasUnpushedChanges(noteId);
        }
    }
    bool isEnmlChanged = false;
    QByteArray enml = m_storageManager->enmlContentFromContentIni(returnMap.value("NoteId").toString(), &isEnmlChanged);
    returnMap["IsEnmlChanged"] = isEnmlChanged;
    if (isEnmlChanged) {
        returnMap["Enml"] = enml;
    }
    return returnMap;
}

QString QmlDataAccess::createNoteWithUrl(const QString &titleAndContent, const QString &sourceUrl)
{
    QString title;
    QByteArray enmlContent;
    EvernoteMarkup::enmlFromPlainText(titleAndContent, QVariantList(), &title, &enmlContent);
    QString createdNoteId = m_storageManager->createNote(title, enmlContent, sourceUrl);
    m_storageManager->setNoteHasUnpushedChanges(createdNoteId);
    return createdNoteId;
}

bool QmlDataAccess::saveCheckboxStates(const QString &noteId, const QVariantList &checkboxStates, const QByteArray &enmlContent, const QByteArray &previousBaseContentHash)
{
    bool saved = m_storageManager->saveCheckboxStates(noteId, checkboxStates, enmlContent, previousBaseContentHash);
    if (saved) {
        m_storageManager->setNoteHasUnpushedChanges(noteId);
    }
    return saved;
}

QVariant QmlDataAccess::appendTextToNote(const QString &noteId, const QString &text, const QByteArray &enmlContent, const QByteArray &baseContentHash)
{
    QByteArray updatedEnml, htmlToAdd;
    bool ok = m_storageManager->appendTextToNote(noteId, text, enmlContent, baseContentHash, &updatedEnml, &htmlToAdd);
    if (ok) {
        QVariantMap returnMap;
        returnMap["Appended"] = true;
        returnMap["UpdatedEnml"] = updatedEnml;
        returnMap["HtmlToAdd"] = QString::fromUtf8(htmlToAdd);
        m_storageManager->setNoteHasUnpushedChanges(noteId);
        return returnMap;
    }
    return QVariantMap();
}

static QImage thumbnailFromImageFile(const QString &fileName, int side = 75)
{
    QImageReader imageReader(fileName);
    if (!imageReader.canRead()) {
        return QImage();
    }
    QSize imageSize = imageReader.size();
    QSize scaledSize = imageSize;
    if (imageSize.width() > side || imageSize.height() > side) {
        scaledSize.scale(QSize(side, side), Qt::KeepAspectRatioByExpanding);
    }
    QRect scaledClipRect((scaledSize.width() - side) / 2, (scaledSize.height() - side) / 2, side, side);
    imageReader.setScaledSize(scaledSize);
    imageReader.setScaledClipRect(scaledClipRect);
    return imageReader.read();
}

QVariantMap QmlDataAccess::removeAttachmentInstanceFromNote(const QString &noteId, const QString &titleAndContent, const QVariantList &_attachmentsData, int attachmentIndexToRemove, const QByteArray &baseContentHash)
{
    if (noteId.isEmpty()) {
        return QVariantMap();
    }
    if (attachmentIndexToRemove < 0 || attachmentIndexToRemove >= _attachmentsData.size()) {
        return QVariantMap();
    }

    QVariantList attachmentsData = _attachmentsData;
    QVariantMap map = attachmentsData.takeAt(attachmentIndexToRemove).toMap(); // remove this instance

    // See if the same attachment is used multiple times in this note; if not, remove attachment
    // Also, see if we need to (and are able to) update the thumbnail for this note
    QByteArray md5Hash = map.value("Hash").toByteArray();
    QString mimeType = map.value("MimeType").toString();
    bool isRemovingThumbnailSourceImage = (mimeType.startsWith("image/") && !md5Hash.isEmpty() && (md5Hash == m_storageManager->noteThumbnailSourceHash(noteId)));
    QPair<QString /*fileName*/, QByteArray /*md5*/> anotherImage; // we'll pick another locally available image to create a new thumbnail with
    bool otherInstancesExist = false;
    foreach (const QVariant &v, attachmentsData) {
        const QVariantMap map = v.toMap();
        QByteArray hash = map.value("Hash").toByteArray();
        if (hash == md5Hash) {
            otherInstancesExist = true;
        } else if (isRemovingThumbnailSourceImage) {
            QString mimeType = map.value("MimeType").toString();
            if (mimeType.startsWith("image/")) {
                QString urlStr = map.value("Url").toString();
                QUrl url(urlStr);
                if (url.isValid() && url.scheme() == "file") {
                    anotherImage = qMakePair(url.toLocalFile(), hash);
                }
            }
        }
        if (otherInstancesExist && (!isRemovingThumbnailSourceImage || !anotherImage.second.isEmpty())) {
            break;
        }
    }
    if (!otherInstancesExist) { // if there are no other instances of this attachment in this note
        m_storageManager->removeAttachmentData(noteId, md5Hash); // remove from attachments.ini
        m_storageManager->removeAttachmentFile(noteId, map); // remove the file
    }

    // Construct enml
    QString title;
    QByteArray enmlContent;
    EvernoteMarkup::enmlFromPlainText(titleAndContent, attachmentsData, &title, &enmlContent);
    m_storageManager->updateNoteTitleAndContent(noteId, title, enmlContent, baseContentHash);
    m_storageManager->setNoteHasUnpushedChanges(noteId);

    if (isRemovingThumbnailSourceImage) { // update the thumbnail for this note
        if (anotherImage.first.isEmpty() || anotherImage.second.isEmpty()) {
            m_storageManager->setNoteThumbnail(noteId, QImage());
        } else {
            m_storageManager->setNoteThumbnail(noteId, thumbnailFromImageFile(anotherImage.first), anotherImage.second);
        }
    }

    QVariantMap returnMap;
    returnMap["Removed"] = true;
    returnMap["Enml"] = enmlContent;
    returnMap["AttachmentsData"] = attachmentsData;
    return returnMap;
}

bool QmlDataAccess::setNoteTitle(const QString &noteId, const QString &title)
{
    bool updated = m_storageManager->updateNoteTitle(noteId, title);
    if (updated) {
        m_storageManager->setNoteHasUnpushedChanges(noteId);
    }
    return updated;
}

void QmlDataAccess::startListNotes(int whichNotes, const QString &objectId /* notebook/tag id */)
{
    StorageConstants::NotesListType type = static_cast<StorageConstants::NotesListType>(whichNotes);  // Doesn't work from QML if we use the enum in the API
    NotesListModel *notesListModel = 0;
    if (type == StorageConstants::AllNotes) {
        notesListModel = m_allNotesListModel;
    } else if (type == StorageConstants::NotesInNotebook ||
               type == StorageConstants::FavouriteNotes ||
               type == StorageConstants::TrashNotes) {
        notesListModel = m_notebookNotesListModel;
    } else if (type == StorageConstants::NotesWithTag) {
        notesListModel = m_tagNotesListModel;
    }
    ListNotesThread *listNotesThread = new ListNotesThread(notesListModel, type, objectId, this);
    if (type == StorageConstants::AllNotes) {
        connect(listNotesThread, SIGNAL(gotNotesList()), SIGNAL(gotAllNotesList()));
    } else {
        connect(listNotesThread, SIGNAL(gotNotesList()), SIGNAL(gotNotesList()));
    }
    listNotesThread->start();
}

void QmlDataAccess::startGetNoteData(const QString &noteId)
{
    GetNoteDataThread *getNoteDataThread = new GetNoteDataThread(m_storageManager, m_evernoteAccess, noteId, this);
    connect(getNoteDataThread, SIGNAL(gotNoteTitle(QString)), SIGNAL(gotNoteTitle(QString)));
    connect(getNoteDataThread, SIGNAL(gotNoteData(QVariantMap)), SIGNAL(gotNoteData(QVariantMap)));
    connect(getNoteDataThread, SIGNAL(fetchNoteDataFinished(bool,QString)), SLOT(fetchNoteDataFinished(bool,QString)));
    connect(getNoteDataThread, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));
    getNoteDataThread->start();
}

void QmlDataAccess::fetchNoteDataFinished(bool success, const QString &message)
{
    if (!success) {
        emit errorFetchingNote(message);
    }
}

void QmlDataAccess::startSearchLocalNotes(const QString &words)
{
    SearchLocalNotesThread *searchLocalNotesThread = new SearchLocalNotesThread(m_storageManager, words, this);
    connect(searchLocalNotesThread, SIGNAL(searchLocalNotesMatchingNote(QString)), SLOT(searchLocalNotesMatchObtained(QString)));
    connect(searchLocalNotesThread, SIGNAL(searchLocalNotesProgressPercentage(int)), SLOT(searchLocalNotesProgressPercentageChanged(int)));
    connect(searchLocalNotesThread, SIGNAL(searchLocalNotesFinished(int)), SLOT(searchLocalNotesThreadFinished(int)));
    m_searchLocalNotesThread = searchLocalNotesThread;
    if (m_searchResultsCount != 0) {
        m_searchResultsCount = 0;
        emit searchResultsCountChanged();
    }
    if (m_unsearchedNotesCount != 0) {
        m_unsearchedNotesCount = 0;
        emit unsearchedNotesCountChanged();
    }
    if (m_searchProgressPercentage != 0) {
        m_searchProgressPercentage = 0;
        emit searchProgressPercentageChanged();
    }
    searchLocalNotesThread->start();
}

void QmlDataAccess::searchLocalNotesMatchObtained(const QString &noteId)
{
    int matchingNotesCount = m_searchNotesListModel->appendNoteIds(QStringList() << noteId);
    if (m_searchResultsCount != matchingNotesCount) {
        m_searchResultsCount = matchingNotesCount;
        emit searchResultsCountChanged();
    }
}

void QmlDataAccess::searchLocalNotesProgressPercentageChanged(int searchProgressPercentage)
{
    if (m_searchProgressPercentage != searchProgressPercentage) {
        m_searchProgressPercentage = searchProgressPercentage;
        emit searchProgressPercentageChanged();
    }
}

void QmlDataAccess::searchLocalNotesThreadFinished(int unsearchedNotesCount)
{
    m_searchLocalNotesThread = 0;
    if (m_unsearchedNotesCount != unsearchedNotesCount) {
        m_unsearchedNotesCount = unsearchedNotesCount;
        emit unsearchedNotesCountChanged();
    }
    emit searchLocalNotesFinished();
}

void QmlDataAccess::startSearchNotes(const QString &words)
{
    SearchServerNotesThread *searchServerNotesThread = new SearchServerNotesThread(m_storageManager, m_evernoteAccess, words, this);
    connect(searchServerNotesThread, SIGNAL(searchServerNotesResultObtained(QStringList,int)), SLOT(searchServerNotesResultObtained(QStringList,int)));
    connect(searchServerNotesThread, SIGNAL(searchServerNotesFinished()), SIGNAL(searchServerNotesFinished()));
    connect(searchServerNotesThread, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));

    if (m_searchProgressPercentage != 0) {
        m_searchProgressPercentage = 0;
        emit searchProgressPercentageChanged();
    }

    searchServerNotesThread->start();
}

void QmlDataAccess::searchServerNotesResultObtained(const QStringList &noteIdsList, int searchProgressPercentage)
{
    int matchingNotesCount = m_searchNotesListModel->appendNoteIds(noteIdsList);
    if (m_searchResultsCount != matchingNotesCount) {
        m_searchResultsCount = matchingNotesCount;
        emit searchResultsCountChanged();
    }
    if (m_searchProgressPercentage != searchProgressPercentage) {
        m_searchProgressPercentage = searchProgressPercentage;
        emit searchProgressPercentageChanged();
    }
}

void QmlDataAccess::startAttachmentDownload(const QString &urlString, const QString &fileName, const QString &fileExtension)
{
    AttachmentDownloadThread *attachmentDownloadThread = new AttachmentDownloadThread(m_storageManager, m_evernoteAccess, urlString, fileName, fileExtension, this);
    connect(attachmentDownloadThread, SIGNAL(attachmentDownloadProgressed(qint64)), SLOT(attachmentDownloadProgressed(qint64)));
    connect(attachmentDownloadThread, SIGNAL(attachmentDownloaded(QString)), SIGNAL(attachmentDownloaded(QString)));
    connect(attachmentDownloadThread, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));
    if (m_attachmentDownloadedBytes != 0) {
        m_attachmentDownloadedBytes = 0;
        emit attachmentDownloadedBytesChanged();
    }
    attachmentDownloadThread->start();
}

void QmlDataAccess::attachmentDownloadProgressed(qint64 bytesDownloaded)
{
    if (m_attachmentDownloadedBytes != bytesDownloaded) {
        m_attachmentDownloadedBytes = bytesDownloaded;
        emit attachmentDownloadedBytesChanged();
    }
}

qint64 QmlDataAccess::attachmentDownloadedBytes() const
{
    return m_attachmentDownloadedBytes;
}

bool QmlDataAccess::startAppendImageToNote(const QString &noteId, const QString &imageUrl, const QByteArray &enmlContent, const QByteArray &baseContentHash)
{
    QString imagePath;
#ifndef QT_SIMULATOR
    QUrl url(imageUrl);
    if (url.isValid() && url.scheme() == "file") {
        imagePath = url.toLocalFile();
    }
#else
    imagePath = imageUrl;
#endif
    if (!QFile::exists(imagePath)) {
        return false;
    }
    QImageReader imageReader(imagePath);
    if (!imageReader.canRead()) {
        return false;
    }
    QByteArray imageFormat = imageReader.format();
    if (imageFormat.isEmpty()) {
        return false;
    }
    QString mimeType = QString("image/%1").arg(QLatin1String(imageFormat.constData()));
    QVariantMap attachment;
    attachment["FilePath"] = imagePath;
    attachment["MimeType"] = mimeType;
    AddAttachmentsThread *appendImageThread = new AddAttachmentsThread(m_storageManager, noteId, (QVariantList() << attachment), enmlContent, baseContentHash, this);
    connect(appendImageThread, SIGNAL(addingAttachmentStatusChanged(QVariantMap)), SLOT(updateAddingAttachmentStatus(QVariantMap)));
    connect(appendImageThread, SIGNAL(addedAttachment(QVariantMap)), SIGNAL(addedAttachment(QVariantMap)));
    connect(appendImageThread, SIGNAL(doneAddingAttachments(bool,QString,QString)), SIGNAL(doneAddingAttachments(bool,QString,QString)));
    connect(this, SIGNAL(cancelAddingAttachmentPrivateSignal()), appendImageThread, SLOT(cancel()));
    if (!m_addingAttachmentStatus.isEmpty()) {
        m_addingAttachmentStatus = QVariantMap();
        emit addingAttachmentStatusChanged();
    }
    appendImageThread->start(QThread::LowPriority);
    return true;
}

bool QmlDataAccess::startAppendAttachmentsToNote(const QString &noteId, const QVariantList &attachments, const QByteArray &enmlContent, const QByteArray &baseContentHash)
{
    AddAttachmentsThread *appendAttachmentsThread = new AddAttachmentsThread(m_storageManager, noteId, attachments, enmlContent, baseContentHash, this);
    connect(appendAttachmentsThread, SIGNAL(addingAttachmentStatusChanged(QVariantMap)), SLOT(updateAddingAttachmentStatus(QVariantMap)));
    connect(appendAttachmentsThread, SIGNAL(addedAttachment(QVariantMap)), SIGNAL(addedAttachment(QVariantMap)));
    connect(appendAttachmentsThread, SIGNAL(doneAddingAttachments(bool,QString,QString)), SIGNAL(doneAddingAttachments(bool,QString,QString)));
    connect(this, SIGNAL(cancelAddingAttachmentPrivateSignal()), appendAttachmentsThread, SLOT(cancel()));
    if (!m_addingAttachmentStatus.isEmpty()) {
        m_addingAttachmentStatus = QVariantMap();
        emit addingAttachmentStatusChanged();
    }
    appendAttachmentsThread->start(QThread::LowPriority);
    return true;
}

void QmlDataAccess::updateAddingAttachmentStatus(const QVariantMap &status)
{
    if (m_addingAttachmentStatus != status) {
        m_addingAttachmentStatus = status;
        emit addingAttachmentStatusChanged();
    }
}

QVariantMap QmlDataAccess::addingAttachmentStatus() const
{
    return m_addingAttachmentStatus;
}

int QmlDataAccess::searchResultsCount() const
{
    return m_searchResultsCount;
}

int QmlDataAccess::unsearchedNotesCount() const
{
    return m_unsearchedNotesCount;
}

int QmlDataAccess::searchProgressPercentage() const
{
    return m_searchProgressPercentage;
}

QString QmlDataAccess::createNotebook(const QString &name, const QString &firstNoteId) // returns notebookId
{
    QString notebookId = m_storageManager->createNotebook(name, firstNoteId);
    if (!firstNoteId.isEmpty()) {
        m_storageManager->setNoteHasUnpushedChanges(firstNoteId);
    }
    return notebookId;
}

bool QmlDataAccess::setNotebookForNote(const QString &noteId, const QString &notebookId)
{
    QString currentNotebook = notebookForNote(noteId);
    bool changed = m_storageManager->setNotebookForNote(noteId, notebookId);
    if (changed) {
        m_storageManager->setNoteHasUnpushedChanges(noteId);
        QStringList offlineNotebooks = m_storageManager->offlineNotebookIds();
        bool noteWasInOfflineNotebook = offlineNotebooks.contains(currentNotebook);
        bool noteIsNowInOfflineNotebook = offlineNotebooks.contains(notebookId);
        if (noteWasInOfflineNotebook != noteIsNowInOfflineNotebook) {
            m_storageManager->setOfflineStatusChangeUnresolvedNote(noteId, true);
            m_offlineStatusChangedTimer.start();
        }
    }
    return changed;
}

QString QmlDataAccess::notebookForNote(const QString &noteId) // returns notebookId
{
    return m_storageManager->notebookForNote(noteId);
}

bool QmlDataAccess::renameNotebook(const QString &notebookId, const QString &name)
{
    bool renamed = m_storageManager->renameNotebook(notebookId, name);
    if (renamed) {
        m_storageManager->setNotebookHasUnpushedChanges(notebookId);
    }
    return renamed;
}

QString QmlDataAccess::defaultNotebookName()
{
    return m_storageManager->defaultNotebookName();
}

void QmlDataAccess::setOfflineNotebookIds(const QString &offlineNotebookIdsStr)
{
    QStringList offlineNotebookIds;
    if (!offlineNotebookIdsStr.isEmpty()) {
        offlineNotebookIds = offlineNotebookIdsStr.split(',');
    }
    m_storageManager->setOfflineNotebookIds(offlineNotebookIds);
    emit offlineNotebooksCountChanged();
}

QStringList QmlDataAccess::offlineNotebookIds()
{
    return m_storageManager->offlineNotebookIds();
}

int QmlDataAccess::offlineNotebooksCount()
{
    return offlineNotebookIds().count();
}

void QmlDataAccess::startListNotebooks(int /*StorageConstants::NotebookTypes*/ notebookTypes)
{
    StorageConstants::NotebookTypes notebookSelector = static_cast<StorageConstants::NotebookTypes>(notebookTypes);
    ListNotebooksThread *listNotebooksThread = new ListNotebooksThread(m_storageManager, notebookSelector, this);
    connect(listNotebooksThread, SIGNAL(gotNotebooksList(int, QVariantList)), SIGNAL(gotNotebooksList(int, QVariantList)));
    listNotebooksThread->start();
}

QString QmlDataAccess::createTag(const QString &name, const QString &firstNoteId) // returns tagId
{
    QString tagId = m_storageManager->createTag(name, firstNoteId);
    if (!firstNoteId.isEmpty()) {
        m_storageManager->setNoteHasUnpushedChanges(firstNoteId);
    }
    return tagId;
}

bool QmlDataAccess::setTagsOnNote(const QString &noteId, const QString &tagIds /* comma separated */)
{
    bool changed = m_storageManager->setTagsOnNote(noteId, tagIds);
    if (changed) {
        m_storageManager->setNoteHasUnpushedChanges(noteId);
    }
    return changed;
}

bool QmlDataAccess::renameTag(const QString &tagId, const QString &name)
{
    bool renamed = m_storageManager->renameTag(tagId, name);
    if (renamed) {
        m_storageManager->setTagHasUnpushedChanges(tagId);
    }
    return renamed;
}

void QmlDataAccess::startListTagsWithTagsOnNoteChecked(const QString &noteId)
{
    ListTagsWithTagsOnNoteCheckedThread *listTagsThread = new ListTagsWithTagsOnNoteCheckedThread(m_storageManager, noteId, this);
    connect(listTagsThread, SIGNAL(gotTagsListWithTagsOnNoteChecked(QVariantList)), SIGNAL(gotTagsListWithTagsOnNoteChecked(QVariantList)));
    listTagsThread->start();
}

void QmlDataAccess::startListTags()
{
    ListTagsThread *listTagsThread = new ListTagsThread(m_storageManager, this);
    connect(listTagsThread, SIGNAL(gotTagsList(QVariantList)), SIGNAL(gotTagsList(QVariantList)));
    listTagsThread->start();
}

bool QmlDataAccess::setFavouriteNote(const QString &noteId, bool isFavourite)
{
    bool changed = m_storageManager->setFavouriteNote(noteId, isFavourite);
    if (changed) {
        m_storageManager->setNoteHasUnpushedChanges(noteId);
        m_storageManager->setOfflineStatusChangeUnresolvedNote(noteId, true);
        m_offlineStatusChangedTimer.start();
    }
    return changed;
}

void QmlDataAccess::tryResolveOfflineStatusChanges()
{
    m_storageManager->tryResolveOfflineStatusChanges();
}

void QmlDataAccess::unscheduleTryResolveOfflineStatusChanges()
{
    if (m_offlineStatusChangedTimer.isActive()) {
        m_offlineStatusChangedTimer.stop();
    }
}

bool QmlDataAccess::moveNoteToTrash(const QString &noteId)
{
    bool changed = m_storageManager->moveNoteToTrash(noteId);
    if (changed) {
        QString guid = m_storageManager->guidForNoteId(noteId);
        bool noteHasSynced = (!guid.isEmpty());
        // if note has never synced, it need not be pushed since it's now gotten trashed
        m_storageManager->setNoteHasUnpushedChanges(noteId, noteHasSynced);
    }
    return changed;
}

bool QmlDataAccess::restoreNoteFromTrash(const QString &noteId)
{
    bool changed = m_storageManager->restoreNoteFromTrash(noteId);
    if (changed) {
        m_storageManager->setNoteHasUnpushedChanges(noteId);
    }
    return changed;
}

bool QmlDataAccess::expungeNoteFromTrash(const QString &noteId)
{
    bool expunged = m_storageManager->expungeNoteFromTrash(noteId);
    if (expunged) { // only unpushed notes can be expunged
        m_storageManager->setNoteHasUnpushedChanges(noteId, false); // we don't have to push it
    }
    return expunged;
}

void QmlDataAccess::saveSetting(const QString &key, bool value)
{
    m_storageManager->saveSetting(key, value);
}

bool QmlDataAccess::retrieveSetting(const QString &key)
{
    return m_storageManager->retrieveSetting(key);
}

void QmlDataAccess::saveStringSetting(const QString &key, const QString &value)
{
    m_storageManager->saveStringSetting(key, value);
}

QString QmlDataAccess::retrieveStringSetting(const QString &key)
{
    return m_storageManager->retrieveStringSetting(key);
}

void QmlDataAccess::saveEvernoteUsernamePassword(const QString &username, const QString &password)
{
    m_storageManager->saveEvernoteAuthData("username", username);
    m_storageManager->saveEncryptedEvernoteAuthData("password", password);
    m_storageManager->saveEncryptedEvernoteAuthData("authToken", ""); // force verification of new password
}

bool QmlDataAccess::isAuthTokenValid()
{
    return (!m_storageManager->retrieveEncryptedEvernoteAuthData("authToken").isEmpty());
}

bool QmlDataAccess::isLoggedIn()
{
    if (activeUser().isEmpty()) {
        return false;
    }
    if (!m_storageManager->isAuthDataValid()) {
        return false;
    }
#ifdef QT_SIMULATOR
    return true;
#endif
    return m_storageManager->retrieveEvernoteSyncData("FullSyncDone").toBool();
}

bool QmlDataAccess::isLoggedOutBecauseLoginMethodIsObsolete()
{
    if (activeUser().isEmpty()) {
        return false;
    }
    QString password = m_storageManager->retrieveEncryptedEvernoteAuthData("password");
    bool fullSyncDone = m_storageManager->retrieveEvernoteSyncData("FullSyncDone").toBool();
    return (!password.isEmpty() && fullSyncDone);
}

bool QmlDataAccess::hasUserLoggedInBefore()
{
    if (activeUser().isEmpty()) {
        return false;
    }
#ifdef QT_SIMULATOR
    return true;
#endif
    return m_storageManager->retrieveEvernoteSyncData("FullSyncDone").toBool();
}

bool QmlDataAccess::hasUserLoggedInBeforeByName(const QString &username)
{
    return m_storageManager->retrieveEvernoteSyncDataForUser(username, "FullSyncDone").toBool();
}

void QmlDataAccess::setActiveUser(const QString &username)
{
    m_storageManager->setActiveUser(username);
}

QString QmlDataAccess::activeUser()
{
    return m_storageManager->activeUser();
}

void QmlDataAccess::logout()
{
    emit aboutToLogout();
    QString currentActiveUser = m_storageManager->activeUser();
    if (!currentActiveUser.isEmpty()) {
        m_storageManager->purgeEvernoteAuthData();
    }
    m_storageManager->setActiveUser("");
    clearCookieJar();

    // revert all noteslistmodels back to an uninitialized state
    m_allNotesListModel->clearNotesListQuery();
    m_notebookNotesListModel->clearNotesListQuery();
    m_tagNotesListModel->clearNotesListQuery();
    m_searchNotesListModel->clearNotesListQuery();
}

void QmlDataAccess::clearCookieJar()
{
    QmlNetworkCookieJar::instance()->clearCookieJar();
}

void QmlDataAccess::purgeActiveUserData()
{
    m_storageManager->purgeActiveUserData();
}

void QmlDataAccess::setReadyForCreateNoteRequests(bool isReady)
{
    m_storageManager->setReadyForCreateNoteRequests(isReady);
}

QString QmlDataAccess::loggedText()
{
    return m_storageManager->loggedText();
}

void QmlDataAccess::clearLog()
{
    m_storageManager->clearLog();
}

QVariantMap QmlDataAccess::userAccountData()
{
    QVariantMap map;
    qint64 uploadLimitForThisCycle = m_storageManager->retrieveEvernoteSyncData("Account/UploadLimitForThisCycle").toLongLong();
    qint64 uploadLimitNextResetTimestamp = m_storageManager->retrieveEvernoteSyncData("Account/UploadLimitNextResetTimestamp").toLongLong();
    if (uploadLimitForThisCycle == 0 || uploadLimitNextResetTimestamp == 0) {
        return map;
    }
    map["AccountDataValid"] = true;
    map["UploadLimitForThisCycle"] = uploadLimitForThisCycle;
    map["UploadLimitNextResetsAt"] = QDateTime::fromMSecsSinceEpoch(uploadLimitNextResetTimestamp);
    map["UploadedBytesInThisCycle"] = m_storageManager->retrieveEvernoteSyncData("Account/UploadedBytesInThisCycle").toLongLong();
    QString evernoteHost(EVERNOTE_API_HOST);
    evernoteHost.replace("www.", "m.");
    QString incomingEmail = m_storageManager->retrieveEncryptedEvernoteAuthData("Account/IncomingEmail");
    if (!incomingEmail.isEmpty()) {
        map["IncomingEmail"] = QString(incomingEmail % "@" % evernoteHost);
    }
    map["UserType"] = m_storageManager->retrieveEvernoteSyncData("Account/UserType").toString();

    return map;
}

bool QmlDataAccess::removeTempFile(const QString &tempFile)
{
    if (!QFile::exists(tempFile)) {
        return false;
    }
    if (m_storageManager->unremovedTempFiles().contains(tempFile)) {
        bool ok = QFile::remove(tempFile);
        if (ok) {
            m_storageManager->setUnremovedTempFile(tempFile, false);
            return true;
        }
    }
    return false;
}

void QmlDataAccess::removeAllPendingTempFiles()
{
    QStringList allUnremovedTempFiles = m_storageManager->unremovedTempFiles();
    foreach (const QString &tempFile, allUnremovedTempFiles) {
        if ((!QFile::exists(tempFile)) || QFile::remove(tempFile)) {
            m_storageManager->setUnremovedTempFile(tempFile, false);
        }
    }
}

bool QmlDataAccess::createDir(const QString &drive, const QString &path)
{
    QFileInfo fileInfo(drive + path);
    if (fileInfo.exists()) {
        if (fileInfo.isDir()) {
            return true;
        } else {
            return false;
        }
    }
    return QDir(drive).mkpath(path);
}

bool QmlDataAccess::fileExistsInFull(const QString &path, qint64 size)
{
    QFile file(path);
    return (file.exists() && (file.size() == size));
}

QVariantMap QmlDataAccess::diskCacheInfo()
{
    QVariantMap map;
    map["Used"] = SharedDiskCache::instance()->cacheSize();
    map["Max"] = SharedDiskCache::instance()->maximumCacheSize();
    return map;
}

void QmlDataAccess::clearDiskCache()
{
    SharedDiskCache::instance()->clear();
}

void QmlDataAccess::setDiskCacheMaximumSize(qint64 bytes)
{
    if (bytes <= 0) {
        return;
    }
    SharedDiskCache::instance()->setMaximumCacheSize(bytes);
    saveStringSetting("DiskCache/MaxSize", QString::number(bytes));
}

void QmlDataAccess::setRecentSearchQuery(const QString &searchQuery)
{
    m_storageManager->setRecentSearchQuery(searchQuery);
}

QStringList QmlDataAccess::recentSearchQueries()
{
    return m_storageManager->recentSearchQueries();
}

void QmlDataAccess::clearRecentSearchQueries()
{
    m_storageManager->clearRecentSearchQueries();
}

bool QmlDataAccess::resetPreeditText() const
{
    QInputContext *inputContext = qApp->inputContext();
    if (inputContext->isComposing()) {
        inputContext->reset();
    }
}

int QmlDataAccess::rotationForCorrectOrientation(const QString &imageUrlString) const
{
    QUrl imageUrl(imageUrlString);
    QString localImagePath = imageUrl.toLocalFile();
    QExifReader *exifReader = 0;
    QIODevice *cachedData = 0;
    if (!localImagePath.isEmpty()) {
        exifReader = new QExifReader(localImagePath);
    } else {
        cachedData = SharedDiskCache::instance()->data(imageUrl);
        if (cachedData) {
            exifReader = new QExifReader(cachedData);
        }
    }
    int correctionAngle = 0;
    if (exifReader) {
        exifReader->setIncludeKeys(QStringList() << "Orientation");
        int orientationCode = exifReader->read().value("Orientation", 0).toInt();
        if (orientationCode == 3) {
            correctionAngle = 180;
        } else if (orientationCode == 6) {
            correctionAngle = 90;
        } else if (orientationCode == 8) {
            correctionAngle = 270;
        }
    }
    delete exifReader;
    delete cachedData;
    return correctionAngle;
}

bool QmlDataAccess::cancel()
{
    bool wasBusy = false;
#ifndef QT_SIMULATOR
    wasBusy = m_evernoteAccess->cancel();
#endif
    if (m_searchLocalNotesThread) {
        m_searchLocalNotesThread->cancel();
    }
    return (wasBusy || m_searchLocalNotesThread != 0);
}

void QmlDataAccess::cancelAddingAttachment()
{
    emit cancelAddingAttachmentPrivateSignal();
}

void QmlDataAccess::setDarkColorSchemeEnabled(bool enabled)
{
    if (m_isDarkColorSchemeEnabled != enabled) {
        m_isDarkColorSchemeEnabled = enabled;
        m_storageManager->saveSetting("ColorScheme/dark", enabled);
        emit isDarkColorSchemeEnabledChanged();
    }
}

bool QmlDataAccess::isDarkColorSchemeEnabled() const
{
    return m_isDarkColorSchemeEnabled;
}

int QmlDataAccess::qtVersionRuntimeMinor() const
{
    QString runtimeQtVersion(qVersion());
    QRegExp regex("(\\d)\\.(\\d)\\.(\\d)");
    if (regex.indexIn(runtimeQtVersion) < 0) {
        qDebug() << "Invalid Qt version" << runtimeQtVersion;
        return 0;
    }
    return regex.cap(2).toInt();
}

QString QmlDataAccess::harmattanPicturesDir() const
{
#if defined(MEEGO_EDITION_HARMATTAN) || defined(QT_SIMULATOR)
    return QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
#else
    return QString();
#endif
}

QString QmlDataAccess::harmattanDocumentsDir() const
{
#if defined(MEEGO_EDITION_HARMATTAN) || defined(QT_SIMULATOR)
    return QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
    return QString();
#endif
}

QVariantMap QmlDataAccess::harmattanReadAndRemoveCreateNoteRequest() const
{
#if defined(MEEGO_EDITION_HARMATTAN) || defined(QT_SIMULATOR)
    QString createNoteRequestIniPath = (m_storageManager->notesDataLocation() % "/Store/Session/createNoteRequest.ini");
    if (!QFile::exists(createNoteRequestIniPath)) {
        return QVariantMap();
    }
    QSettings *createNoteRequestIni = new QSettings(createNoteRequestIniPath, QSettings::IniFormat);
    bool requestedAtOk = false;
    qint64 requestedAt = createNoteRequestIni->value("RequestedAt").toLongLong(&requestedAtOk);
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if ((!requestedAtOk) || ((now - requestedAt) > 15 * 1000)) { // if request was not initiated in the last 15 seconds, ignore request
        delete createNoteRequestIni;
        createNoteRequestIni = 0;
        QFile::remove(createNoteRequestIniPath);
        return QVariantMap();
    }
    QVariantMap map;
    if (createNoteRequestIni->contains("Title")) {
        map["Title"] = createNoteRequestIni->value("Title").toString();
    }
    if (createNoteRequestIni->contains("Url")) {
        map["Url"] = createNoteRequestIni->value("Url").toString();
    }
    if (createNoteRequestIni->contains("Attachments")) {
        QStringList attachments = createNoteRequestIni->value("Attachments").toStringList();
        QVariantList attachmentsList;
        QString attachmentFilePath, attachmentMimeType;
        QStringList attachmentFileNames;
        for (int i = 0; i < attachments.length(); i++) {
            if ((i % 2) == 0) {
                attachmentFilePath = attachments.at(i);
                QString baseName = QFileInfo(attachmentFilePath).baseName();
                if (!baseName.isEmpty()) {
                    attachmentFileNames << baseName;
                }
            } else {
                attachmentMimeType = attachments.at(i);
                if ((!attachmentFilePath.isEmpty()) && QFile::exists(attachmentFilePath) &&
                    (!attachmentMimeType.isEmpty()) && (attachmentMimeType.indexOf('/') > 0)) {
                    QVariantMap attachmentMap;
                    attachmentMap["FilePath"] = attachmentFilePath;
                    attachmentMap["MimeType"] = attachmentMimeType;
                    attachmentsList << attachmentMap;
                }
            }
        }
        if (!attachmentsList.isEmpty()) {
            map["Attachments"] = attachmentsList;
            if (map.value("Title").toString().isEmpty() && (!attachmentFileNames.isEmpty())) {
                map["Title"] = attachmentFileNames.join(", ");
            }
        }
    }
    if (map.contains("Title") || map.contains("Url") || map.contains("Attachments")) {
        delete createNoteRequestIni;
        createNoteRequestIni = 0;
        bool fileRemoved = QFile::remove(createNoteRequestIniPath);
        if (fileRemoved) {
            map["isValidRequest"] = true;
            return map;
        }
        return QVariantMap();
    }
    delete createNoteRequestIni;
    createNoteRequestIni = 0;
    QFile::remove(createNoteRequestIniPath);
#endif
    return QVariantMap();
}

bool QmlDataAccess::harmattanRemoveCreateNoteRequest() const
{
#if defined(MEEGO_EDITION_HARMATTAN) || defined(QT_SIMULATOR)
    QString createNoteRequestIniPath = (m_storageManager->notesDataLocation() % "/Store/Session/createNoteRequest.ini");
    if (!QFile::exists(createNoteRequestIniPath)) {
        return false;
    }
    return QFile::remove(createNoteRequestIniPath);
#endif
    return false;
}

// Threaded operations

ListNotesThread::ListNotesThread(NotesListModel *notesListModel, StorageConstants::NotesListType whichNotes, const QString &objectId, QObject *parent)
    : QThread(parent), m_notesListModel(notesListModel), m_whichNotes(whichNotes), m_objectId(objectId)
{
    Q_ASSERT(notesListModel != 0);
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void ListNotesThread::run()
{
    m_notesListModel->setNotesListQuery(m_whichNotes, m_objectId);
    emit gotNotesList();
}

ListNotebooksThread::ListNotebooksThread(StorageManager *storageManager, StorageConstants::NotebookTypes whichNotebooks, QObject *parent)
    : QThread(parent), m_storageManager(storageManager), m_whichNotebooks(whichNotebooks)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void ListNotebooksThread::run()
{
    QVariantList notebooksList = m_storageManager->listNotebooks(m_whichNotebooks);
    emit gotNotebooksList(m_whichNotebooks, notebooksList);
}

ListTagsThread::ListTagsThread(StorageManager *storageManager, QObject *parent)
    : QThread(parent), m_storageManager(storageManager)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void ListTagsThread::run()
{
    QVariantList tagsList = m_storageManager->listTags();
    emit gotTagsList(tagsList);
}

ListTagsWithTagsOnNoteCheckedThread::ListTagsWithTagsOnNoteCheckedThread(StorageManager *storageManager, const QString &noteId, QObject *parent)
    : QThread(parent), m_storageManager(storageManager), m_noteId(noteId)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void ListTagsWithTagsOnNoteCheckedThread::run()
{
    QVariantList tagsList = m_storageManager->listTagsWithTagsOnNoteChecked(m_noteId);
    emit gotTagsListWithTagsOnNoteChecked(tagsList);
}

GetNoteDataThread::GetNoteDataThread(StorageManager *storageManager, EvernoteAccess *evernoteAccess, const QString &noteId, QObject *parent)
    : QThread(parent), m_storageManager(storageManager), m_evernoteAccess(evernoteAccess), m_noteId(noteId)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void GetNoteDataThread::run()
{
    QVariantMap data = m_storageManager->noteData(m_noteId);
    QString title = data.value("Title").toString();
    emit gotNoteTitle(title);

    bool contentDataAvailable = data.take("ContentDataAvailable").toBool();
    QByteArray enmlContent;
    QVariantList attachmentsData;
    if (contentDataAvailable) {
        enmlContent = data.take("Content").toByteArray();
        attachmentsData = m_storageManager->attachmentsData(m_noteId);
    } else {
        QVariantMap fetchedData;
        bool noteFetched = false;
#ifndef QT_SIMULATOR
        connect(m_evernoteAccess, SIGNAL(finished(bool, QString)), SIGNAL(fetchNoteDataFinished(bool, QString)));
        connect(m_evernoteAccess, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));
        noteFetched = m_evernoteAccess->fetchNote(m_noteId, &fetchedData);
        m_evernoteAccess->disconnect(this);
#endif
        if (!noteFetched) {
            return;
        }
        enmlContent = fetchedData.value("Content").toByteArray();
        attachmentsData = fetchedData.value("AttachmentsData").toList();
        data["Title"] = fetchedData.value("Title").toString();
        data["ContentHash"] = data["BaseContentHash"] = fetchedData.value("ContentHash").toByteArray();
        data["CreatedTime"] = QDateTime::fromMSecsSinceEpoch(fetchedData.value("CreatedTime").toLongLong());
        data["UpdatedTime"] = QDateTime::fromMSecsSinceEpoch(fetchedData.value("UpdatedTime").toLongLong());
        data["BaseContentUSN"] = fetchedData.value("USN").toInt();
    }

    QString attachmentUrlBase = m_storageManager->retrieveEvernoteAuthData("webApiUrlPrefix") % "res";
    QString qmlResourceBase = m_storageManager->property("qmlResourceBase").toString();

    bool isPlainText;
    QString parsedContent;
    QVariantList allAttachments, imageAttachments, checkboxStates;
    QByteArray convertedEnmlContent;
    bool parsed = EvernoteMarkup::parseEvernoteMarkup(enmlContent, &isPlainText, &parsedContent, true /*shouldReturnHtml*/, attachmentsData, attachmentUrlBase, qmlResourceBase, &allAttachments, &imageAttachments, &checkboxStates, &convertedEnmlContent);
    data["IsPlainText"] = isPlainText;
    data["ParsedContent"] = parsedContent;
    data["AllAttachments"] = allAttachments;
    data["ImageAttachments"] = imageAttachments;
    data["CheckboxStates"] = checkboxStates;
    data["EnmlContent"] = (convertedEnmlContent.isEmpty()? enmlContent : convertedEnmlContent);
    data["IsEnmlParsingErrored"] = (!parsed);
    data["NonImageAttachmentsCount"] = qMax(0, allAttachments.count() - imageAttachments.count());

    if (!convertedEnmlContent.isEmpty()) {
        m_storageManager->log(QString("Info: Wrong XML encoding in note [%1] with title [%2] has been auto-fixed during ENML parsing").arg(data.value("guid").toString()).arg(data.value("Title").toString()));
    }
    emit gotNoteData(data);
    return;
}

SearchServerNotesThread::SearchServerNotesThread(StorageManager *storageManager, EvernoteAccess *evernoteAccess, const QString &words, QObject *parent)
    : QThread(parent), m_storageManager(storageManager), m_evernoteAccess(evernoteAccess), m_words(words)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
#ifndef QT_SIMULATOR
    connect(m_evernoteAccess, SIGNAL(serverSearchMatchingNotes(QStringList,int)), SIGNAL(searchServerNotesResultObtained(QStringList,int)));
    connect(m_evernoteAccess, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));
#endif
}

void SearchServerNotesThread::run()
{
 #ifndef QT_SIMULATOR
    m_evernoteAccess->searchNotes(m_words);
#endif
    emit searchServerNotesFinished();
}

AttachmentDownloadThread::AttachmentDownloadThread(StorageManager *storageManager, EvernoteAccess *evernoteAccess, const QString &urlStr, const QString &fileName, const QString &fileExtension, QObject *parent)
    : QThread(parent), m_storageManager(storageManager), m_evernoteAccess(evernoteAccess), m_urlStr(urlStr), m_fileName(fileName), m_fileExtension(fileExtension)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
    if (!m_urlStr.isEmpty() && QUrl(m_urlStr).scheme() != "file") {
#ifndef QT_SIMULATOR
        connect(m_evernoteAccess, SIGNAL(webResourceFetchProgressed(qint64)), this, SIGNAL(attachmentDownloadProgressed(qint64)));
        connect(m_evernoteAccess, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));
#endif
    }
}

void AttachmentDownloadThread::run()
{
    m_storageManager->log(QString("Downloading attachment from url [%1] to file [%2] / extension [%3]").arg(m_urlStr).arg(m_fileName).arg(m_fileExtension));
    if (m_urlStr.isEmpty()) {
        emit attachmentDownloaded(QString());
        return;
    }

    bool useTempFileName = (m_fileName.isEmpty());
    QString tempFileLocation = QDir::tempPath();
    if (!QFile::exists(tempFileLocation)) {
#ifdef MEEGO_EDITION_HARMATTAN
        tempFileLocation = "/tmp";
#else // if symbian
        tempFileLocation = "C:/";
#endif
    }

    QString tempFileNameTemplate = tempFileLocation + "/notekeeper_tmp_XXXXXX";
    if (!m_fileExtension.isEmpty()) {
        tempFileNameTemplate = tempFileNameTemplate % "." % m_fileExtension;
    }
    QTemporaryFile tempFile(tempFileNameTemplate);
    QUrl url(m_urlStr);
    if (url.scheme() == "file") { // local url
        QFile sourceFile(url.toLocalFile());
        bool ok1 = sourceFile.open(QFile::ReadOnly);
        bool ok2 = tempFile.open();
        if (!ok1 || !ok2) {
            emit attachmentDownloaded(QString());
            return;
        }
        int bytesCopied = 0;
        while (!sourceFile.atEnd()) {
            QByteArray data = sourceFile.read(1 << 18); // max 256 kb
            bytesCopied += tempFile.write(data);
            emit attachmentDownloadProgressed(bytesCopied);
        }
    } else {
        QNetworkAccessManager nwAccessManager;
        nwAccessManager.setCache(new QmlNetworkDiskCache); // nwAccessManager takes ownership
#ifndef QT_SIMULATOR
        bool ok = m_evernoteAccess->fetchEvernoteWebResource(url, &nwAccessManager, &tempFile, true /*emitProgress*/);
        m_storageManager->log(QString("fetchEvernoteWebResource ok=%1 tempFile=[%2]").arg(ok).arg(tempFile.fileName()));
#else
        bool ok = false;
#endif
        if (!ok) {
            emit attachmentDownloaded(QString());
            return;
        }
    }
    if (useTempFileName) { // Leave the file in the temp dir
        m_storageManager->setUnremovedTempFile(tempFile.fileName());
        tempFile.setAutoRemove(false);
        m_storageManager->log(QString("No filename for saving specified. Returning [%1]").arg(tempFile.fileName()));
        emit attachmentDownloaded(tempFile.fileName());
        return;
    } else { // Rename the file to the required fileName
        Q_ASSERT(!m_fileName.isEmpty());
        QString targetFileName = m_fileName;
        QFileInfo fileInfo(targetFileName);
        if (QFile::exists(targetFileName)) {
            QString targetTemplate = fileInfo.absolutePath() % "/" % fileInfo.completeBaseName() % "_%1" % (fileInfo.suffix().isEmpty()? "" : ".") % fileInfo.suffix();
            int n = 1;
            while (QFile::exists(targetTemplate.arg(n))) {
                n++;
            }
            targetFileName = targetTemplate.arg(n);
        }
        bool ok = tempFile.rename(targetFileName);
        m_storageManager->log(QString("Renamed to [%1] ok=%2").arg(targetFileName).arg(ok));
        if (ok) {
            tempFile.setAutoRemove(false);
            emit attachmentDownloaded(targetFileName);
            return;
        } else {
            emit attachmentDownloaded(QString());
            return;
        }
    }
    emit attachmentDownloaded(QString());
}

class ImageReadCancelHelper : public QFile
{
public:
    ImageReadCancelHelper(const QString &fileName, QObject *parent = 0)
        : QFile(fileName), m_cancelled(false) { }
    void cancel() { m_cancelled = true; }
    virtual qint64 readData(char *data, qint64 maxlen) {
        if (m_cancelled) {
            return 0;
        }
        return QFile::readData(data, maxlen);
    }
    virtual bool atEnd() const {
        if (m_cancelled) {
            return true;
        }
        return QFile::atEnd();
    }
private:
    bool m_cancelled;
};

static bool imageDoesNeedResizing(QImageReader *imageReader, int maxDimension, QSize *actualSize, QSize *requiredSize)
{
    if (!imageReader->canRead()) {
        (*actualSize) = QSize();
        (*requiredSize) = QSize();
        return false;
    }
    QByteArray imageFormat = imageReader->format();
    if (imageFormat.isEmpty()) {
        (*actualSize) = QSize();
        (*requiredSize) = QSize();
        return false;
    }
    QSize imageSize = imageReader->size();
    QSize scaledSize = imageSize;
    if (imageSize.width() > maxDimension || imageSize.height() > maxDimension) {
        scaledSize.scale(QSize(maxDimension, maxDimension), Qt::KeepAspectRatio);
    }
    (*actualSize) = imageSize;
    (*requiredSize) = scaledSize;
    return (scaledSize != imageSize);
}

static bool resizeAndRotateImage(QImageReader *imageReader, QSize scaledSize, int rotationAngle, QString *resultImagePath)
{
    if (!imageReader->canRead()) {
        return false;
    }
    QByteArray imageFormat = imageReader->format();
    if (imageFormat.isEmpty()) {
        return false;
    }
    QTemporaryFile tempScaledImage("notekeeper_tmp");
    imageReader->setScaledSize(scaledSize);
    QImage image = imageReader->read();
    if (!image.isNull()) {
        if (rotationAngle) {
            QTransform rotation;
            rotation.rotate(rotationAngle);
            image = image.transformed(rotation, Qt::SmoothTransformation);
        }
        tempScaledImage.open();
        QImageWriter imageWriter(&tempScaledImage, imageFormat);
        imageWriter.write(image);
        tempScaledImage.setAutoRemove(false);
        (*resultImagePath) = tempScaledImage.fileName();
        return true;
    }
    return false;
}

static bool canNoteAccomodateAttachment(const QString &attachmentToAdd, StorageManager *storageManager, const QString &noteId)
{
    qint64 newNoteAttachmentFileSize = QFileInfo(attachmentToAdd).size();
    qint64 existingNoteAttachmentsTotalSize = storageManager->noteAttachmentsTotalSize(noteId);
    QString evernoteUserType = storageManager->retrieveEvernoteSyncData("Account/UserType").toString().toLower();
    bool isFreeUser = (evernoteUserType == "free" || evernoteUserType == "unknown" || evernoteUserType.isEmpty());
#ifndef QT_SIMULATOR
    qint64 maxNoteSize = 26214400;
    if (isFreeUser) {
        maxNoteSize = static_cast<qint64>(evernote::limits::g_Limits_constants.EDAM_NOTE_SIZE_MAX_FREE);
    } else {
        maxNoteSize = static_cast<qint64>(evernote::limits::g_Limits_constants.EDAM_NOTE_SIZE_MAX_PREMIUM);
    }
#else
    qint64 maxNoteSize = 1 << 20; // 1 MB
#endif
    return ((existingNoteAttachmentsTotalSize + newNoteAttachmentFileSize) <= maxNoteSize);
}

static bool appendAttachment(const QString &attachmentToAdd, StorageManager *storageManager, const QString &noteId, const QByteArray &enmlContent, const QByteArray &baseContentHash, const QVariantMap &attributes, QVariantMap *_returnData)
{
    QString mimeType = attributes.value("MimeType", QString("")).toString();
    if (mimeType.isEmpty()) {
        return false;
    }
    bool isImage = false;
    QSize imageDimensions;
    if (mimeType.startsWith("image/", Qt::CaseInsensitive)) {
        isImage = true;
        imageDimensions = attributes.value("Dimensions", QSize()).toSize();
        if (imageDimensions.isEmpty()) {
            return false;
        }
    }

    QByteArray updatedEnml, htmlToAdd, hash;
    QString absoluteFilePath;
    bool ok = storageManager->appendAttachmentToNote(noteId, attachmentToAdd, attributes, enmlContent, baseContentHash, &updatedEnml, &htmlToAdd, &hash, &absoluteFilePath);
    if (ok) {
        QVariantMap returnMap;
        returnMap["Appended"] = true;
        returnMap["NoteId"] = noteId;
        returnMap["IsImage"] = isImage;
        returnMap["UpdatedEnml"] = updatedEnml;
        returnMap["HtmlToAdd"] = QString::fromUtf8(htmlToAdd);
        returnMap["guid"] = QString("");
        returnMap["Hash"] = hash;
        returnMap["MimeType"] = mimeType;
        returnMap["TrailingText"] = QString("");
        returnMap["Url"] = "file:///" + absoluteFilePath;
        if (isImage) {
            Q_ASSERT(!imageDimensions.isEmpty());
            returnMap["Width"] = imageDimensions.width();
            returnMap["Height"] = imageDimensions.height();
        }
        returnMap["FileName"] = attributes.value("FileName", QString("")).toString();
        returnMap["Size"] = QFileInfo(attachmentToAdd).size();
        returnMap["MimeType"] = mimeType;
        storageManager->setNoteHasUnpushedChanges(noteId);
        if (isImage && (!storageManager->noteThumbnailExists(noteId))) {
            storageManager->setNoteThumbnail(noteId, thumbnailFromImageFile(attachmentToAdd), hash);
        }
        (*_returnData) = returnMap;
        return true;
    }
    (*_returnData) = QVariantMap();
    return false;
}

AddAttachmentsThread::AddAttachmentsThread(StorageManager *storageManager, const QString &noteId, const QVariantList &attachmentsToAdd, const QByteArray &enmlContent, const QByteArray &baseContentHash, QObject *parent)
    : QThread(parent)
    , m_storageManager(storageManager)
    , m_noteId(noteId)
    , m_attachmentsToAdd(attachmentsToAdd)
    , m_enmlContent(enmlContent)
    , m_baseContentHash(baseContentHash)
    , m_cancelled(false)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
    connect(this, SIGNAL(finished()), SLOT(resetAddingAttachmentStatus()));
}

void AddAttachmentsThread::run()
{
    QVariantMap status;
    status["TotalFileCount"] = m_attachmentsToAdd.count();
    int currentFileIndex = 0;

    QString maxImageDimensionString = m_storageManager->retrieveStringSetting("ImageAttachments/maxDimensionInPixels");
    bool conversionOk = false;
    int maxImageDimension = maxImageDimensionString.toInt(&conversionOk);
    if (!conversionOk) {
        maxImageDimension = 2400;
    }

    QByteArray currentEnmlContent = m_enmlContent;
    foreach (const QVariant &attachment, m_attachmentsToAdd) {
        currentFileIndex++;
        const QVariantMap attachmentMap = attachment.toMap();
        QString sourceFilePathToAttach = attachmentMap.value("FilePath").toString();
        QString mimeType = attachmentMap.value("MimeType").toString();
        bool isImage = mimeType.startsWith("image/", Qt::CaseInsensitive);

        if (m_cancelled) {
            emit doneAddingAttachments(false, "Cancelled", QFileInfo(sourceFilePathToAttach).fileName());
            return;
        }

        status["CurrentFileIndex"] = currentFileIndex;
        status["CurrentFilePath"] = QFileInfo(sourceFilePathToAttach).absoluteFilePath();
        status["CurrentFileName"] = QFileInfo(sourceFilePathToAttach).fileName();
        status["MimeType"] = mimeType;

        status["Message"] = "Preparing";
        emit addingAttachmentStatusChanged(status);

        if (sourceFilePathToAttach.isEmpty() || (!QFile::exists(sourceFilePathToAttach))) {
            emit doneAddingAttachments(false, "File not found", QFileInfo(sourceFilePathToAttach).fileName());
            return;
        }

        QString filePathToAttach = sourceFilePathToAttach; // this will change if image-resizing is to be done
        QString fileToRemoveAtEnd; // the resized image file, if any, should be deleted after attaching it to the note
        QSize imageDimensions;
        if (isImage) {
            m_imageResizeHelper = QSharedPointer<ImageReadCancelHelper>(new ImageReadCancelHelper(sourceFilePathToAttach));
            QImageReader imageReader(m_imageResizeHelper.data());

            QSize actualImageSize, scaledImageSize;
            bool shouldResize = imageDoesNeedResizing(&imageReader, maxImageDimension, &actualImageSize, &scaledImageSize);
            if (m_cancelled) {
                emit doneAddingAttachments(false, "Cancelled", QFileInfo(sourceFilePathToAttach).fileName());
                return;
            }
            if (actualImageSize.isEmpty()) {
                emit doneAddingAttachments(false, "Empty image", QFileInfo(sourceFilePathToAttach).fileName());
                return;
            }

            int correctionAngle = 0;
            {
                QExifReader exifReader(sourceFilePathToAttach);
                exifReader.setIncludeKeys(QStringList() << "Orientation");
                int exifOrientationCode = exifReader.read().value("Orientation", 0).toInt();
                if (exifOrientationCode == 3) {
                    correctionAngle = 180;
                } else if (exifOrientationCode == 6) {
                    correctionAngle = 90;
                } else if (exifOrientationCode == 8) {
                    correctionAngle = 270;
                }
            }

            imageDimensions = actualImageSize;
            if (shouldResize || correctionAngle) {
                if (shouldResize) {
                    status["Message"] = "Resizing";
                } else {
                    status["Message"] = "Rotating";
                }
                emit addingAttachmentStatusChanged(status);

                QString scaledImagePath;
                bool ok = resizeAndRotateImage(&imageReader, scaledImageSize, correctionAngle, &scaledImagePath);
                filePathToAttach = scaledImagePath;
                fileToRemoveAtEnd = scaledImagePath;
                imageDimensions = scaledImageSize;
                if (correctionAngle == 90 || correctionAngle == 270) {
                    imageDimensions.transpose();
                }
                if (m_cancelled) {
                    emit doneAddingAttachments(false, "Cancelled", QFileInfo(sourceFilePathToAttach).fileName());
                    if (!fileToRemoveAtEnd.isEmpty() && QFile::exists(fileToRemoveAtEnd)) {
                        QFile::remove(fileToRemoveAtEnd);
                    }
                    return;
                }
                if (!ok) {
                    emit doneAddingAttachments(false, "Cannot resize image", QFileInfo(sourceFilePathToAttach).fileName());
                    if (!fileToRemoveAtEnd.isEmpty() && QFile::exists(fileToRemoveAtEnd)) {
                        QFile::remove(fileToRemoveAtEnd);
                    }
                    return;
                }
            }
        } // Close of if (isImage)

        Q_ASSERT(!filePathToAttach.isEmpty());

        if (! canNoteAccomodateAttachment(filePathToAttach, m_storageManager, m_noteId)) {
            emit doneAddingAttachments(false, "Adding this file would make this note too big to be synced to Evernote.", QFileInfo(sourceFilePathToAttach).fileName());
            if (!fileToRemoveAtEnd.isEmpty() && QFile::exists(fileToRemoveAtEnd)) {
                QFile::remove(fileToRemoveAtEnd);
            }
            return;
        }

        if (m_cancelled) {
            emit doneAddingAttachments(false, "Cancelled", QFileInfo(sourceFilePathToAttach).fileName());
            if (!fileToRemoveAtEnd.isEmpty() && QFile::exists(fileToRemoveAtEnd)) {
                QFile::remove(fileToRemoveAtEnd);
            }
            return;
        }

        status["Message"] = "Appending";
        emit addingAttachmentStatusChanged(status);

        QVariantMap attributes;
        attributes["MimeType"] = mimeType;
        if (isImage && imageDimensions.isValid()) {
            attributes["Dimensions"] = imageDimensions;
        }
        attributes["FileName"] = QFileInfo(sourceFilePathToAttach).fileName();
        QVariantMap returnMap;
        bool appended = appendAttachment(filePathToAttach, m_storageManager, m_noteId, currentEnmlContent, m_baseContentHash, attributes, &returnMap);
        QByteArray updatedEnml = returnMap.value("UpdatedEnml").toByteArray();
        if (!updatedEnml.isEmpty()) {
            currentEnmlContent = updatedEnml;
        }
        if (!fileToRemoveAtEnd.isEmpty() && QFile::exists(fileToRemoveAtEnd)) {
            QFile::remove(fileToRemoveAtEnd);
        }
        if (appended) {
            emit addedAttachment(returnMap);
        } else {
            emit doneAddingAttachments(false, "Cannot append", QFileInfo(sourceFilePathToAttach).fileName());
            return;
        }
    }
    emit doneAddingAttachments(true, "", "");
}

void AddAttachmentsThread::cancel()
{
    m_cancelled = true;
    if (m_imageResizeHelper) {
        m_imageResizeHelper->cancel();
    }
}

void AddAttachmentsThread::resetAddingAttachmentStatus()
{
    QVariantMap status;
    status["TotalFileCount"] = 0;
    status["CurrentFileIndex"] = 0;
    status["CurrentFilePath"] = QString("");
    status["Message"] = QString("");
    emit addingAttachmentStatusChanged(status);
}
