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

#include "storagemanager.h"
#include "crypto/crypto.h"
#include "cloud/evernote/evernotesync/evernotemarkup.h"
#include "storage/diskcache/shareddiskcache.h"
#include "logger.h"
#include "qplatformdefs.h"
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QStringBuilder>
#include <QSharedData>
#include <QDateTime>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QStringBuilder>
#include <QVariantMap>
#include <QMap>
#include <QMapIterator>
#include <QFile>
#include <QDesktopServices>

#define ID_PATH(id) pathFragmentFromObjectId(id)

// #define DEBUG

#ifdef DEBUG
#include <QDebug>
#endif

Logger* StorageManager::s_logger = 0;

void redirectMessageToLog(QtMsgType type, const char *msg)
{
    if (StorageManager::s_logger == 0) {
        return;
    }
    switch (type) {
    case QtDebugMsg:
        StorageManager::s_logger->log(QString("qDebug: %1").arg(msg));
        break;
    case QtWarningMsg:
        StorageManager::s_logger->log(QString("qWarning: %1").arg(msg));
        break;
    case QtCriticalMsg:
        StorageManager::s_logger->log(QString("qCritical: %1").arg(msg));
        break;
    case QtFatalMsg:
        StorageManager::s_logger->log(QString("qFatal: %1").arg(msg));
        abort();
    default:
        StorageManager::s_logger->log(QString("qUnknownMessage: %1").arg(msg));
    }
}

static inline QString pathFragmentFromObjectId(const QString &objectId)
{
    const int len = objectId.length();
    Q_ASSERT(len == 8);
    return (objectId.midRef(len - 4, 1) % objectId.rightRef(1) % QLatin1String("/") % objectId);
    // To limit the number of items per dir to improve performance in Symbian:
    // We first to try to keep <= 16 items per dir for the first 256 items and
    // after that, try to keep <= 256 items per dir
}


StorageManager::StorageManager(QObject *parent)
    : QObject(parent)
    , m_encryptedSettingsFormat(QSettings::registerFormat("dat", Crypto::readEncryptedSettings, Crypto::writeEncryptedSettings))
    , m_loggingEnabledStatus(LoggingEnabledStatusUnknown)
{
    m_iniFileCache.setMaxCost(2 << 20); // 2MB
    writeStorageVersion("1.0");
}

void StorageManager::writeStorageVersion(const QString &versionString)
{
    IniFile sessionIni = sessionDataIniFile("session.ini");
    sessionIni.setValue("StorageVersion", versionString);
}

QString StorageManager::notesDataLocation() const
{
    QString appExecName = "notekeeper-open";

#ifdef MEEGO_EDITION_HARMATTAN
    return (QDesktopServices::storageLocation(QDesktopServices::DataLocation) % "/" % appExecName);
#else
    return QDir::currentPath();
#endif
}

// Notes

static QString legalizedNoteTitle(const QString &_title)
{
    QString title = _title;
    if (title.contains('\n')) {
        int newLinePos = title.indexOf('\n');
        if (newLinePos >= 0) {
            title.truncate(newLinePos);
        }
    }
    Q_ASSERT(!title.contains('\n'));
    title = title.trimmed();
    title.truncate(255); // 255 is the max length of title supported by Evernote
    if (title.isEmpty()) {
        title = '-';
    }
    return title;
}

QString StorageManager::createNote(const QString &title, const QByteArray &content, const QString &sourceUrl)
{
    QVariantMap gistData;
    gistData[QString::fromLatin1("Title")] = legalizedNoteTitle(title);
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    gistData[QString::fromLatin1("CreatedTime")] = now;
    gistData[QString::fromLatin1("UpdatedTime")] = now;
#if defined(Q_OS_SYMBIAN) || defined(USE_SYMBIAN_COMPONENTS)
    gistData["Attributes/Source"] = "mobile.symbian";
#endif
#if defined(MEEGO_EDITION_HARMATTAN) || defined(USE_MEEGO_COMPONENTS)
    gistData["Attributes/Source"] = "mobile.nokia.n9";
#endif
    gistData["Attributes/SourceApplication"] = "Notekeeper";
    if (!sourceUrl.isEmpty()) {
        gistData["Attributes/SourceUrl"] = sourceUrl;
    }
    gistData["CreatedHere"] = true;
    QVariantMap contentData;
    contentData[QString::fromLatin1("Content")] = content;
    contentData[QString::fromLatin1("ContentHash")] = QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex();
    contentData[QString::fromLatin1("ContentValid")] = true;
    QString noteId = createStorageObject("Notes", "nt",
                                         "list.ini", "CurrentMaxLocalNoteIdNumber", "NoteIds",
                                         "gist.ini", gistData,
                                         "content.ini", contentData);
    emit noteCreated(noteId);
    setNotebookForNote(noteId, defaultNotebookId());

#ifdef DEBUG
    qDebug() << "Note " << noteId << " created";
#endif
    return noteId;
}

bool StorageManager::updateNoteTitle(const QString &noteId, const QString &_title)
{
    if (noteId.isEmpty()) {
        return false;
    }
    QString title = legalizedNoteTitle(_title);
    QVariantMap gistData;
    gistData[QString::fromLatin1("Title")] = title;
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    gistData[QString::fromLatin1("UpdatedTime")] = now;

    bool titleChanged = false;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        titleChanged = (noteGistIni.value("Title").toString() != title);
        if (titleChanged) {
            noteGistIni.setValues(gistData);
        }
    }
    if (titleChanged) {
        updateNotesListOrder(noteId);
        emit noteDisplayDataChanged(noteId, true);
    }
#ifdef DEBUG
    qDebug() << "Note " << noteId << " updated";
#endif

    return titleChanged;
}

bool StorageManager::updateNoteTitleAndContent(const QString &noteId, const QString &_title, const QByteArray &content,
                                               const QByteArray &baseContentHash /* value of contentBaseHash when the user opened this note */)
{
    if (noteId.isEmpty()) {
        return false;
    }
    IniFile noteContentEditLock = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/note_content_edit_lock");
    Q_UNUSED(noteContentEditLock);

    // update note gist
    bool titleChanged = false;
    QString title = legalizedNoteTitle(_title);
    QByteArray currentBaseContentHash; // md5sum of the content as last fetched, excluding any local edits
    QString currentTitle;
    QVariantMap gistData;
    gistData[QString::fromLatin1("UpdatedTime")] = QDateTime::currentDateTime().toMSecsSinceEpoch();
    gistData[QString::fromLatin1("BaseContentHash")] = baseContentHash;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentBaseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
        currentTitle = noteGistIni.value("Title").toString();
        titleChanged = (title != currentTitle);
        if (titleChanged) {
            gistData[QString::fromLatin1("Title")] = title;
        }
        noteGistIni.setValues(gistData);
    }

    // update note content
    bool contentChanged = false;
    QByteArray contentHash = QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex();
    QByteArray currentContentHash;
    bool currentContentValid;
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        currentContentHash = noteContentsIni.value("ContentHash").toByteArray();
        currentContentValid = noteContentsIni.value("ContentValid").toBool();
        contentChanged = (!currentContentValid || currentContentHash != contentHash);
        if (contentChanged) {
            QVariantMap contentData;
            contentData[QString::fromLatin1("Content")] = content;
            contentData[QString::fromLatin1("ContentHash")] = contentHash;
            contentData[QString::fromLatin1("ContentValid")] = true;
            noteContentsIni.setValues(contentData);
        }
    }

    if (!currentBaseContentHash.isEmpty() && currentBaseContentHash != baseContentHash) {
        // When the user opened the note for editing, the "BaseContentHash" of the note was baseContentHash.
        // While the user was editing it, sync must have updated the note, and changed the note's
        // "BaseContentHash" to currentBaseContentHash. So, when the user saves her edits, we see that
        // the "BaseContentHash" of the note is currentBaseContentHash.
        // => Conflict.
        // We record this conflict, but defer resolution of this conflict to the next sync.
        // We have also reset the "BaseContentHash" of this note to baseContentHash to help conflict detection later.
        addToEvernoteSyncIdsList("NoteIdsWithDeferredConflictResolution", noteId);
    }

    if (titleChanged || contentChanged) {
        updateNotesListOrder(noteId);
        emit noteDisplayDataChanged(noteId, true);
    }
#ifdef DEBUG
    qDebug() << "Note " << noteId << " updated";
#endif
    return (titleChanged || contentChanged);
}

static bool rmMinusR(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return true;
    foreach(const QFileInfo &info, dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
        if (info.isDir()) {
            if (!rmMinusR(info.filePath()))
                return false;
        } else {
            if (!dir.remove(info.fileName()))
                return false;
        }
    }
    QDir parentDir(QFileInfo(dirPath).path());
    return parentDir.rmdir(QFileInfo(dirPath).fileName());
}

QVariantList StorageManager::listNotes(StorageConstants::NotesListType whichNotes, const QString &objectId)
{
    if (whichNotes == StorageConstants::AllNotes) {
        return listNotes("Notes/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::NotesInNotebook) {
        if (objectId.isEmpty()) {
            return QVariantList();
        }
        return listNotes("Notebooks/" % ID_PATH(objectId) % "/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::FavouriteNotes) {
        return listNotes("SpecialNotebooks/Favourites/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::TrashNotes) {
        return listNotes("SpecialNotebooks/Trash/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::NotesWithTag) {
        if (objectId.isEmpty()) {
            return QVariantList();
        }
        return listNotes("Tags/" % ID_PATH(objectId) % "/list.ini", "NoteIds");
    }
    return QVariantList();
}

QStringList StorageManager::listNoteIds(StorageConstants::NotesListType whichNotes, const QString &objectId)
{
    if (whichNotes == StorageConstants::AllNotes) {
        return listNoteIds("Notes/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::NotesInNotebook) {
        if (objectId.isEmpty()) {
            return QStringList();
        }
        return listNoteIds("Notebooks/" % ID_PATH(objectId) % "/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::FavouriteNotes) {
        return listNoteIds("SpecialNotebooks/Favourites/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::TrashNotes) {
        return listNoteIds("SpecialNotebooks/Trash/list.ini", "NoteIds");
    } else if (whichNotes == StorageConstants::NotesWithTag) {
        if (objectId.isEmpty()) {
            return QStringList();
        }
        return listNoteIds("Tags/" % ID_PATH(objectId) % "/list.ini", "NoteIds");
    }
    return QStringList();
}

QVariantMap StorageManager::noteData(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return QVariantMap();
    }
    QString guid;
    QByteArray syncContentHash;
    QVariantMap data;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        data[QString::fromLatin1("Title")] = noteGistIni.value("Title").toString();
        data[QString::fromLatin1("Favourite")] = noteGistIni.value("Favourite").toBool();
        data[QString::fromLatin1("Trashed")] = noteGistIni.value("Trashed").toBool();

        QString notebookId = noteGistIni.value("NotebookId").toString();
        data[QString::fromLatin1("NotebookId")] = notebookId; // used in EvernoteAccess::synchronize()
        data[QString::fromLatin1("NotebookName")] = (notebookId.isEmpty()? "" : notebookName(notebookId));

        QString tagIdsStr = noteGistIni.value("TagIds").toString();
        QStringList tagIds = (tagIdsStr.isEmpty()? QStringList() : tagIdsStr.split(","));
        data[QString::fromLatin1("TagIds")] = tagIds; // used in EvernoteAccess::synchronize()
        QString tagNames;
        int tagCount = 0;
        foreach (const QString tagId, tagIds) {
            if (tagId.isEmpty())
                continue;
            QString name = tagName(tagId);
            if (name.isEmpty())
                continue;
            if (tagNames.isEmpty()) {
                tagNames = name;
            } else {
                tagNames += ", " + name;
            }
            tagCount++;
        }
        data[QString::fromLatin1("TagNames")] = tagNames;
        data[QString::fromLatin1("TagCount")] = tagCount;

        guid = noteGistIni.value("guid").toString();
        data[QString::fromLatin1("guid")] = guid;
        data[QString::fromLatin1("SyncUSN")] = noteGistIni.value("SyncUSN").toInt();
        syncContentHash = noteGistIni.value("SyncContentHash").toByteArray();
        data[QString::fromLatin1("SyncContentHash")] = syncContentHash;
        data[QString::fromLatin1("CreatedTime")] = QDateTime::fromMSecsSinceEpoch(noteGistIni.value("CreatedTime").toLongLong());
        data[QString::fromLatin1("UpdatedTime")] = QDateTime::fromMSecsSinceEpoch(noteGistIni.value("UpdatedTime").toLongLong());
        data[QString::fromLatin1("BaseContentUSN")] = noteGistIni.value("BaseContentUSN").toInt();
        data[QString::fromLatin1("BaseContentHash")] = noteGistIni.value("BaseContentHash").toByteArray();
        QStringList attributeKeys;
        attributeKeys << "SubjectDate" << "Latitude" << "Longitude" << "Author" << "Source" << "SourceUrl" << "SourceApplication" << "ShareDate" << "ContentClass";
        foreach (const QString &key, attributeKeys) {
            QString fullKey = "Attributes/" + key;
            data[fullKey] = noteGistIni.value(fullKey);
        }
    }
    {
        IniFile noteContentIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        bool contentValid = noteContentIni.value("ContentValid").toBool();
        if (contentValid) {
            // take content from content.ini
            data[QString::fromLatin1("ContentDataAvailable")] = true;
            data[QString::fromLatin1("Content")] = noteContentIni.value("Content").toByteArray();
            data[QString::fromLatin1("ContentHash")] = noteContentIni.value("ContentHash").toByteArray();
        } else {
            Q_ASSERT(!guid.isEmpty());
            QByteArray cachedContent, cachedContentHash;
            SharedDiskCache::instance()->retrieveNoteContent(guid, &cachedContent, &cachedContentHash);
            if (!cachedContentHash.isEmpty() && (cachedContentHash == syncContentHash)) {
                // take content from the disk cache
                data[QString::fromLatin1("ContentDataAvailable")] = true;
                data[QString::fromLatin1("Content")] = cachedContent;
                data[QString::fromLatin1("ContentHash")] = cachedContentHash;
            } else {
                // gotta fetch the content from the server
                data[QString::fromLatin1("ContentDataAvailable")] = false;
            }
        }
    }
    return data;
}

QByteArray StorageManager::enmlContentFromContentIni(const QString &noteId, bool *ok)
{
    if (noteId.isEmpty()) {
        return QByteArray();
    }
    bool contentValid;
    QByteArray content;
    {
        IniFile noteContentIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        contentValid = noteContentIni.value("ContentValid").toBool();
        content = noteContentIni.value("Content").toByteArray();
    }
    if (ok) {
        (*ok) = contentValid;
    }
    if (contentValid) {
        return content;
    }
    return QByteArray();
}

QString StorageManager::setSyncedNoteGist(const QString &guid, const QString &title, qint32 usn, const QByteArray &contentHash, qint64 createdTime, qint64 updatedTime,
                                          const QVariantMap &noteAttributes, bool *isUsnChanged)
{
    Q_ASSERT(!guid.isEmpty());
    QVariantMap data;
    data[QString::fromLatin1("Title")] = title;
    data[QString::fromLatin1("guid")] = guid;
    data[QString::fromLatin1("SyncUSN")] = usn;
    data[QString::fromLatin1("SyncContentHash")] = contentHash;
    data[QString::fromLatin1("CreatedTime")] = createdTime;
    data[QString::fromLatin1("UpdatedTime")] = updatedTime;
    QMapIterator<QString, QVariant> iter(noteAttributes);
    while (iter.hasNext()) {
        iter.next();
        data.insert(iter.key(), iter.value());
    }

    bool _isUsnChanged = false;
    QString noteId = noteIdForGuid(guid);
    bool noteTimestampChanged = false;
    if (noteId.isEmpty()) {
        // no note with this guid exists, create maadi
        noteId = createStorageObject("Notes", "nt",
                                     "list.ini", "CurrentMaxLocalNoteIdNumber", "NoteIds",
                                     "gist.ini", data);
        emit noteCreated(noteId);
        _isUsnChanged = true;
    } else {
        // there's already a note with this guid, let's just update that
        {
            IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
            qint64 existingUpdatedTime = noteGistIni.value("UpdatedTime").toLongLong();
            qint32 existingUsn = noteGistIni.value("SyncUSN").toInt();
            noteGistIni.setValues(data);
            noteTimestampChanged = (existingUpdatedTime != updatedTime);
            _isUsnChanged = (existingUsn != usn);
        }
        emit noteDisplayDataChanged(noteId, noteTimestampChanged);
    }
    setGuidMapping("Notes/byGuid.ini", guid, noteId);
    if (noteTimestampChanged) {
        updateNotesListOrder(noteId);
    }
#ifdef DEBUG
    qDebug() << "Note " << noteId << " gist updated from server";
#endif
    if (isUsnChanged) {
        (*isUsnChanged) = _isUsnChanged;
    }
    return noteId;
}

bool StorageManager::setFetchedNoteContent(const QString &noteGuid, const QString &title, const QByteArray &content, const QByteArray &contentHash, qint32 usn, qint64 updatedTime,
                                           const QVariantMap &noteAttributes, const QVariantList &attachments)
{
    Q_ASSERT(!noteGuid.isEmpty());
    Q_ASSERT(!contentHash.isEmpty());
    QString noteId = noteIdForGuid(noteGuid);
    if (noteId.isEmpty()) {
        noteId = setSyncedNoteGist(noteGuid, title, usn, contentHash, updatedTime, updatedTime, noteAttributes);
        qDebug() << "No note with guid [" + noteGuid + "] exists to set content. Created [" + noteId + "] on the fly.";
    }

    IniFile noteContentEditLock = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/note_content_edit_lock");
    Q_UNUSED(noteContentEditLock);

    // check for conflicts
    QByteArray resolvedContent;
    bool isConflict = false; // if true, should use resolvedContent
    QByteArray currentBaseContentHash; // md5sum of the content as last fetched, excluding any local edits
    qint64 lastUpdatedTime;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentBaseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
        lastUpdatedTime = noteGistIni.value("UpdatedTime").toLongLong();
    }
    if (!currentBaseContentHash.isEmpty()) { // if this note has been fetched earlier
        QByteArray currentContent;
        QByteArray currentContentHash; // md5sum of the content, including any local edits
        bool currentContentValid = false;
        {
            IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
            currentContentHash = noteContentsIni.value("ContentHash").toByteArray();
            currentContent = noteContentsIni.value("Content").toByteArray();
            currentContentValid = noteContentsIni.value("ContentValid").toBool();
        }
        if (currentBaseContentHash == contentHash) {
            // We are already up-to-date w.r.t. the fetched content
            // We just need to store the fetched content, if we don't have it already
            if (currentContentValid) {
                // We have valid content in content.ini, which is an edit on top of the fetched version
                return true;
            }
        } else {
            // currentBaseContentHash != contentHash
            // When we last synced, "BaseContentHash" was currentBaseContentHash
            // But now, the server's content hash has changed to contentHash
            // => Note content has changed in the server since we last synced
            if (currentContentValid && currentContentHash != currentBaseContentHash && currentContentHash != contentHash) {
                // Note has been edited by the user in this device since we last synced
                bool ok = false;
                resolvedContent = EvernoteMarkup::resolveConflict(content, currentContent, lastUpdatedTime, attachments, &ok);
                if (ok && (content != resolvedContent)) {
                    isConflict = true;
                }
            }
        }
    }

    // update note content
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        if (isConflict) {
            // if content has been changed by the user, save it in a file
            noteContentsIni.setValue("Content", resolvedContent);
            noteContentsIni.setValue("ContentHash", QCryptographicHash::hash(resolvedContent, QCryptographicHash::Md5).toHex());
            noteContentsIni.setValue("ContentValid", true);
        } else if (noteNeedsToBeAvailableOffline(noteId)) {
            // if content needs to be available offline, save it in a file
            noteContentsIni.setValue("Content", content);
            noteContentsIni.setValue("ContentHash", contentHash);
            noteContentsIni.setValue("ContentValid", true);
        } else {
            // else, save it in the disk cache (if it gets uncached, we can always re-fetch it)
            noteContentsIni.setValue("ContentValid", false);
            noteContentsIni.setValue("Content", QByteArray());
            noteContentsIni.setValue("ContentHash", QByteArray());
            SharedDiskCache::instance()->insertNoteContent(noteGuid, content, contentHash);
        }
    }

    // update note gist
    QVariantMap data;
    data[QString::fromLatin1("Title")] = title;
    data[QString::fromLatin1("BaseContentUSN")] = usn;
    data[QString::fromLatin1("BaseContentHash")] = contentHash;
    data[QString::fromLatin1("SyncContentHash")] = contentHash;
    data[QString::fromLatin1("UpdatedTime")] = updatedTime;
    bool titleChanged = false;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        titleChanged = (noteGistIni.value("Title") != title);
        noteGistIni.setValues(data);
    }
    emit noteDisplayDataChanged(noteId, (lastUpdatedTime != updatedTime));
#ifdef DEBUG
    qDebug() << "Note " << noteId << " content updated from server";
#endif
    return true;
}

bool StorageManager::setPushedNote(const QString &noteId, const QString &title, const QByteArray &content, const QByteArray &contentHash, qint32 usn, qint64 updatedTime)
{
    if (noteId.isEmpty()) {
        return false;
    }
    qint64 lastUpdatedTime = 0;
    {
        QVariantMap gistData;
        gistData[QString::fromLatin1("Title")] = title;
        gistData[QString::fromLatin1("BaseContentUSN")] = usn;
        gistData[QString::fromLatin1("BaseContentHash")] = contentHash;
        gistData[QString::fromLatin1("SyncContentHash")] = contentHash;
        gistData[QString::fromLatin1("UpdatedTime")] = updatedTime;
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        lastUpdatedTime = noteGistIni.value("UpdatedTime").toLongLong();
        noteGistIni.setValues(gistData);
    }
    QByteArray currentContentHash; // md5sum of the content, including any local edits
    bool currentContentValid = false;
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        currentContentHash = noteContentsIni.value("ContentHash").toByteArray();
        currentContentValid = noteContentsIni.value("ContentValid").toBool();
    }
    if (currentContentValid && currentContentHash == contentHash) {
        // We have content in content.ini which is the same as what's been pushed now
        bool needsTobeAvailableOffline = noteNeedsToBeAvailableOffline(noteId);
        if (!needsTobeAvailableOffline) {
            // remove from content.ini
            QVariantMap contentData;
            contentData[QString::fromLatin1("ContentValid")] = false;
            contentData[QString::fromLatin1("ContentHash")] = QByteArray();
            contentData[QString::fromLatin1("Content")] = QByteArray();
            {
                IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
                noteContentsIni.setValues(contentData);
            }
            // store in cache
            QString noteGuid = guidForNoteId(noteId);
            if (!noteGuid.isEmpty()) {
                SharedDiskCache::instance()->insertNoteContent(noteGuid, content, contentHash);
            }
        }
    }

    if (lastUpdatedTime != updatedTime) {
        emit noteDisplayDataChanged(noteId, true);
    }
    return true;
}

int StorageManager::noteInsertionPositionInNotesList(const QString &noteId, const QStringList &noteIdsList)
{
    Q_ASSERT(!noteId.isEmpty());
    qint64 noteTimestamp;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        qint64 updatedTime = noteGistIni.value("UpdatedTime").toLongLong();
        qint64 createdTime = noteGistIni.value("CreatedTime").toLongLong();
        if (updatedTime == 0) {
            noteTimestamp = createdTime;
        } else {
            noteTimestamp = updatedTime;
        }
    }
    int requiredInsertionPos = noteIdsList.length();
    for (int i = 0; i < noteIdsList.length(); i++) {
        const QString &nId = noteIdsList.at(i);
        Q_ASSERT(!nId.isEmpty());
        Q_ASSERT(nId != noteId);
        qint64 timestamp;
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(nId) % "/gist.ini");
        qint64 updatedTime = noteGistIni.value("UpdatedTime").toLongLong();
        qint64 createdTime = noteGistIni.value("CreatedTime").toLongLong();
        if (updatedTime == 0) {
            timestamp = createdTime;
        } else {
            timestamp = updatedTime;
        }
        if (noteTimestamp > timestamp) {
            requiredInsertionPos = i;
            break;
        }
    }
    return requiredInsertionPos;
}

void StorageManager::updateNotesListOrder(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return;
    }
    QStringList noteIdsList;
    {
        noteIdsList = listNoteIds("Notes/list.ini", "NoteIds");
    }

    int pos = noteIdsList.indexOf(noteId);
    if (pos < 0) {
        // noteId is not a note; nothing to do
        return;
    }

    noteIdsList.removeAt(pos);
    int insertPos = noteInsertionPositionInNotesList(noteId, noteIdsList);
    noteIdsList.insert(insertPos, noteId);
    {
        IniFile notesListIni = notesDataIniFile("Notes/list.ini");
        notesListIni.setValue("NoteIds", noteIdsList.join(","));
    }
}

bool StorageManager::isNoteContentUpToDate(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    QString guid = guidForNoteId(noteId);
    if (guid.isEmpty()) { // unpushed note
        return true;
    }
    qint32 syncUsn = 0;
    qint32 baseContentUsn = 0;
    QByteArray syncContentHash, baseContentHash;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        syncUsn = noteGistIni.value("SyncUSN").toInt();
        syncContentHash = noteGistIni.value("SyncContentHash").toByteArray();
        baseContentUsn = noteGistIni.value("BaseContentUsn").toInt();
        baseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
    }
    if (syncUsn == 0 || baseContentUsn == 0) {
        return false;
    }
    if (syncContentHash.isEmpty() || baseContentHash.isEmpty()) {
        return false;
    }
    if (syncContentHash != baseContentHash) {
        return false;
    }
    bool contentValid = false;
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        contentValid = noteContentsIni.value("ContentValid").toBool();
    }
    if (contentValid) {
        return true;
    }
    // if content is not present in content.ini, let's look in the cache
    bool cached = SharedDiskCache::instance()->containsNoteContent(guid, baseContentHash);
    return cached;
}

void StorageManager::setNoteGuid(const QString &noteId, const QString &guid)
{
    if (noteId.isEmpty()) {
        return;
    }
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        noteGistIni.setValue("guid", guid);
    }
    setGuidMapping("Notes/byGuid.ini", guid, noteId);
}

QString StorageManager::guidForNoteId(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return QString();
    }
    IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
    return noteGistIni.value("guid").toString();
}

QString StorageManager::noteIdForGuid(const QString &guid)
{
    if (guid.isEmpty()) {
        return QString();
    }
    return localIdForGenericGuid("Notes/byGuid.ini", guid);
}

static QString legalizedNotebookName(const QString &name)
{
    // name: A sequence of characters representing the name of the notebook.
    // May be changed by clients, but the account may not contain two notebooks
    // with names that are equal via a case-insensitive comparison. Can't begin
    // or end with a space.
    return name.simplified();
}

static QString legalizedTagName(const QString &name)
{

    // name: A sequence of characters representing the tag's identifier.
    // Case is preserved, but is ignored for comparisons. This means that an
    // account may only have one tag with a given name, via case-insensitive
    // comparison, so an account may not have both "food" and "Food" tags.
    // May not contain a comma (','), and may not begin or end with a space.
    QString str = name.simplified();
    str.replace(',', QLatin1String("-"));
    return str;
}

// Notebooks

QString StorageManager::createNotebook(const QString &_name, const QString &firstNoteId)
{
    if (_name.isEmpty()) {
        return QString();
    }

    // Make sure that we don't already have a notebook by that name
    QString name = legalizedNotebookName(_name);

    bool notebookAlreadyExists = false;
    QString notebookId;

    {
        IniFile notebookNameMap = notesDataIniFile("Notebooks/names.ini");
        notebookId = notebookNameMap.value(name.toUpper()).toString();
        notebookAlreadyExists = (!notebookId.isEmpty());
    }

    if (!notebookAlreadyExists) {
        // Create the notebook and add the note to the notebook
        QVariantMap data;
        data[QString::fromLatin1("Name")] = name;
        data[QString::fromLatin1("NoteIds")] = firstNoteId;
        notebookId = createStorageObject("Notebooks", "nb",
                                         "list.ini", "CurrentMaxLocalNotebookIdNumber", "NotebookIds",
                                         "list.ini", data);
        // Add the new notebook to the name map
        {
            IniFile notebookNameMap = notesDataIniFile("Notebooks/names.ini");
            notebookNameMap.setValue(name.toUpper(), notebookId);
        }
    }

    Q_ASSERT(!notebookId.isEmpty());

    if (notebookAlreadyExists && !firstNoteId.isEmpty()) {
        // Add the note to the notebook
        addNoteIdToNotebookData(firstNoteId, notebookId);
    }

    // Set the note's notebook as this notebook
    if (!firstNoteId.isEmpty()) {
        QString currentNotebookId;
        {
            IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(firstNoteId) % "/gist.ini");
            currentNotebookId = noteGistIni.value("NotebookId").toString();
            Q_ASSERT(currentNotebookId != notebookId);
            noteGistIni.setValue("NotebookId", notebookId);
        }
        if (!currentNotebookId.isEmpty()) {
            removeNoteIdFromNotebookData(firstNoteId, currentNotebookId);
        }
        emit notebookForNoteChanged(firstNoteId, notebookId);
    }

    if (notebookId == "nb000001") { // first notebook ever
        setDefaultNotebookId(notebookId); // to avoid a defaultnotebookless state
        // just being paranoid, also good for the simulator
    }

#ifdef DEBUG
    qDebug() << "Notebook" << notebookId << "created with note" << firstNoteId;
#endif
    emit notebooksListChanged();
    return notebookId;
}

QString StorageManager::setSyncedNotebookData(const QString &guid, const QString &name)
{
    Q_ASSERT(!guid.isEmpty());
    Q_ASSERT(!name.isEmpty());
    QString notebookId = notebookIdForGuid(guid);
    if (notebookId.isEmpty()) {
        notebookId = createNotebook(name);
    } else {
        setNotebookName(notebookId, name);
        emit notebooksListChanged();
    }
    setNotebookGuid(notebookId, guid);
    return notebookId;
}

bool StorageManager::setNotebookForNote(const QString &noteId, const QString &notebookId)
{
    if (notebookId.isEmpty() || noteId.isEmpty()) {
        return false;
    }
    QString currentNotebookId;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentNotebookId = noteGistIni.value("NotebookId").toString();
        if (currentNotebookId == notebookId) {
            return false;
        }
        noteGistIni.setValue("NotebookId", notebookId);
    }

    removeNoteIdFromNotebookData(noteId, currentNotebookId);
    addNoteIdToNotebookData(noteId, notebookId);

    emit notebookForNoteChanged(noteId, notebookId);
    return true;
}

QString StorageManager::notebookForNote(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return QString();
    }
    IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
    return noteGistIni.value("NotebookId").toString();
}

QStringList StorageManager::listNormalNotebookIds()
{
    QStringList notebookIds;
    {
        IniFile notebooksRootIni = notesDataIniFile("Notebooks/list.ini");
        QString notebookIdsStr = notebooksRootIni.value("NotebookIds").toString();
        if (!notebookIdsStr.isEmpty()) {
            notebookIds = notebookIdsStr.split(",");
        }
    }
    return notebookIds;
}

QVariantList StorageManager::listNotebooks(StorageConstants::NotebookTypes notebookTypes)
{
    QVariantList toReturn;

    // add favourites notebook
    if ((notebookTypes & StorageConstants::FavouritesNotebook) == StorageConstants::FavouritesNotebook) {
        QVariantMap notebookData;
        notebookData[QString::fromLatin1("Name")] = QString::fromLatin1("Favourites");
        notebookData[QString::fromLatin1("NotebookType")] = StorageConstants::FavouritesNotebook;
        toReturn.append(notebookData);
    }

    // add normal notebooks
    if ((notebookTypes & StorageConstants::NormalNotebook) == StorageConstants::NormalNotebook) {
        QMap<QString, QVariant> orderedNotebooks;
        QStringList notebookIds = listNormalNotebookIds();
        foreach (const QString &notebookId, notebookIds) {
            IniFile notebookGistIni = notesDataIniFile("Notebooks/" % ID_PATH(notebookId) % "/list.ini");
            QVariantMap notebookData;
            notebookData[QString::fromLatin1("NotebookId")] = notebookId;
            QString notebookName = notebookGistIni.value("Name").toString();
            notebookData[QString::fromLatin1("Name")] = notebookName;
            notebookData[QString::fromLatin1("NotebookType")] = StorageConstants::NormalNotebook;
            orderedNotebooks.insert(notebookName.toLower(), notebookData);
        }
        toReturn.append(orderedNotebooks.values());
    }

    // add trash notebook
    if ((notebookTypes & StorageConstants::TrashNotebook) == StorageConstants::TrashNotebook) {
        QVariantMap notebookData;
        notebookData[QString::fromLatin1("Name")] = QString::fromLatin1("Trash");
        notebookData[QString::fromLatin1("NotebookType")] = StorageConstants::TrashNotebook;
        toReturn.append(notebookData);
    }

    return toReturn;
}

bool StorageManager::notebookExists(const QString &notebookId)
{
    if (notebookId.isEmpty()) {
        return false;
    }
    IniFile notebooksRootIni = notesDataIniFile("Notebooks/list.ini");
    QString notebookIdsStr = notebooksRootIni.value("NotebookIds").toString();
    QStringList notebookIds;
    if (!notebookIdsStr.isEmpty()) {
        notebookIds = notebookIdsStr.split(",");
    }
    return notebookIds.contains(notebookId);
}

void StorageManager::setNotebookName(const QString &notebookId, const QString &name)
{
    IniFile notebookGistIni = notesDataIniFile("Notebooks/" % ID_PATH(notebookId) % "/list.ini");
    notebookGistIni.setValue("Name", name);
}

QString StorageManager::notebookName(const QString &notebookId)
{
    if (notebookId.isEmpty()) {
        return QString();
    }
    IniFile notebookGistIni = notesDataIniFile("Notebooks/" % ID_PATH(notebookId) % "/list.ini");
    return notebookGistIni.value("Name").toString();
}

QString StorageManager::notebookIdForNotebookName(const QString &_name)
{
    QString name = legalizedNotebookName(_name);
    if (name.isEmpty()) {
        return QString();
    }
    IniFile notebookNameMap = notesDataIniFile("Notebooks/names.ini");
    return notebookNameMap.value(name.toUpper()).toString();
}

bool StorageManager::renameNotebook(const QString &notebookId, const QString &_name)
{
    if (_name.isEmpty()) {
        return false;
    }
    if (!notebookExists(notebookId)) {
        return false;
    }

    QString currentNotebookName = notebookName(notebookId);
    if (currentNotebookName.isEmpty()) {
        return false;
    }

    // Make sure that we don't already have a notebook by that name
    QString name = legalizedNotebookName(_name);

    bool notebookAlreadyExists = false;
    {
        IniFile notebookNameMap = notesDataIniFile("Notebooks/names.ini");
        QString existingNotebookId = notebookNameMap.value(name.toUpper()).toString();
        notebookAlreadyExists = (!existingNotebookId.isEmpty());
    }

    if (notebookAlreadyExists) {
        return false;
    }

    // Rename notebook
    setNotebookName(notebookId, name);

    // Update our name map
    {
        IniFile notebookNameMap = notesDataIniFile("Notebooks/names.ini");
        notebookNameMap.removeKey(currentNotebookName.toUpper());
        notebookNameMap.setValue(name.toUpper(), notebookId);
    }

    emit notebooksListChanged();
    return true;
}

void StorageManager::setNotebookGuid(const QString &notebookId, const QString &guid)
{
    if (notebookId.isEmpty() || guid.isEmpty()) {
        return;
    }
    {
        IniFile notebookIni = notesDataIniFile("Notebooks/" % ID_PATH(notebookId) % "/list.ini");
        notebookIni.setValue("guid", guid);
    }
    setGuidMapping("Notebooks/byGuid.ini", guid, notebookId);
}

QString StorageManager::guidForNotebookId(const QString &notebookId)
{
    if (notebookId.isEmpty()) {
        return QString();
    }
    IniFile notebookIni = notesDataIniFile("Notebooks/" % ID_PATH(notebookId) % "/list.ini");
    return notebookIni.value("guid").toString();
}

QString StorageManager::notebookIdForGuid(const QString &guid)
{
    return localIdForGenericGuid("Notebooks/byGuid.ini", guid);
}

void StorageManager::setDefaultNotebookId(const QString &notebookId)
{
    if (!notebookId.isEmpty() && notebookExists(notebookId)) {
        IniFile notebooksRootIni = notesDataIniFile("Notebooks/list.ini");
        QString currentValue = notebooksRootIni.value("DefaultNotebookId").toString();
        if (currentValue != notebookId) {
            notebooksRootIni.setValue("DefaultNotebookId", notebookId);
        }
    }
}

QString StorageManager::defaultNotebookId()
{
    QString notebookId;
    {
        IniFile notebooksRootIni = notesDataIniFile("Notebooks/list.ini");
        notebookId = notebooksRootIni.value("DefaultNotebookId").toString();
    }
#ifndef QT_SIMULATOR
    Q_ASSERT(!notebookId.isEmpty());
    Q_ASSERT(notebookExists(notebookId));
#endif
    return notebookId;
}

QString StorageManager::defaultNotebookName()
{
    return notebookName(defaultNotebookId());
}

void StorageManager::setOfflineNotebookIds(const QStringList &notebookIds)
{
    QSet<QString> currentOfflineNotebooks = offlineNotebookIds().toSet();
    QStringList newlyMadeOfflineNotebooks;
    QString notebookIdsStr("");
    foreach (const QString &notebookId, notebookIds) {
        bool alreadyOffline = currentOfflineNotebooks.remove(notebookId);
        if (!notebookId.isEmpty() && notebookExists(notebookId)) {
            if (notebookIdsStr.isEmpty()) {
                notebookIdsStr = notebookId;
            } else {
                notebookIdsStr = notebookIdsStr % "," % notebookId;
            }
            if (!alreadyOffline) {
                newlyMadeOfflineNotebooks << notebookId;
            }
        }
    }
    foreach (const QString &unresolvedNotebookId, newlyMadeOfflineNotebooks) {
        setOfflineStatusChangeUnresolvedNotebook(unresolvedNotebookId, true);
    }
    foreach (const QString &unresolvedNotebookId, currentOfflineNotebooks) {
        setOfflineStatusChangeUnresolvedNotebook(unresolvedNotebookId, true);
    }
    {
        IniFile notebooksRootIni = notesDataIniFile("Notebooks/list.ini");
        notebooksRootIni.setValue("OfflineNotebookIds", notebookIdsStr);
    }
}

QStringList StorageManager::offlineNotebookIds()
{
    QString notebookIdsStr;
    {
        IniFile notebooksRootIni = notesDataIniFile("Notebooks/list.ini");
        notebookIdsStr = notebooksRootIni.value("OfflineNotebookIds").toString().trimmed();
    }
    if (notebookIdsStr.isEmpty()) {
        return QStringList();
    }
    QStringList notebookIds;
    foreach (const QString &notebookId, notebookIdsStr.split(',')) {
        if (!notebookId.isEmpty() && notebookExists(notebookId)) {
            notebookIds << notebookId;
        }
    }
    return notebookIds;
}

// Tags

QString StorageManager::createTag(const QString &_name, const QString &firstNoteId)
{
    if (_name.isEmpty()) {
        return QString();
    }
    // Make sure that we don't already have a tag by that name
    QString name = legalizedTagName(_name);

    bool tagAlreadyExists = false;
    QString tagId;

    {
        IniFile tagNameMap = notesDataIniFile("Tags/names.ini");
        tagId = tagNameMap.value(name.toUpper()).toString();
        tagAlreadyExists = (!tagId.isEmpty());
    }

    if (!tagAlreadyExists) {
        // Create the tag and add the note to the tag
        QVariantMap data;
        data[QString::fromLatin1("Name")] = name;
        data[QString::fromLatin1("NoteIds")] = firstNoteId;
        tagId = createStorageObject("Tags", "tg",
                                    "list.ini", "CurrentMaxLocalTagIdNumber", "TagIds",
                                    "list.ini", data);
        // Add the new tag to the name map
        {
            IniFile tagNameMap = notesDataIniFile("Tags/names.ini");
            tagNameMap.setValue(name.toUpper(), tagId);
        }
    }

    Q_ASSERT(!tagId.isEmpty());

    if (tagAlreadyExists && !firstNoteId.isEmpty()) {
        // Add the note to the tag data
        addNoteIdToTagData(firstNoteId, tagId);
    }

    // Add tag to the note
    if (!firstNoteId.isEmpty()) {
        bool added = addObjectIdToCollectionData("Notes/" % ID_PATH(firstNoteId) % "/gist.ini", "TagIds", tagId);
        Q_ASSERT(added);
        emit tagsForNoteChanged(firstNoteId, tagsOnNote(firstNoteId));
    }

#ifdef DEBUG
    qDebug() << "Tag" << tagId << "created with note" << firstNoteId;
#endif
    emit tagsListChanged();
    return tagId;
}

QString StorageManager::setSyncedTagData(const QString &guid, const QString &name)
{
    Q_ASSERT(!guid.isEmpty());
    Q_ASSERT(!name.isEmpty());
    QString tagId = tagIdForGuid(guid);
    if (tagId.isEmpty()) {
        tagId = createTag(name);
    } else {
        setTagName(tagId, name);
        emit tagsListChanged();
    }
    Q_ASSERT(!tagId.isEmpty());
    setTagGuid(tagId, guid);
    return tagId;
}

bool StorageManager::setTagsOnNote(const QString &noteId, const QString &tagIds)
{
    if (noteId.isEmpty()) {
        return false;
    }
    QStringList tagIdsList;
    if (!tagIds.isEmpty()) {
        tagIdsList = tagIds.split(',');
    }
    return setTagsOnNote(noteId, tagIdsList);
}

bool StorageManager::setTagsOnNote(const QString &noteId, const QStringList &tagIds)
{
#ifdef DEBUG
    qDebug() << "Setting tags" << tagIds << "on note" << noteId;
#endif
    if (noteId.isEmpty()) {
        return false;
    }
    QString currentTagIdsStr;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentTagIdsStr = noteGistIni.value("TagIds").toString();
        noteGistIni.setValue("TagIds", tagIds.join(","));
    }
    QSet<QString> currentTags;
    if (!currentTagIdsStr.isEmpty()) {
        currentTags = currentTagIdsStr.split(",").toSet();
    }
    if (tagIds.toSet() == currentTags) {
        return false;
    }
    if (!tagIds.isEmpty()) {
        foreach (const QString &t, tagIds) {
            bool isCurrentTag = currentTags.remove(t);
            if (!isCurrentTag) {
                addNoteIdToTagData(noteId, t);
            }
        }
    }
    foreach (const QString &t, currentTags) {
        removeNoteIdFromTagData(noteId, t);
    }
    emit tagsForNoteChanged(noteId, tagIds);
    return true;
}

void StorageManager::addTagToNote(const QString &noteId, const QString &tagId)
{
    if (noteId.isEmpty() || tagId.isEmpty()) {
        return;
    }
    QString currentTagIdsStr;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentTagIdsStr = noteGistIni.value("TagIds").toString();
    }
    QSet<QString> currentTags;
    if (!currentTagIdsStr.isEmpty()) {
        currentTags = currentTagIdsStr.split(",").toSet();
    }
    if (!currentTags.contains(tagId)) {
        if (currentTagIdsStr.isEmpty()) {
            currentTagIdsStr = tagId;
        } else {
            currentTagIdsStr = currentTagIdsStr % "," % tagId;
        }
        addNoteIdToTagData(noteId, tagId);
        {
            IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
            noteGistIni.setValue("TagIds", currentTagIdsStr);
        }
        emit tagsForNoteChanged(noteId, currentTagIdsStr.split(","));
    }
}

QVariantList StorageManager::listTagsWithTagsOnNoteChecked(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return listTags(); // don't have to make any tag 'checked'
    }
    QStringList allTagIds = idsList("Tags/list.ini", "TagIds");
    QStringList tagIdsOnNote = idsList("Notes/" % ID_PATH(noteId) % "/gist.ini", "TagIds");
    return listTagsChecked(allTagIds, tagIdsOnNote.toSet());
}

QVariantList StorageManager::listTags()
{
    return listTagsChecked(idsList("Tags/list.ini", "TagIds"));
}

bool StorageManager::tagExists(const QString &tagId)
{
    if (tagId.isEmpty()) {
        return false;
    }
    QStringList allTagIds = idsList("Tags/list.ini", "TagIds");
    return allTagIds.contains(tagId);
}

QStringList StorageManager::tagsOnNote(const QString &noteId)
{
    QStringList tagIdsOnNote = idsList("Notes/" % ID_PATH(noteId) % "/gist.ini", "TagIds");
    return tagIdsOnNote;
}

void StorageManager::setTagName(const QString &tagId, const QString &name)
{
    if (tagId.isEmpty() || name.isEmpty()) {
        return;
    }
    IniFile tagGistIni = notesDataIniFile("Tags/" % ID_PATH(tagId) % "/list.ini");
    tagGistIni.setValue("Name", name);
}

QString StorageManager::tagName(const QString &tagId)
{
    if (tagId.isEmpty()) {
        return QString();
    }
    IniFile tagGistIni = notesDataIniFile("Tags/" % ID_PATH(tagId) % "/list.ini");
    return tagGistIni.value("Name").toString();
}

bool StorageManager::renameTag(const QString &tagId, const QString &_name)
{
    if (_name.isEmpty()) {
        return false;
    }
    if (!tagExists(tagId)) {
        return false;
    }

    QString currentTagName = tagName(tagId);
    if (currentTagName.isEmpty()) {
        return false;
    }

    // Make sure that we don't already have a tag by that name
    QString name = legalizedTagName(_name);

    bool tagAlreadyExists = false;
    {
        IniFile tagNameMap = notesDataIniFile("Tags/names.ini");
        QString existingTagId = tagNameMap.value(name.toUpper()).toString();
        tagAlreadyExists = (!existingTagId.isEmpty());
    }

    if (tagAlreadyExists) {
        return false;
    }

    // Rename tag
    setTagName(tagId, name);

    // Update our name map
    {
        IniFile tagNameMap = notesDataIniFile("Tags/names.ini");
        tagNameMap.removeKey(currentTagName.toUpper());
        tagNameMap.setValue(name.toUpper(), tagId);
    }

    emit tagsListChanged();
    return true;
}

void StorageManager::setTagGuid(const QString &tagId, const QString &guid)
{
    if (tagId.isEmpty() || guid.isEmpty()) {
        return;
    }
    {
        IniFile tagIni = notesDataIniFile("Tags/" % ID_PATH(tagId) % "/list.ini");
        tagIni.setValue("guid", guid);
    }
    setGuidMapping("Tags/byGuid.ini", guid, tagId);
}

QString StorageManager::guidForTagId(const QString &tagId)
{
    Q_ASSERT(!tagId.isEmpty());
    if (tagId.isEmpty()) {
        return QString();
    }
    IniFile tagIni = notesDataIniFile("Tags/" % ID_PATH(tagId) % "/list.ini");
    return tagIni.value("guid").toString();
}

QString StorageManager::tagIdForGuid(const QString &guid)
{
    Q_ASSERT(!guid.isEmpty());
    if (guid.isEmpty()) {
        return QString();
    }
    return localIdForGenericGuid("Tags/byGuid.ini", guid);
}

// Favourites

bool StorageManager::setFavouriteNote(const QString &noteId, bool isFavourite)
{
    Q_ASSERT(!noteId.isEmpty());
    if (noteId.isEmpty()) {
        return false;
    }
    bool isCurrentlyFavourite = false;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        isCurrentlyFavourite = noteGistIni.value("Favourite").toBool();
    }
    if (isCurrentlyFavourite != isFavourite) {
        if (isFavourite) {
            bool added = addObjectIdToCollectionData("SpecialNotebooks/Favourites/list.ini", "NoteIds", noteId);
            Q_ASSERT(added);
        } else {
            bool removed = removeObjectIdFromCollectionData("SpecialNotebooks/Favourites/list.ini", "NoteIds", noteId);
            Q_ASSERT(removed);
        }
        {
            IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
            noteGistIni.setValue("Favourite", isFavourite);
        }
        emit noteFavouritenessChanged(noteId, isFavourite);
        return true;
    }
    return false;
}

bool StorageManager::isFavouriteNote(const QString &noteId)
{
    IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
    return noteGistIni.value("Favourite").toBool();
}

// Trash

bool StorageManager::moveNoteToTrash(const QString &noteId)
{
    Q_ASSERT(!noteId.isEmpty());
    if (noteId.isEmpty()) {
        return false;
    }
    bool isTrashed = false;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        isTrashed = noteGistIni.value("Trashed").toBool();
        if (!isTrashed) {
            noteGistIni.setValue("Trashed", true);
        }
    }
    if (!isTrashed) {
        StorageConstants::NotesListTypes notesListsToRemoveRefsFrom = (StorageConstants::AllNotes |
                                                                       StorageConstants::NotesInNotebook |
                                                                       StorageConstants::NotesWithTag |
                                                                       StorageConstants::FavouriteNotes);
        removeNoteReferences(noteId, notesListsToRemoveRefsFrom);
        addObjectIdToCollectionData("SpecialNotebooks/Trash/list.ini", "NoteIds", noteId);
        emit noteTrashednessChanged(noteId, true);
        return true;
    }
    return false;
}

bool StorageManager::restoreNoteFromTrash(const QString &noteId)
{
    Q_ASSERT(!noteId.isEmpty());
    if (noteId.isEmpty()) {
        return false;
    }
    bool isTrashed = true;
    QString notebookId;
    QString tagIdsStr;
    bool isFavourite = false;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        isTrashed = noteGistIni.value("Trashed").toBool();
        if (isTrashed) {
            notebookId = noteGistIni.value("NotebookId").toString();
            tagIdsStr = noteGistIni.value("TagIds").toString();
            isFavourite = noteGistIni.value("Favourite").toBool();
            noteGistIni.setValue("Trashed", false);
        }
    }
    if (isTrashed) {
        // add to notebook ini
        bool notebookDoesExist = notebookExists(notebookId);
        if (!notebookDoesExist) {
            // move to default notebook
            notebookId = defaultNotebookId();
            {
                IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
                noteGistIni.setValue("NotebookId", notebookId);
            }
        }
        Q_ASSERT(notebookExists(notebookId));
        // add to tag ini
        QStringList tagIds;
        if (!tagIdsStr.isEmpty()) {
            tagIds = tagIdsStr.split(",");
        }
        QString updatedTagIdsStr;
        foreach (const QString &tagId, tagIds) {
            if (tagExists(tagId)) {
                addNoteIdToTagData(noteId, tagId);
                updatedTagIdsStr = (updatedTagIdsStr.isEmpty()? tagId : (updatedTagIdsStr % "," % tagId));
            }
        }
        if (tagIdsStr != updatedTagIdsStr) {
            IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
            noteGistIni.setValue("TagIds", updatedTagIdsStr);
        }
        // add to all-notes ini
        addObjectIdToCollectionData("Notes/list.ini", "NoteIds", noteId);
        updateNotesListOrder(noteId);
        // add to favourites
        if (isFavourite) {
            addObjectIdToCollectionData("SpecialNotebooks/Favourites/list.ini", "NoteIds", noteId);
        }
        // remove from trash ini
        removeObjectIdFromCollectionData("SpecialNotebooks/Trash/list.ini", "NoteIds", noteId);
        // notify changes
        emit noteTrashednessChanged(noteId, false);
        return true;
    }
    return false;
}

bool StorageManager::isTrashNote(const QString &noteId)
{
    IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
    return noteGistIni.value("Trashed").toBool();
}

bool StorageManager::expungeNoteFromTrash(const QString &noteId)
{
    Q_ASSERT(!noteId.isEmpty());
    if (noteId.isEmpty()) {
        return false;
    }
    QString guid = guidForNoteId(noteId);
    if (!guid.isEmpty()) {
        return false; // Cannot expunge pushed notes
    }
    bool removed = removeObjectIdFromCollectionData("SpecialNotebooks/Trash/list.ini", "NoteIds", noteId);
    if (removed) {
        removeGuidMapping("Notes/byGuid.ini", guid);
        rmMinusR(notesDataFullPath() % "/Notes/" % ID_PATH(noteId));
#ifdef DEBUG
        qDebug() << "Note " << noteId << " expunged";
#endif
        emit noteExpunged(noteId);
        return true;
    }
    return false;
}

void StorageManager::expungeNote(const QString &noteId)
{
    Q_ASSERT(!noteId.isEmpty());
    if (noteId.isEmpty()) {
        return;
    }
    StorageConstants::NotesListTypes notesListsToRemoveRefsFrom = (StorageConstants::AllNotes |
                                                                   StorageConstants::NotesInNotebook |
                                                                   StorageConstants::NotesWithTag |
                                                                   StorageConstants::FavouriteNotes |
                                                                   StorageConstants::TrashNotes);
    removeNoteReferences(noteId, notesListsToRemoveRefsFrom);
    QString guid = guidForNoteId(noteId);
    if (!guid.isEmpty()) {
        removeGuidMapping("Notes/byGuid.ini", guid);
    }
    rmMinusR(notesDataFullPath() % "/Notes/" % ID_PATH(noteId));
    setNoteHasUnpushedChanges(noteId, false);
    emit noteExpunged(noteId);
}

void StorageManager::expungeNotebook(const QString &notebookId)
{
    Q_ASSERT(!notebookId.isEmpty());
    if (notebookId.isEmpty()) {
        return;
    }

    // update default notebook
    QString defaultNotebook = defaultNotebookId();
    if (notebookId == defaultNotebook) { // the current default notebook is getting expunged
        // pick another notebook to be default
        QStringList notebookIds = listNormalNotebookIds();
        Q_ASSERT(!notebookIds.isEmpty()); // we can't be expunging a notebook if we don't have any notebooks
        if (notebookIds.isEmpty()) {
            return;
        }
        QString arbitlyPickedNewDefaultNotebook;
        foreach (const QString &notebookId, notebookIds) {
            if (notebookId != defaultNotebook) {
                arbitlyPickedNewDefaultNotebook = notebookId;
            }
        }
        Q_ASSERT(!arbitlyPickedNewDefaultNotebook.isEmpty()); // we shouldn't be expunging the only notebook we have
        if (!arbitlyPickedNewDefaultNotebook.isEmpty()) {
            return;
        }
        setDefaultNotebookId(arbitlyPickedNewDefaultNotebook);
        defaultNotebook = arbitlyPickedNewDefaultNotebook;
    }

    Q_ASSERT(!defaultNotebook.isEmpty());
    Q_ASSERT(notebookExists(defaultNotebook));
    Q_ASSERT(defaultNotebook != notebookId);

    // move notes from this notebook to the default notebook
    QStringList notesInNotebook = listNoteIds("Notebooks/" % ID_PATH(notebookId) % "/list.ini", "NoteIds");
    foreach (const QString &noteId, notesInNotebook) {
        setNotebookForNote(noteId, defaultNotebook);
    }
    Q_ASSERT(listNoteIds("Notebooks/" % ID_PATH(notebookId) % "/list.ini", "NoteIds").isEmpty());

    // remove from notebook names map
    QString notebookName;
    {
        IniFile notebookGistIni = notesDataIniFile("Notebooks/" % ID_PATH(notebookId) % "/list.ini");
        notebookName = notebookGistIni.value("Name").toString();
    }
    if (!notebookName.isEmpty()) {
        IniFile notebookNameMap = notesDataIniFile("Notebooks/names.ini");
        notebookNameMap.removeKey(notebookName);
    }

    // remove from guid map
    QString guid = guidForNotebookId(notebookId);
    if (!guid.isEmpty()) {
        removeGuidMapping("Notebooks/byGuid.ini", guid);
    }

    // remove from notebooks list
    removeObjectIdFromCollectionData("Notebooks/list.ini", "NotebookIds", notebookId);

    // remove notebook directory
    rmMinusR(notesDataFullPath() % "/Notebooks/" % ID_PATH(notebookId));
    emit notebooksListChanged();
}

void StorageManager::expungeTag(const QString &tagId)
{
    Q_ASSERT(!tagId.isEmpty());
    if (tagId.isEmpty()) {
        return;
    }

    // remove reference to this tag from the ini of the notes with this tag
    foreach (const QString &noteId, listNoteIds("Tags/" % ID_PATH(tagId) % "/list.ini", "NoteIds")) {
        removeObjectIdFromCollectionData("Notes/" % ID_PATH(noteId) % "/gist.ini", "TagIds", tagId);
    }

    // remove from tag names map
    QString tagName;
    {
        IniFile tagGistIni = notesDataIniFile("Tags/" % ID_PATH(tagId) % "/list.ini");
        tagName = tagGistIni.value("Name").toString();
    }
    if (!tagName.isEmpty()) {
        IniFile tagNameMap = notesDataIniFile("Tags/names.ini");
        tagNameMap.removeKey(tagName);
    }

    // remove from guid map
    QString guid = guidForTagId(tagId);
    if (!guid.isEmpty()) {
        removeGuidMapping("Tags/byGuid.ini", guid);
    }

    // remove from tags list
    removeObjectIdFromCollectionData("Tags/list.ini", "TagIds", tagId);

    // remove tag directory
    rmMinusR(notesDataFullPath() % "/Tags/" % ID_PATH(tagId));

    emit tagsListChanged();
}

bool StorageManager::setNoteThumbnail(const QString &noteId, const QImage &image, const QByteArray &sourceImageHash)
{
    if (noteId.isEmpty()) {
        return false;
    }
    if (image.isNull() || (image.width() <= 0) || (image.height() <= 0)) {
        bool changed = false;
        {
            IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
            QString existingThumbnailPath = noteGistIni.value("ThumbnailPath").toString();
            if (!existingThumbnailPath.isEmpty()) {
                QFile file(notesDataLocation() % "/" % existingThumbnailPath);
                if (file.exists()) {
                    file.remove();
                }
                changed = true;
            }
            noteGistIni.setValue("ThumbnailPath", QString());
            noteGistIni.setValue("ThumbnailSourceImageHash", QByteArray());
        }
        if (changed) {
            emit noteDisplayDataChanged(noteId, false);
        }
        return true;
    }

    // We try to change the thumbnail filename whenever we change the thumbnail
    // To prevent NotesList.qml from showing the same image
    Q_ASSERT(image.height() > 0);
    QByteArray middleScanlineMd5Hash = QCryptographicHash::hash(QByteArray(reinterpret_cast<const char*>(image.constScanLine(image.height() / 2)), image.bytesPerLine()), QCryptographicHash::Md5);
    QByteArray thumbnailHash = middleScanlineMd5Hash.toHex().left(8);
    QLatin1String imageHashSuffix(thumbnailHash.constData());
    QString imageFilename(notesDataRelativePath() % "/Notes/" % ID_PATH(noteId) % "/note_thumbnail_" % imageHashSuffix % ".jpg");

    bool ok = image.save(notesDataLocation() % "/" % imageFilename, "JPG");

    if (ok) {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        QString existingThumbnailPath = noteGistIni.value("ThumbnailPath").toString();
        if (!existingThumbnailPath.isEmpty() && existingThumbnailPath != imageFilename) {
            QFile file(notesDataLocation() % "/" % existingThumbnailPath);
            if (file.exists()) {
                file.remove();
            }
        }
        noteGistIni.setValue("ThumbnailPath", imageFilename);
        noteGistIni.setValue("ThumbnailWidth", image.width());
        noteGistIni.setValue("ThumbnailHeight", image.height());
        if (noteGistIni.value("ThumbnailSourceImageHash").toByteArray().isEmpty() && sourceImageHash.isEmpty()) {
            // don't introduce the key unnecessarily
        } else {
            noteGistIni.setValue("ThumbnailSourceImageHash", sourceImageHash);
        }
    }
    emit noteDisplayDataChanged(noteId, false);
    return ok;
}

bool StorageManager::noteThumbnailExists(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
    QString thumbnailPath = noteGistIni.value("ThumbnailPath").toString();
    return (!thumbnailPath.isEmpty() && QFile::exists(notesDataLocation() % "/" % thumbnailPath));
}

QByteArray StorageManager::noteThumbnailSourceHash(const QString &noteId)
{
    IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
    return noteGistIni.value("ThumbnailSourceImageHash").toByteArray();
}

// Settings / Preferences

void StorageManager::saveSetting(const QString &key, bool value)
{
    {
        IniFile settingsIni = preferencesIniFile("settings.ini");
        settingsIni.setValue(key, value);
    }
    if (key == "Logging/enabled") {
        if (value == true) {
            m_loggingEnabledStatus = LoggingEnabled;
            log("Logging enabled");
            qInstallMsgHandler(redirectMessageToLog);
        } else {
            m_loggingEnabledStatus = LoggingEnabled; // enable to note down the disabled-ness
            log("Logging disabled");
            m_loggingEnabledStatus = LoggingDisabled;
            qInstallMsgHandler(0);
        }
    }
}

bool StorageManager::retrieveSetting(const QString &key)
{
    IniFile settingsIni = preferencesIniFile("settings.ini");
    return settingsIni.value(key).toBool();
}

void StorageManager::saveStringSetting(const QString &key, const QString &value)
{
    IniFile settingsIni = preferencesIniFile("settings.ini");
    settingsIni.setValue(key, value);
}

QString StorageManager::retrieveStringSetting(const QString &key)
{
    IniFile settingsIni = preferencesIniFile("settings.ini");
    return settingsIni.value(key).toString();
}


void StorageManager::setNoteHasUnpushedChanges(const QString &noteId, bool hasUnpushedChanges)
{
    if (noteId.isEmpty()) {
        return;
    }
    if (hasUnpushedChanges) {
        addObjectIdToCollectionData("list.ini", "NotesToPush", noteId);
    } else {
        removeObjectIdFromCollectionData("list.ini", "NotesToPush", noteId);
    }
}

QStringList StorageManager::notesWithUnpushedChanges()
{
    return idsList("list.ini", "NotesToPush");
}

bool StorageManager::noteHasUnpushedChanges(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    return notesWithUnpushedChanges().contains(noteId);
}

bool StorageManager::noteHasUnpushedContentChanges(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    if (noteHasUnpushedChanges(noteId)) {
        QVariantMap noteDataMap = noteData(noteId);
        bool contentAvailableLocally = noteDataMap.value("ContentDataAvailable").toBool();
        if (!contentAvailableLocally) {
            return false;
        }
        QByteArray contentHash = noteDataMap.value("ContentHash").toByteArray();
        if (contentHash.isEmpty()) {
            return false;
        }
        QByteArray baseContentHash = noteDataMap.value("BaseContentHash").toByteArray();
        if (baseContentHash.isEmpty()) {
            return true;
        }
        return (contentHash != baseContentHash);
    }
    return false;
}

void StorageManager::setNotebookHasUnpushedChanges(const QString &notebookId, bool hasUnpushedChanges)
{
    if (notebookId.isEmpty()) {
        return;
    }
    if (hasUnpushedChanges) {
        addObjectIdToCollectionData("list.ini", "NotebooksToPush", notebookId);
    } else {
        removeObjectIdFromCollectionData("list.ini", "NotebooksToPush", notebookId);
    }
}

QStringList StorageManager::notebooksWithUnpushedChanges()
{
    return idsList("list.ini", "NotebooksToPush");
}

void StorageManager::setTagHasUnpushedChanges(const QString &tagId, bool hasUnpushedChanges)
{
    if (tagId.isEmpty()) {
        return;
    }
    if (hasUnpushedChanges) {
        addObjectIdToCollectionData("list.ini", "TagsToPush", tagId);
    } else {
        removeObjectIdFromCollectionData("list.ini", "TagsToPush", tagId);
    }
}

QStringList StorageManager::tagsWithUnpushedChanges()
{
    return idsList("list.ini", "TagsToPush");
}


void StorageManager::setAttachmentsDataFromServer(const QString &noteId, const QVariantList &attachments, bool isAfterPush, int *_removedImagesCount)
{
    if (noteId.isEmpty()) {
        return;
    }
    IniFile noteAttachmentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/attachments.ini");
    bool isMarkedOffline = noteNeedsToBeAvailableOffline(noteId);

    // read existing attachments to memory
    QHash<QByteArray /*md5sum*/, QVariantMap /*old attachments.ini data*/> oldAttachments;
    int currentCount = noteAttachmentsIni.beginReadArray("Attachments");
    if (currentCount == 0 && attachments.count() == 0) {
        return;
    }
    for (int i = 0; i < currentCount; i++) {
        noteAttachmentsIni.setArrayIndex(i);
        QByteArray md5Hash = noteAttachmentsIni.value("Hash").toByteArray();
        QVariantMap map;
        map["guid"] = noteAttachmentsIni.value("guid").toString();
        map["Hash"] = md5Hash;
        map["MimeType"] = noteAttachmentsIni.value("MimeType").toString();
        map["Dimensions"] = noteAttachmentsIni.value("Dimensions").toSize();
        map["Duration"] = noteAttachmentsIni.value("Duration").toInt();
        map["Size"] = noteAttachmentsIni.value("Size").toInt();
        map["FileName"] = noteAttachmentsIni.value("FileName").toString();
        oldAttachments.insert(md5Hash, map);
    }
    noteAttachmentsIni.endArray();

    QStringList unpushedRemovedGuids;
    if (isAfterPush) {
        noteAttachmentsIni.setValue("UnpushedRemovals/guids", QString());
    } else {
        QString unpushedRemovedGuidsStr = noteAttachmentsIni.value("UnpushedRemovals/guids").toString();
        unpushedRemovedGuids = unpushedRemovedGuidsStr.split(',');
    }

    // set new attachments data
    noteAttachmentsIni.beginWriteArray("Attachments");
    int i = 0;
    for (i = 0; i < attachments.size(); i++) {
        noteAttachmentsIni.setArrayIndex(i);
        const QVariantMap &map = attachments.at(i).toMap();
        if (!map.isEmpty()) {
            QString attachmentGuid = map.value("guid").toString();
            QByteArray md5Hash = map.value("Hash").toByteArray();
            qint32 attachmentSize = map.value("Size").toInt();

            if (!isAfterPush  && !oldAttachments.contains(md5Hash) &&
                !attachmentGuid.isEmpty() && unpushedRemovedGuids.contains(attachmentGuid)) {
                // This attachment was removed in Notekeeper; We're just seeing it during the pull before the push
                // Shouldn't be added
                continue;
            }

            // Write to attachments.ini
            noteAttachmentsIni.setValue("guid", attachmentGuid);
            noteAttachmentsIni.setValue("Hash", md5Hash);
            noteAttachmentsIni.setValue("Size", attachmentSize);
            noteAttachmentsIni.setValue("MimeType", map.value("MimeType").toString());
            noteAttachmentsIni.setValue("Dimensions", map.value("Dimensions").toSize());
            noteAttachmentsIni.setValue("Duration", map.value("Duration").toInt());
            noteAttachmentsIni.setValue("FileName", map.value("FileName").toString());

            // Get existing attachment fileName
            QString attachmentFullLocalPath = notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % md5Hash;
            if (!QFile::exists(attachmentFullLocalPath)) {
                attachmentFullLocalPath = notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % attachmentGuid; // v1.3 and earlier
                if (!QFile::exists(attachmentFullLocalPath)) {
                    attachmentFullLocalPath = "";
                }
            }

            // Remove attachment file if applicable
            if (!attachmentGuid.isEmpty() && !isMarkedOffline && !attachmentFullLocalPath.isEmpty()) {
                // attachment was pushed to the server, can remove local copy now
                QString webApiUrlPrefix = retrieveEvernoteAuthData("webApiUrlPrefix");
                QFile attachmentLocalFile(attachmentFullLocalPath);
                if (!webApiUrlPrefix.isEmpty()) {
                    SharedDiskCache::instance()->insertEvernoteNoteAttachment(webApiUrlPrefix, attachmentGuid, &attachmentLocalFile, static_cast<qint64>(attachmentSize));
                }
                attachmentLocalFile.remove();
            }

            // Remove from the QHash
            oldAttachments.remove(md5Hash);
        }
    }

    // figure out what to do with the attachments not found in the attachment list from Evernote
    QHashIterator<QByteArray, QVariantMap> unusedAttachmentsIter(oldAttachments);
    int removedImagesCount = 0;
    while (unusedAttachmentsIter.hasNext()) {
        unusedAttachmentsIter.next();
        const QVariantMap &map = unusedAttachmentsIter.value();
        QString attachmentGuid = map.value("guid").toString();
        QByteArray md5Hash = map.value("Hash").toByteArray();
        QString mimeType = map.value("MimeType").toString();

        // Get existing attachment fileName
        QString attachmentFullLocalPath = notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % md5Hash;
        if (!QFile::exists(attachmentFullLocalPath)) {
            attachmentFullLocalPath = notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % attachmentGuid;
            if (attachmentGuid.isEmpty() || !QFile::exists(attachmentFullLocalPath)) {
                attachmentFullLocalPath = "";
            }
        }

        // Make sure files under Attachments/ dir are in accordance with attachments.ini
        if (!attachmentFullLocalPath.isEmpty()) {
            // if the attachment exists as a file in Attachments/
            if (attachmentGuid.isEmpty()) {
                // attachment is not pushed yet; this should remain in attachments.ini
                noteAttachmentsIni.setArrayIndex(i);
                noteAttachmentsIni.setValue("guid", map.value("guid").toString());
                noteAttachmentsIni.setValue("Hash", md5Hash);
                noteAttachmentsIni.setValue("MimeType", mimeType);
                noteAttachmentsIni.setValue("Dimensions", map.value("Dimensions").toSize());
                noteAttachmentsIni.setValue("Duration", map.value("Duration").toInt());
                noteAttachmentsIni.setValue("Size", map.value("Size").toInt());
                noteAttachmentsIni.setValue("FileName", map.value("FileName").toString());
                i++;
            } else {
                // attachment was probably removed in the server; we don't need the file anymore
                QFile::remove(attachmentFullLocalPath);
                if (mimeType.startsWith("image/")) {
                    removedImagesCount++;
                }
            }
        }
    }

    noteAttachmentsIni.endArray();

    if (_removedImagesCount) {
        (*_removedImagesCount) = removedImagesCount;
    }
}

void StorageManager::addAttachmentData(const QString &noteId, const QVariantMap &attachmentData)
{
    // appends attachmentData to attachments.ini
    // if data pertaining to attachmentData already exists, overwrites that data instead of appending

    QByteArray incomingAttachmentMd5Hash = attachmentData.value("Hash").toByteArray();
    if (incomingAttachmentMd5Hash.isEmpty()) {
        return;
    }

    IniFile noteAttachmentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/attachments.ini");
    bool added = false;

    // read existing attachments to memory
    QList<QVariantMap> existingAttachments;
    int attachmentsCount = noteAttachmentsIni.beginReadArray("Attachments");
    for (int i = 0; i < attachmentsCount; i++) {
        noteAttachmentsIni.setArrayIndex(i);
        QByteArray md5Hash = noteAttachmentsIni.value("Hash").toByteArray();
        QVariantMap map;
        if (md5Hash == incomingAttachmentMd5Hash) {
            map = attachmentData; // overwrite with incoming data
            added = true;
        } else {
            map["guid"] = noteAttachmentsIni.value("guid").toString();
            map["Hash"] = md5Hash;
            map["MimeType"] = noteAttachmentsIni.value("MimeType").toString();
            map["Dimensions"] = noteAttachmentsIni.value("Dimensions").toSize();
            map["Duration"] = noteAttachmentsIni.value("Duration").toInt();
            map["Size"] = noteAttachmentsIni.value("Size").toInt();
            map["FileName"] = noteAttachmentsIni.value("FileName").toString();
        }
        existingAttachments << map;
    }
    noteAttachmentsIni.endArray();

    if (!added) {
        existingAttachments << attachmentData;
    }

    // write to attachments.ini
    noteAttachmentsIni.beginWriteArray("Attachments", existingAttachments.count());
    for (int i = 0; i < existingAttachments.count(); i++) {
        const QVariantMap &map = existingAttachments[i];
        noteAttachmentsIni.setArrayIndex(i);
        noteAttachmentsIni.setValue("guid", map.value("guid").toString());
        noteAttachmentsIni.setValue("Hash", map.value("Hash").toByteArray());
        noteAttachmentsIni.setValue("MimeType", map.value("MimeType").toString());
        noteAttachmentsIni.setValue("Dimensions", map.value("Dimensions").toSize());
        noteAttachmentsIni.setValue("Duration", map.value("Duration").toInt());
        noteAttachmentsIni.setValue("Size", map.value("Size").toInt());
        noteAttachmentsIni.setValue("FileName", map.value("FileName").toString());
    }
    noteAttachmentsIni.endArray();
}

bool StorageManager::removeAttachmentData(const QString &noteId, const QByteArray &md5HashToRemove)
{
    // removes data pertaining to an attachment with hash md5HashToRemove from attachments.ini

    if (md5HashToRemove.isEmpty()) {
        return false;
    }

    IniFile noteAttachmentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/attachments.ini");
    bool removed = false;
    QStringList removedGuids;

    // read existing attachments to memory
    QList<QVariantMap> existingAttachments;
    int attachmentsCount = noteAttachmentsIni.beginReadArray("Attachments");
    for (int i = 0; i < attachmentsCount; i++) {
        noteAttachmentsIni.setArrayIndex(i);
        QByteArray md5Hash = noteAttachmentsIni.value("Hash").toByteArray();
        QString guid = noteAttachmentsIni.value("guid").toString();
        if (md5Hash == md5HashToRemove) {
            removed = true;
            if (!guid.isEmpty()) {
                removedGuids << guid;
            }
        } else {
            QVariantMap map;
            map["guid"] = guid;
            map["Hash"] = md5Hash;
            map["MimeType"] = noteAttachmentsIni.value("MimeType").toString();
            map["Dimensions"] = noteAttachmentsIni.value("Dimensions").toSize();
            map["Duration"] = noteAttachmentsIni.value("Duration").toInt();
            map["Size"] = noteAttachmentsIni.value("Size").toInt();
            map["FileName"] = noteAttachmentsIni.value("FileName").toString();
            existingAttachments << map;
        }
    }
    noteAttachmentsIni.endArray();

    // write to attachments.ini
    if (removed) {
        noteAttachmentsIni.beginWriteArray("Attachments", existingAttachments.count());
        for (int i = 0; i < existingAttachments.count(); i++) {
            const QVariantMap &map = existingAttachments[i];
            noteAttachmentsIni.setArrayIndex(i);
            noteAttachmentsIni.setValue("guid", map.value("guid").toString());
            noteAttachmentsIni.setValue("Hash", map.value("Hash").toByteArray());
            noteAttachmentsIni.setValue("MimeType", map.value("MimeType").toString());
            noteAttachmentsIni.setValue("Dimensions", map.value("Dimensions").toSize());
            noteAttachmentsIni.setValue("Duration", map.value("Duration").toInt());
            noteAttachmentsIni.setValue("Size", map.value("Size").toInt());
            noteAttachmentsIni.setValue("FileName", map.value("FileName").toString());
        }
        noteAttachmentsIni.endArray();

        if (!removedGuids.isEmpty()) {
            QString unpushedRemovalGuids = noteAttachmentsIni.value("UnpushedRemovals/guids").toString();
            if (unpushedRemovalGuids.isEmpty()) {
                unpushedRemovalGuids = removedGuids.join(",");
            } else {
                unpushedRemovalGuids += ("," + removedGuids.join(","));
            }
            noteAttachmentsIni.setValue("UnpushedRemovals/guids", unpushedRemovalGuids);
        }
    }

    return removed;
}

QVariantList StorageManager::attachmentsData(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return QVariantList();
    }
    QVariantList returnAttachmentsList;
    IniFile noteAttachmentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/attachments.ini");
    int count = noteAttachmentsIni.beginReadArray("Attachments");
    for (int i = 0; i < count; i++) {
        noteAttachmentsIni.setArrayIndex(i);
        QVariantMap map;
        QString attachmentGuid = noteAttachmentsIni.value("guid").toString();
        map["guid"] = attachmentGuid;
        QByteArray md5Hash = noteAttachmentsIni.value("Hash").toByteArray();
        map["Hash"] = md5Hash;
        map["MimeType"] = noteAttachmentsIni.value("MimeType").toString();
        map["Dimensions"] = noteAttachmentsIni.value("Dimensions").toSize();
        map["Duration"] = noteAttachmentsIni.value("Duration").toInt();
        qint32 attachmentSize = noteAttachmentsIni.value("Size").toInt();
        map["Size"] = attachmentSize;
        map["FileName"] = noteAttachmentsIni.value("FileName").toString();
        QFileInfo fileInfo(notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % QLatin1String(md5Hash.constData()));
        if (fileInfo.exists() && (fileInfo.size() == attachmentSize)) {
            map["FilePath"] = fileInfo.absoluteFilePath();
        } else if (!attachmentGuid.isEmpty()) {
            // In v1.3 and earlier, we used attachmentGuid as the filename
            QFileInfo fileInfo(notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % attachmentGuid);
            if (fileInfo.exists() && (fileInfo.size() == attachmentSize)) {
                map["FilePath"] = fileInfo.absoluteFilePath();
            }
        }
        returnAttachmentsList << map;
    }
    noteAttachmentsIni.endArray();
    return returnAttachmentsList;
}

qint64 StorageManager::noteAttachmentsTotalSize(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return 0;
    }
    qint64 totalSize = 0;
    IniFile noteAttachmentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/attachments.ini");
    int count = noteAttachmentsIni.beginReadArray("Attachments");
    for (int i = 0; i < count; i++) {
        noteAttachmentsIni.setArrayIndex(i);
        qint32 attachmentSize = noteAttachmentsIni.value("Size").toInt();
        totalSize += static_cast<qint64>(attachmentSize);
    }
    noteAttachmentsIni.endArray();
    return totalSize;
}

bool StorageManager::saveCheckboxStates(const QString &noteId, const QVariantList &checkboxStates,  const QByteArray &enmlContent,const QByteArray &baseContentHash)
{
    if (noteId.isEmpty()) {
        return false;
    }
    IniFile noteContentEditLock = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/note_content_edit_lock");
    Q_UNUSED(noteContentEditLock);

    QByteArray newContent;
    bool updated = EvernoteMarkup::updateCheckboxStatesInEnml(enmlContent, checkboxStates, &newContent);
    if (!updated) {
        return false;
    }

    bool contentChanged = false;
    QByteArray newContentHash = QCryptographicHash::hash(newContent, QCryptographicHash::Md5).toHex();
    QByteArray currentContentHash;
    bool currentContentValid;
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        currentContentHash = noteContentsIni.value("ContentHash").toByteArray();
        currentContentValid = noteContentsIni.value("ContentValid").toBool();
        contentChanged = (!currentContentValid || currentContentHash != newContentHash);
        if (contentChanged) {
            QVariantMap contentData;
            contentData[QString::fromLatin1("Content")] = newContent;
            contentData[QString::fromLatin1("ContentHash")] = newContentHash;
            contentData[QString::fromLatin1("ContentValid")] = true;
            noteContentsIni.setValues(contentData);
        }
    }

    QByteArray currentBaseContentHash; // md5sum of the content as last fetched, excluding any local edits
    if (contentChanged) {
        qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentBaseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
        noteGistIni.setValue("BaseContentHash", baseContentHash);
        noteGistIni.setValue("UpdatedTime", now);
    }

    if (!currentBaseContentHash.isEmpty() && currentBaseContentHash != baseContentHash) {
        // When the user opened the note for editing, the "BaseContentHash" of the note was baseContentHash.
        // While the user was editing it, sync must have updated the note, and changed the note's
        // "BaseContentHash" to currentBaseContentHash. So, when the user saves her edits, we see that
        // the "BaseContentHash" of the note is currentBaseContentHash.
        // => Conflict.
        // We record this conflict, but defer resolution of this conflict to the next sync.
        // We have also reset the "BaseContentHash" of this note to baseContentHash to help conflict detection later.
        addToEvernoteSyncIdsList("NoteIdsWithDeferredConflictResolution", noteId);
    }
    if (contentChanged) {
        updateNotesListOrder(noteId);
        emit noteDisplayDataChanged(noteId, true);
    }
    return true;
}

bool StorageManager::appendTextToNote(const QString &noteId, const QString &text, const QByteArray &enmlContent, const QByteArray &baseContentHash, QByteArray *updatedEnml, QByteArray *htmlToAdd)
{
    if (noteId.isEmpty()) {
        return false;
    }
    if (text.isEmpty()) {
        return false;
    }
    IniFile noteContentEditLock = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/note_content_edit_lock");
    Q_UNUSED(noteContentEditLock);

    QByteArray newContent;
    bool updated = EvernoteMarkup::appendTextToEnml(enmlContent, text, &newContent, htmlToAdd);
    if (!updated) {
        return false;
    }

    bool contentChanged = false;
    QByteArray newContentHash = QCryptographicHash::hash(newContent, QCryptographicHash::Md5).toHex();
    QByteArray currentContentHash;
    bool currentContentValid;
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        currentContentHash = noteContentsIni.value("ContentHash").toByteArray();
        currentContentValid = noteContentsIni.value("ContentValid").toBool();
        contentChanged = (!currentContentValid || currentContentHash != newContentHash);
        if (contentChanged) {
            QVariantMap contentData;
            contentData[QString::fromLatin1("Content")] = newContent;
            contentData[QString::fromLatin1("ContentHash")] = newContentHash;
            contentData[QString::fromLatin1("ContentValid")] = true;
            noteContentsIni.setValues(contentData);
            if (updatedEnml) {
                (*updatedEnml) = newContent;
            }
        }
    }

    QByteArray currentBaseContentHash; // md5sum of the content as last fetched, excluding any local edits
    if (contentChanged) {
        qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentBaseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
        noteGistIni.setValue("BaseContentHash", baseContentHash);
        noteGistIni.setValue("UpdatedTime", now);
    }

    if (!currentBaseContentHash.isEmpty() && currentBaseContentHash != baseContentHash) {
        // When the user opened the note for editing, the "BaseContentHash" of the note was baseContentHash.
        // While the user was viewing it, sync must have updated the note, and changed the note's
        // "BaseContentHash" to currentBaseContentHash. So, when the user appends text, we see that
        // the "BaseContentHash" of the note is currentBaseContentHash.
        // => Conflict.
        // We record this conflict, but defer resolution of this conflict to the next sync.
        // We have also reset the "BaseContentHash" of this note to baseContentHash to help conflict detection later.
        addToEvernoteSyncIdsList("NoteIdsWithDeferredConflictResolution", noteId);
    }
    if (contentChanged) {
        updateNotesListOrder(noteId);
        emit noteDisplayDataChanged(noteId, true);
    }
    return true;
}

bool StorageManager::appendAttachmentToNote(const QString &noteId, const QString &filePath, const QVariantMap &attributes, const QByteArray &enmlContent, const QByteArray &baseContentHash, QByteArray *updatedEnml, QByteArray *htmlToAdd, QByteArray *hash, QString *absoluteFilePath)
{
    if (noteId.isEmpty()) {
        return false;
    }

    IniFile noteContentEditLock = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/note_content_edit_lock");
    Q_UNUSED(noteContentEditLock);

    // Create a copy and compute md5sum

    QFile file(filePath);
    QTemporaryFile tempFile("notekeeper_tmp");
    if (!file.open(QFile::ReadOnly) || !tempFile.open()) {
        return false;
    }
    QCryptographicHash hasher(QCryptographicHash::Md5);
    qint64 fileSize = file.size();
    qint64 totalBytesWritten = 0;
    while (!file.atEnd()) {
        QByteArray data = file.read(1 << 18); // max 256 kb
        totalBytesWritten += tempFile.write(data);
        hasher.addData(data);
    }
    if (totalBytesWritten != fileSize) {
        return false;
    }
    tempFile.close();
    file.close();
    QByteArray md5Hash = hasher.result().toHex();
    if (hash) {
        (*hash) = md5Hash;
    }

    // Move the copy to the appropriate location and write info to attachments.ini

    QString attachmentsDirPath = notesDataRelativePath() % "/Notes/" % ID_PATH(noteId) % "/Attachments";
    QString targetFilePath = attachmentsDirPath % "/" % QLatin1String(md5Hash.constData());
    QString targetFileAbsolutePath = notesDataLocation() % "/" % targetFilePath;
    if (absoluteFilePath) {
        (*absoluteFilePath) = targetFileAbsolutePath;
    }

    bool attachmentAlreadyExists = false;
    if (QFile::exists(targetFileAbsolutePath)) {
        QFile alreadyExistingFile(targetFileAbsolutePath);
        if (alreadyExistingFile.size() == fileSize) {
            attachmentAlreadyExists = true;
        } else {
            alreadyExistingFile.remove();
            attachmentAlreadyExists = false;
        }
    }

    QVariantMap attachmentPropertiesMap = attributes;
    attachmentPropertiesMap["guid"] = QString("");
    attachmentPropertiesMap["Hash"] = md5Hash;
    attachmentPropertiesMap["Size"] = fileSize;
    if (attachmentPropertiesMap.value("FileName").toString().isEmpty()) {
        attachmentPropertiesMap["FileName"] = QFileInfo(file.fileName()).fileName();
    }

    if (!attachmentAlreadyExists) {
        QDir(notesDataLocation()).mkpath(attachmentsDirPath);
        bool ok = tempFile.rename(targetFileAbsolutePath);
        if (!ok) {
            return false;
        }
        tempFile.setAutoRemove(false);
    }
    addAttachmentData(noteId, attachmentPropertiesMap); // add to attachments.ini

    // Get the updated enml

    QByteArray newContent;
    bool updated = EvernoteMarkup::appendAttachmentToEnml(enmlContent, "file:///" % targetFileAbsolutePath, attachmentPropertiesMap, &newContent, htmlToAdd);
    if (!updated) {
        return false;
    }

    // Update note content

    bool contentChanged = false;
    QByteArray newContentHash = QCryptographicHash::hash(newContent, QCryptographicHash::Md5).toHex();
    QByteArray currentContentHash;
    bool currentContentValid;
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        currentContentHash = noteContentsIni.value("ContentHash").toByteArray();
        currentContentValid = noteContentsIni.value("ContentValid").toBool();
        contentChanged = (!currentContentValid || currentContentHash != newContentHash);
        if (contentChanged) {
            QVariantMap contentData;
            contentData[QString::fromLatin1("Content")] = newContent;
            contentData[QString::fromLatin1("ContentHash")] = newContentHash;
            contentData[QString::fromLatin1("ContentValid")] = true;
            noteContentsIni.setValues(contentData);
            if (updatedEnml) {
                (*updatedEnml) = newContent;
            }
        }
    }

    QByteArray currentBaseContentHash; // md5sum of the content as last fetched, excluding any local edits
    if (contentChanged) {
        qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        currentBaseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
        noteGistIni.setValue("BaseContentHash", baseContentHash);
        noteGistIni.setValue("UpdatedTime", now);
    }

    if (!currentBaseContentHash.isEmpty() && currentBaseContentHash != baseContentHash) {
        // When the user opened the note for editing, the "BaseContentHash" of the note was baseContentHash.
        // While the user was viewing it, sync must have updated the note, and changed the note's
        // "BaseContentHash" to currentBaseContentHash. So, when the user adds an attachment, we see that
        // the "BaseContentHash" of the note is currentBaseContentHash.
        // => Conflict.
        // We record this conflict, but defer resolution of this conflict to the next sync.
        // We have also reset the "BaseContentHash" of this note to baseContentHash to help conflict detection later.
        addToEvernoteSyncIdsList("NoteIdsWithDeferredConflictResolution", noteId);
    }
    if (contentChanged) {
        updateNotesListOrder(noteId);
        emit noteDisplayDataChanged(noteId, true);
    }
    return true;
}

bool StorageManager::removeAttachmentFile(const QString &noteId, const QVariantMap &attachmentData)
{
    if (noteId.isEmpty()) {
        return false;
    }
    QByteArray md5Hash = attachmentData.value("Hash").toByteArray();
    if (md5Hash.isEmpty()) {
        return false;
    }
    QString attachmentGuid = attachmentData.value("guid").toString();
    QString attachmentFullLocalPath = notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % md5Hash;
    if (!QFile::exists(attachmentFullLocalPath)) {
        attachmentFullLocalPath = notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % attachmentGuid;
        if (attachmentGuid.isEmpty() || !QFile::exists(attachmentFullLocalPath)) {
            attachmentFullLocalPath = "";
        }
    }
    if (!attachmentFullLocalPath.isEmpty()) {
        QFile file(attachmentFullLocalPath);
        if (file.exists()) {
            bool removed = file.remove();
            return removed;
        }
    }
    return false;
}

bool StorageManager::setUnremovedTempFile(const QString &tempFilePath, bool unremoved)
{
    if (tempFilePath.isEmpty()) {
        return false;
    }
    IniFile tempIni = notesDataIniFile("temp.ini");
    QString tempFilesListStr = tempIni.value("UnremovedTempFiles").toString();
    QStringList tempFilesList;
    if (!tempFilesListStr.isEmpty()) {
        tempFilesList = tempFilesListStr.split(',');
    }
    if (unremoved) { // add to list
        if (!tempFilesList.contains(tempFilePath)) {
            tempFilesList.append(tempFilePath);
            tempIni.setValue("UnremovedTempFiles", tempFilesList.join(","));
            return true;
        }
    } else {         // remove from list
        int removedCount = tempFilesList.removeAll(tempFilePath);
        if (removedCount > 0) {
            tempIni.setValue("UnremovedTempFiles", tempFilesList.join(","));
            return true;
        }
    }
    return false;
}

QStringList StorageManager::unremovedTempFiles()
{
    IniFile tempIni = notesDataIniFile("temp.ini");
    QString tempFilesListStr = tempIni.value("UnremovedTempFiles").toString();
    QStringList tempFilesList;
    if (!tempFilesListStr.isEmpty()) {
        tempFilesList = tempFilesListStr.split(',');
    }
    return tempFilesList;
}

// Offline notes

bool StorageManager::noteNeedsToBeAvailableOffline(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
    bool noteCreatedHere = noteGistIni.value("CreatedHere").toBool();
    if (noteCreatedHere) {
        return true;
    }
    bool isFavourite = noteGistIni.value("Favourite").toBool();
    if (isFavourite) {
        return true;
    }
    QString notebookId = noteGistIni.value("NotebookId").toString();
    if (offlineNotebookIds().contains(notebookId)) {
        return true;
    }
    return false;
}

bool StorageManager::noteContentAvailableOffline(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    QByteArray syncContentHash, baseContentHash;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        syncContentHash = noteGistIni.value("SyncContentHash").toByteArray();
        baseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
    }
    if (baseContentHash.isEmpty() || (syncContentHash != baseContentHash)) {
        return false;
    }
    bool contentValid = false;
    {
        IniFile noteContentIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        contentValid = noteContentIni.value("ContentValid").toBool();
    }
    return contentValid;
}

bool StorageManager::noteAttachmentsAvailableOffline(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }

    foreach (const QVariant &map, attachmentsData(noteId)) {
        QVariantMap attachment = map.toMap();
        QString absoluteFilePath = attachment.value("FilePath").toString();
        if (absoluteFilePath.isEmpty() || !QFileInfo(absoluteFilePath).exists()) {
            return false;
        }
    }
    return true;
}

void StorageManager::setOfflineStatusChangeUnresolvedNote(const QString &noteId, bool changed)
{
    if (noteId.isEmpty()) {
        return;
    }
    if (changed) {
        addObjectIdToCollectionData("list.ini", "OfflineStatusChangeUnresolvedNotes", noteId);
    } else {
        removeObjectIdFromCollectionData("list.ini", "OfflineStatusChangeUnresolvedNotes", noteId);
    }
}

QStringList StorageManager::offlineStatusChangeUnresolvedNotes()
{
    return idsList("list.ini", "OfflineStatusChangeUnresolvedNotes");
}

bool StorageManager::tryMakeNoteContentAvailableOfflineWithCachedData(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
    bool contentValidInContentFile = noteContentsIni.value("ContentValid").toBool();
    if (contentValidInContentFile) { // content is already available offline
        return true;
    }
    QByteArray content, contentHash;
    QString noteGuid = guidForNoteId(noteId);
    if (noteGuid.isEmpty()) {
        return false;
    }
    bool retrieved = SharedDiskCache::instance()->retrieveNoteContent(noteGuid, &content, &contentHash);
    if (retrieved) {
        noteContentsIni.setValue("Content", content);
        noteContentsIni.setValue("ContentHash", contentHash);
        noteContentsIni.setValue("ContentValid", true);
        return true;
    }
    return false;
}

void StorageManager::makeNoteContentUnavailableOfflineIfUnedited(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return;
    }
    QByteArray baseContentHash;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        baseContentHash = noteGistIni.value("BaseContentHash").toByteArray();
    }
    if (baseContentHash.isEmpty()) {
        return;
    }
    {
        IniFile noteContentsIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        bool contentValidInContentFile = noteContentsIni.value("ContentValid").toBool();
        QByteArray contentHash = noteContentsIni.value("ContentHash").toByteArray();
        if (!contentValidInContentFile || contentHash.isEmpty()) { // already unavailable
            return;
        }
        if (baseContentHash != contentHash) { // user has edited it
            return;
        }
        QString noteGuid = guidForNoteId(noteId);
        if (noteGuid.isEmpty()) {
            return;
        }
        bool alreadyInCache = SharedDiskCache::instance()->containsNoteContent(noteGuid, contentHash);
        if (!alreadyInCache) {
            QByteArray content = noteContentsIni.value("Content").toByteArray();
            SharedDiskCache::instance()->insertNoteContent(noteGuid, content, contentHash);
        }
        // remove from content ini file
        noteContentsIni.setValue("ContentValid", false);
        noteContentsIni.setValue("Content", QByteArray());
        noteContentsIni.setValue("ContentHash", QByteArray());
    }
}

bool StorageManager::tryMakeNoteAttachmentsAvailableOfflineWithCachedData(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return false;
    }
    QString webApiUrlPrefix = retrieveEvernoteAuthData("webApiUrlPrefix");
    if (webApiUrlPrefix.isEmpty()) {
        return false;
    }
    bool allAttachmentsMadeAvailable = true;
    foreach (const QVariant &map, attachmentsData(noteId)) {
        QVariantMap attachment = map.toMap();
        QString attachmentGuid = attachment.value("guid").toString();
        int attachmentSize = attachment.value("Size").toInt();
        QString offlineAttachmentPath(notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % attachmentGuid);
        QFileInfo fileInfo(offlineAttachmentPath);
        if (!fileInfo.exists() || (fileInfo.size() != attachmentSize)) {
            // doesn't exist offline, so let's try to get it from the cache
            QDir(notesDataFullPath()).mkpath("Notes/" % ID_PATH(noteId) % "/Attachments/"); // make sure the dir exists
            QFile file(offlineAttachmentPath);
            bool retrievedFromCache = SharedDiskCache::instance()->retrieveEvernoteNoteAttachment(webApiUrlPrefix, attachmentGuid, &file, attachmentSize);
            if (!retrievedFromCache && file.exists()) {
                file.remove();
                SharedDiskCache::instance()->removeEvernoteNoteAttachment(webApiUrlPrefix, attachmentGuid);
            }
            if (!retrievedFromCache) {
                allAttachmentsMadeAvailable = false;
            }
        }
    }
    return allAttachmentsMadeAvailable;
}

void StorageManager::makeNoteAttachmentsUnavailableOffline(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return;
    }
    QString webApiUrlPrefix = retrieveEvernoteAuthData("webApiUrlPrefix");
    if (webApiUrlPrefix.isEmpty()) {
        return;
    }
    foreach (const QVariant &map, attachmentsData(noteId)) {
        QVariantMap attachment = map.toMap();
        QString attachmentGuid = attachment.value("guid").toString();
        int attachmentSize = attachment.value("Size").toInt();
        QString offlineAttachmentPath(notesDataFullPath() % "/Notes/" % ID_PATH(noteId) % "/Attachments/" % attachmentGuid);
        QFileInfo fileInfo(offlineAttachmentPath);
        if (fileInfo.exists()) {
            QFile file(offlineAttachmentPath);
            if (fileInfo.size() == attachmentSize) {
                SharedDiskCache::instance()->insertEvernoteNoteAttachment(webApiUrlPrefix, attachmentGuid, &file, attachmentSize);
            }
            file.remove();
        }
    }
}

void StorageManager::tryResolveOfflineStatusChange(const QString &noteId, bool *_requiredOffline, bool *_madeOffline)
{
    if (noteId.isEmpty()) {
        if (_requiredOffline) {
            (*_requiredOffline) = false;
        }
        if (_madeOffline) {
            (*_madeOffline) = false;
        }
        return;
    }
    bool requiredOffline = noteNeedsToBeAvailableOffline(noteId);
    bool madeOffline = false;
    if (requiredOffline) {
        bool contentOk = tryMakeNoteContentAvailableOfflineWithCachedData(noteId);
        bool attachmentsOk = tryMakeNoteAttachmentsAvailableOfflineWithCachedData(noteId);
        if (contentOk && attachmentsOk) {
            if (noteContentAvailableOffline(noteId) &&
                noteAttachmentsAvailableOffline(noteId)) {
                setOfflineStatusChangeUnresolvedNote(noteId, false);
                madeOffline = true;
            }
        }
    } else {
        makeNoteContentUnavailableOfflineIfUnedited(noteId);
        makeNoteAttachmentsUnavailableOffline(noteId);
        setOfflineStatusChangeUnresolvedNote(noteId, false);
    }
    if (_requiredOffline) {
        (*_requiredOffline) = requiredOffline;
    }
    if (_madeOffline) {
        (*_madeOffline) = madeOffline;
    }
}

void StorageManager::tryResolveOfflineStatusChanges()
{
    // Attempts to resolve the change in offline status using cached data (no network access)
    foreach (const QString &noteId, offlineStatusChangeUnresolvedNotes()) {
        tryResolveOfflineStatusChange(noteId);
        // Process user events to prevent ui lag when invoked
        // in the gui thread by QmlDataAccess::tryResolveOfflineStatusChanges()
        qApp->processEvents(QEventLoop::ExcludeSocketNotifiers);
    }
}

bool StorageManager::setOfflineNoteAttachmentContent(const QString &noteId, const QByteArray &attachmentHash, QTemporaryFile *attachmentFile)
{
    if (noteId.isEmpty() || attachmentHash.isEmpty()) {
        return false;
    }
    if (!noteNeedsToBeAvailableOffline(noteId)) {
        return false;
    }
    if (attachmentFile->fileName().isEmpty()) {
        return false;
    }
    QString attachmentDirPath(notesDataRelativePath() % "/Notes/" % ID_PATH(noteId) % "/Attachments");
    QDir(notesDataLocation()).mkpath(attachmentDirPath);
    QString fileFullName = notesDataLocation() % "/" % attachmentDirPath % "/" % QLatin1String(attachmentHash.constData());
    if (QFile::exists(fileFullName)) {
        QFile::remove(fileFullName);
    }
    bool renamed = attachmentFile->rename(fileFullName);
    if (!renamed) {
        return false;
    }
    attachmentFile->setAutoRemove(false);
    return true;
}

void StorageManager::setOfflineStatusChangeUnresolvedNotebook(const QString &notebookId, bool changed)
{
    if (notebookId.isEmpty()) {
        return;
    }
    if (changed) {
        addObjectIdToCollectionData("list.ini", "OfflineStatusChangeUnresolvedNotebooks", notebookId);
    } else {
        removeObjectIdFromCollectionData("list.ini", "OfflineStatusChangeUnresolvedNotebooks", notebookId);
    }
}

QStringList StorageManager::offlineStatusChangeUnresolvedNotebooks()
{
    return idsList("list.ini", "OfflineStatusChangeUnresolvedNotebooks");
}

// For storing Evernote auth credentials

void StorageManager::saveEvernoteAuthData(const QString &key, const QString &value)
{
    IniFile authIni = evernoteDataIniFile("auth.ini");
    authIni.setValue(key, value);
}

QString StorageManager::retrieveEvernoteAuthData(const QString &key)
{
    IniFile authIni = evernoteDataIniFile("auth.ini");
    return authIni.value(key).toString();
}

void StorageManager::saveEncryptedEvernoteAuthData(const QString &key, const QString &value)
{
    IniFile authIni = evernoteDataIniFile("auth.dat");
    authIni.setValue(key, value);
}

QString StorageManager::retrieveEncryptedEvernoteAuthData(const QString &key)
{
    IniFile authIni = evernoteDataIniFile("auth.dat");
    return authIni.value(key).toString();
}

bool StorageManager::isAuthDataValid()
{
    QString username, authToken, noteStoreUrl, webApiUrlPrefix;
    {
        IniFile authIni = evernoteDataIniFile("auth.ini");
        username = authIni.value("username").toString();
        noteStoreUrl = authIni.value("noteStoreUrl").toString();
        webApiUrlPrefix = authIni.value("webApiUrlPrefix").toString();
    }
    {
        IniFile authIni = evernoteDataIniFile("auth.dat");
        authToken = authIni.value("authToken").toString();
    }
    bool invalid = (username.isEmpty() || authToken.isEmpty() || noteStoreUrl.isEmpty() || webApiUrlPrefix.isEmpty());
    return (!invalid);
}

void StorageManager::purgeEvernoteAuthData()
{
    removeSettingsFile(evernoteDataIniFilePath("auth.ini"));
    removeSettingsFile(evernoteDataIniFilePath("auth.dat"));
}

// Bookkeeping information used during a sync
// The USN needs to be on disk to perform the next sync.
// Some other stuff is stored to disk in order to handle the case where
// the app exits while a sync is in progress.

void StorageManager::saveEvernoteSyncData(const QString &key, const QVariant &value)
{
    IniFile syncIni = evernoteDataIniFile("sync.ini");
    syncIni.setValue(key, value);
}

QVariant StorageManager::retrieveEvernoteSyncData(const QString &key)
{
    IniFile syncIni = evernoteDataIniFile("sync.ini");
    return syncIni.value(key);
}

QVariant StorageManager::retrieveEvernoteSyncDataForUser(const QString &username, const QString &key)
{
    IniFile syncIni = evernoteDataIniFileForUser(username, "sync.ini");
    return syncIni.value(key);
}

bool StorageManager::addToEvernoteSyncIdsList(const QString &key, const QString &id)
{
    IniFile syncIni = evernoteDataIniFile("sync.ini");
    QString idsListStr = syncIni.value(key).toString();
    QStringList idsList;
    if (!idsListStr.isEmpty()) {
        idsList = idsListStr.split(',');
    }
    if (!idsList.contains(id)) {
        idsList.append(id);
        syncIni.setValue(key, idsList.join(","));
        return true;
    }
    return false;
}

QStringList StorageManager::retrieveEvernoteSyncIdsList(const QString &key)
{
    IniFile syncIni = evernoteDataIniFile("sync.ini");
    QString idsListStr = syncIni.value(key).toString();
    QStringList idsList;
    if (!idsListStr.isEmpty()) {
        idsList = idsListStr.split(',');
    }
    return idsList;
}

void StorageManager::clearEvernoteSyncIdsList(const QString &key)
{
    IniFile syncIni = evernoteDataIniFile("sync.ini");
    syncIni.removeKey(key);
}

// User management

void StorageManager::setActiveUser(const QString &username)
{
    {
        IniFile currentUserIni = sessionDataIniFile("session.ini");
        currentUserIni.setValue("Username", username);
        if (username.isEmpty()) {
            currentUserIni.setValue("UserDirName", "");
        } else {
            QByteArray userHash = QCryptographicHash::hash(username.toLower().toLatin1(), QCryptographicHash::Sha1).toHex().left(8);
            m_activeUserDirName = QString("ev" % userHash);
            currentUserIni.setValue("UserDirName", m_activeUserDirName);
        }
    }
}

QString StorageManager::activeUser()
{
    IniFile currentUserIni = sessionDataIniFile("session.ini");
    return currentUserIni.value("Username").toString();
}

QString StorageManager::activeUserDirName()
{
    if (m_activeUserDirName.isEmpty()) {
        IniFile currentUserIni = sessionDataIniFile("session.ini");
        m_activeUserDirName = currentUserIni.value("UserDirName").toString();
    }
    return m_activeUserDirName;
}

void StorageManager::purgeActiveUserData()
{
    QString userDirName = activeUserDirName();
    if (!userDirName.isEmpty()) {
        if (QFile::exists(notesDataLocation() % "/Store/Data/" % userDirName)) {
            rmMinusR(notesDataLocation() % "/Store/Data/" % userDirName);
        }
        setActiveUser("");
    }
}

void StorageManager::setReadyForCreateNoteRequests(bool isReady)
{
    IniFile sessionIni = sessionDataIniFile("session.ini");
    sessionIni.setValue("ReadyForCreateNoteRequests", isReady);
}

// Logging

void StorageManager::log(const QString &message)
{
    if (m_loggingEnabledStatus == LoggingEnabledStatusUnknown) {
        bool enabled = retrieveSetting("Logging/enabled");
        m_loggingEnabledStatus = (enabled? LoggingEnabled : LoggingDisabled);
        if (m_loggingEnabledStatus == LoggingEnabled) {
            qInstallMsgHandler(redirectMessageToLog);
        } else {
            qInstallMsgHandler(0);
        }
    }
    Q_ASSERT(m_loggingEnabledStatus == LoggingEnabled || m_loggingEnabledStatus == LoggingDisabled);
    if (m_loggingEnabledStatus == LoggingEnabled) {
        if (s_logger == 0) {
            s_logger = new Logger(this);
            connect(s_logger, SIGNAL(textAddedToLog(QString)), SIGNAL(textAddedToLog(QString)));
            connect(s_logger, SIGNAL(cleared()), SIGNAL(logCleared()));
        }
        s_logger->log(message);
    }
}

QString StorageManager::loggedText()
{
    if (s_logger == 0) {
        s_logger = new Logger(this);
        connect(s_logger, SIGNAL(textAddedToLog(QString)), SIGNAL(textAddedToLog(QString)));
        connect(s_logger, SIGNAL(cleared()), SIGNAL(logCleared()));
    }
    return s_logger->loggedText();
}

void StorageManager::clearLog()
{
    if (s_logger == 0) {
        s_logger = new Logger(this);
        connect(s_logger, SIGNAL(textAddedToLog(QString)), SIGNAL(textAddedToLog(QString)));
        connect(s_logger, SIGNAL(cleared()), SIGNAL(logCleared()));
    }
    s_logger->clear();
}

// Remembering recent searches

void StorageManager::setRecentSearchQuery(const QString &_searchQuery)
{
    QString searchQuery = _searchQuery.simplified();
    if (searchQuery.isEmpty()) {
        return;
    }

    IniFile recentsIni = notesDataIniFile("recents.ini");
    QStringList recentSearches = recentsIni.value("RecentSearches").toStringList();
    bool changed = false;
    const QRegExp regex(searchQuery, Qt::CaseInsensitive, QRegExp::FixedString);
    int existingIndex = recentSearches.indexOf(regex);
    if (existingIndex >= 1) { // already in the recent searches list, and not already at the front of the list
        // bring it to the front of the list
        recentSearches.removeAt(existingIndex);
        recentSearches.prepend(searchQuery);
        changed = true;
    } else if (existingIndex < 0) { // not already in the recent searches list
        // add it to the list; make sure list size doesn't grow above 10
        recentSearches.prepend(searchQuery);
        while (recentSearches.size() > 10) {
            recentSearches.takeLast();
        }
        changed = true;
    }
    if (changed) {
        recentsIni.setValue("RecentSearches", recentSearches);
        recentsIni.sync();
    }
}

QStringList StorageManager::recentSearchQueries()
{
    IniFile recentsIni = notesDataIniFile("recents.ini");
    return recentsIni.value("RecentSearches").toStringList();
}

void StorageManager::clearRecentSearchQueries()
{
    IniFile recentsIni = notesDataIniFile("recents.ini");
    recentsIni.removeKey("RecentSearches");
}

// private methods

QString StorageManager::createStorageObject(const QString &basePath, const QString &objectPrefix,
                                            const QString &metaFileName, const QString &metaCounterKey, const QString &metaListKey,
                                            const QString &dataFileName1, const QVariantMap &keyValuePairs1,
                                            const QString &dataFileName2, const QVariantMap &keyValuePairs2)
{
    Q_ASSERT(!basePath.isEmpty() && !objectPrefix.isEmpty() && !metaFileName.isEmpty() && !metaCounterKey.isEmpty() && !metaListKey.isEmpty());

    // Generate a new noteId / notebookId / tagId
    QString objectId;
    {
        IniFile metaIniFile = notesDataIniFile(basePath % "/" % metaFileName);
        int currentMaxLocalIdNumber = metaIniFile.value(metaCounterKey, 0).toInt();
        currentMaxLocalIdNumber++;
        QString numStr;
        objectId = objectPrefix % numStr.sprintf("%06x", currentMaxLocalIdNumber);
        metaIniFile.setValue(metaCounterKey, currentMaxLocalIdNumber);
    }

    Q_ASSERT(!objectId.isEmpty());

    // Write data into the note / notebook / tag's ini file
    if (!dataFileName1.isEmpty() && !keyValuePairs1.isEmpty()) {
        IniFile dataIniFile = notesDataIniFile(basePath % "/" % ID_PATH(objectId) % "/" % dataFileName1);
        dataIniFile.setValues(keyValuePairs1);
    }
    if (!dataFileName2.isEmpty() && !keyValuePairs2.isEmpty()) {
        IniFile dataIniFile = notesDataIniFile(basePath % "/" % ID_PATH(objectId) % "/" % dataFileName2);
        dataIniFile.setValues(keyValuePairs2);
    }

    // Add the id of the new note / notebook / tag to the meta ini file
    {
        IniFile metaIniFile = notesDataIniFile(basePath % "/" % metaFileName);
        QString objectIdsStr = metaIniFile.value(metaListKey).toString();
        if (objectIdsStr.isEmpty()) {
            objectIdsStr = objectId;
        } else {
            objectIdsStr.prepend(objectId % ",");
        }
        metaIniFile.setValue(metaListKey, objectIdsStr);
    }

    return objectId;
}

QStringList StorageManager::listNoteIds(const QString &metaListFile, const QString &metaListKey)
{
    QString noteIdsStr;
    {
        IniFile rootLocalIni = notesDataIniFile(metaListFile);
        noteIdsStr = rootLocalIni.value(metaListKey).toString();
    }
    QStringList noteIds;
    if (!noteIdsStr.isEmpty()) {
        noteIds = noteIdsStr.split(",");
    }
    return noteIds;
}

QVariantList StorageManager::listNotes(const QString &metaListFile, const QString &metaListKey)
{
    QStringList noteIdsList = listNoteIds(metaListFile, metaListKey);
    return listNotesFromNoteIds(noteIdsList);
}

QVariantMap StorageManager::noteSummaryData(const QString &noteId)
{
    QVariantMap noteDataMap;
    QString guid;
    qint64 timestamp;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        guid = noteGistIni.value("guid").toString();
        noteDataMap[QString::fromLatin1("NoteId")] = noteId;
        noteDataMap[QString::fromLatin1("Title")] = noteGistIni.value("Title").toString();
        noteDataMap[QString::fromLatin1("Favourite")] = noteGistIni.value("Favourite").toBool();

        QString thumbnailPath = noteGistIni.value("ThumbnailPath").toString();
        if (!thumbnailPath.isEmpty() && QFile::exists(notesDataLocation() % "/" % thumbnailPath)) {
            noteDataMap[QString::fromLatin1("ThumbnailPath")] = QString("file:///" % notesDataLocation() % "/" % thumbnailPath);
            noteDataMap[QString::fromLatin1("ThumbnailWidth")] = noteGistIni.value("ThumbnailWidth").toInt();
            noteDataMap[QString::fromLatin1("ThumbnailHeight")] = noteGistIni.value("ThumbnailHeight").toInt();
        } else {
            noteDataMap[QString::fromLatin1("ThumbnailPath")] = QString("");
            noteDataMap[QString::fromLatin1("ThumbnailWidth")] = 0;
            noteDataMap[QString::fromLatin1("ThumbnailHeight")] = 0;
        }

        timestamp = noteGistIni.value("UpdatedTime").toLongLong();
        if (timestamp <= 0) {
            timestamp = noteGistIni.value("CreatedTime").toLongLong();
        }
        noteDataMap[QString::fromLatin1("MillisecondsSinceEpoch")] = timestamp;
        noteDataMap[QString::fromLatin1("Timestamp")] = QDateTime::fromMSecsSinceEpoch(timestamp);
    }
    {
        IniFile noteContentIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/content.ini");
        bool contentValid = noteContentIni.value("ContentValid").toBool();
        QByteArray enmlContent;
        QString contentSummary;
        if (contentValid) {
            enmlContent = noteContentIni.value("Content").toByteArray();
        } else {
            Q_ASSERT(!guid.isEmpty());
            QByteArray cachedContent, cachedContentHash;
            SharedDiskCache::instance()->retrieveNoteContent(guid, &cachedContent, &cachedContentHash);
            enmlContent = cachedContent;
        }
        if (!enmlContent.isEmpty()) {
            contentSummary = EvernoteMarkup::plainTextFromEnml(enmlContent, 100);
        }
        noteDataMap[QString::fromLatin1("ContentSummary")] = contentSummary;
    }
    return noteDataMap;
}

QVariantList StorageManager::listNotesFromNoteIds(const QStringList &noteIdsList)
{
    QVariantList returnList;
    foreach (const QString &noteId, noteIdsList) {
        Q_ASSERT(!noteId.isEmpty());
        QVariantMap noteDataMap = noteSummaryData(noteId);
        returnList << noteDataMap;
    }
    return returnList;
}

QStringList StorageManager::idsList(const QString &metaListFile, const QString &metaListKey)
{
    IniFile rootLocalIni = notesDataIniFile(metaListFile);
    QString objectIdsStr = rootLocalIni.value(metaListKey).toString();
    QStringList objectIds;
    if (!objectIdsStr.isEmpty()) {
        objectIds = objectIdsStr.split(",");
    }
    return objectIds;
}

QVariantList StorageManager::listTagsChecked(const QStringList &tagIdList, const QSet<QString> &checkedTagIds)
{
    QMap<QString, QVariant> orderedTags;
    foreach (const QString &tagId, tagIdList) {
        IniFile tagGistIni = notesDataIniFile("Tags/" % ID_PATH(tagId) % "/list.ini");
        QVariantMap tagGist;
        tagGist[QString::fromLatin1("TagId")] = tagId;
        QString tagName = tagGistIni.value("Name").toString();
        tagGist[QString::fromLatin1("Name")] = tagName;
        tagGist[QString::fromLatin1("Checked")] = checkedTagIds.contains(tagId);
        orderedTags.insert(tagName.toLower(), tagGist);
    }
    return orderedTags.values();
}

bool StorageManager::addNoteIdToNotebookData(const QString &noteId, const QString &notebookId)
{
    if (notebookId.isEmpty())
        return false;
    bool added = addObjectIdToCollectionData("Notebooks/" % ID_PATH(notebookId) % "/list.ini", "NoteIds", noteId);
#ifdef DEBUG
        qDebug() << "Note " << noteId << " added to notebook " << notebookId;
#endif
        return added;
}

bool StorageManager::removeNoteIdFromNotebookData(const QString &noteId, const QString &notebookId)
{
    if (notebookId.isEmpty())
        return false;
    bool removed = removeObjectIdFromCollectionData("Notebooks/" % ID_PATH(notebookId) % "/list.ini", "NoteIds", noteId);
    Q_ASSERT(removed);
#ifdef DEBUG
        qDebug() << "Note " << noteId << " removed from notebook " << notebookId;
#endif
    return removed;
}

bool StorageManager::addNoteIdToTagData(const QString &noteId, const QString &tagId)
{
    if (tagId.isEmpty())
        return false;
    bool added = addObjectIdToCollectionData("Tags/" % ID_PATH(tagId) % "/list.ini", "NoteIds", noteId);
#ifdef DEBUG
        qDebug() << "Note " << noteId << " added under tag " << tagId;
#endif
    return added;
}

bool StorageManager::removeNoteIdFromTagData(const QString &noteId, const QString &tagId)
{
    if (tagId.isEmpty())
        return false;
    bool removed = removeObjectIdFromCollectionData("Tags/" % ID_PATH(tagId) % "/list.ini", "NoteIds", noteId);
#ifdef DEBUG
        qDebug() << "Note " << noteId << " removed from under tag " << tagId;
#endif
    return removed;
}

bool StorageManager::addObjectIdToCollectionData(const QString &collectionIniFilename, const QString &objectListKey, const QString &objectId)
{
#ifdef DEBUG
    qDebug() << "Adding" << objectId << "to field" << objectListKey << "of" << collectionIniFilename;
#endif
    if (objectId.isEmpty()) {
        return true;
    }
    IniFile collectionIni = notesDataIniFile(collectionIniFilename);
    QString objectIdsStr = collectionIni.value(objectListKey).toString();
    QStringList objectIds;
    if (!objectIdsStr.isEmpty()) {
        objectIds = objectIdsStr.split(",");
    }
    if (!objectIds.contains(objectId)) {
        objectIds.prepend(objectId);
        collectionIni.setValue(objectListKey, objectIds.join(","));
        return true;
    }
    return false;
}

bool StorageManager::removeObjectIdFromCollectionData(const QString &collectionIniFilename, const QString &objectListKey, const QString &objectId)
{
#ifdef DEBUG
    qDebug() << "Removing" << objectId << "from field" << objectListKey << "of" << collectionIniFilename;
#endif
    if (objectId.isEmpty()) {
        return true;
    }
    IniFile collectionIni = notesDataIniFile(collectionIniFilename);
    QString objectIdsStr = collectionIni.value(objectListKey).toString();
    QStringList objectIds = objectIdsStr.split(",");
    bool removed = objectIds.removeOne(objectId);
    if (removed) {
        collectionIni.setValue(objectListKey, objectIds.join(","));
        return true;
    }
    return false;
}

void StorageManager::removeCollectionKey(const QString &collectionIniFilename, const QString &objectListKeyToRemove)
{
#ifdef DEBUG
    qDebug() << "Removing field" << objectListKey << "of" << collectionIniFilename;
#endif
    if (objectListKeyToRemove.isEmpty()) {
        return;
    }
    IniFile collectionIni = notesDataIniFile(collectionIniFilename);
    collectionIni.removeKey(objectListKeyToRemove);
}

void StorageManager::setGuidMapping(const QString &guidMapFile, const QString &guid, const QString &localId)
{
    IniFile mapIni = notesDataIniFile(guidMapFile);
    mapIni.setValue(guid, localId);
}

void StorageManager::removeGuidMapping(const QString &guidMapFile, const QString &guid)
{
    IniFile mapIni = notesDataIniFile(guidMapFile);
    mapIni.removeKey(guid);
}

QString StorageManager::localIdForGenericGuid(const QString &guidMapFile, const QString &guid)
{
    IniFile mapIni = notesDataIniFile(guidMapFile);
    return mapIni.value(guid).toString();
}

void StorageManager::removeNoteReferences(const QString &noteId, StorageConstants::NotesListTypes referencesInWhatLists)
{
    if (noteId.isEmpty()) {
        return;
    }
    QString notebookId;
    QString tagIdsStr;
    bool isTrashed = false;
    {
        IniFile noteGistIni = notesDataIniFile("Notes/" % ID_PATH(noteId) % "/gist.ini");
        notebookId = noteGistIni.value("NotebookId").toString();
        tagIdsStr = noteGistIni.value("TagIds").toString();
        isTrashed = noteGistIni.value("Trashed").toBool();
    }

    // remove from notebook ini
    if ((referencesInWhatLists & StorageConstants::NotesInNotebook) == StorageConstants::NotesInNotebook) {
        bool notebookDoesExist = notebookExists(notebookId);
        if (notebookDoesExist && !isTrashed) {
            removeNoteIdFromNotebookData(noteId, notebookId);
        }
    }

    // remove from tag ini
    if ((referencesInWhatLists & StorageConstants::NotesWithTag) == StorageConstants::NotesWithTag) {
        QStringList tagIds;
        if (!tagIdsStr.isEmpty()) {
            tagIds = tagIdsStr.split(",");
        }
        foreach (const QString &tagId, tagIds) {
            removeNoteIdFromTagData(noteId, tagId);
        }
    }

    // remove from all-notes ini
    if ((referencesInWhatLists & StorageConstants::AllNotes) == StorageConstants::AllNotes) {
        removeObjectIdFromCollectionData("Notes/list.ini", "NoteIds", noteId);
    }

    // remove from favourites ini
    if ((referencesInWhatLists & StorageConstants::FavouriteNotes) == StorageConstants::FavouriteNotes) {
        removeObjectIdFromCollectionData("SpecialNotebooks/Favourites/list.ini", "NoteIds", noteId);
    }

    // remove from trash ini
    if ((referencesInWhatLists & StorageConstants::TrashNotes) == StorageConstants::TrashNotes) {
        removeObjectIdFromCollectionData("SpecialNotebooks/Trash/list.ini", "NoteIds", noteId);
    }
}

StorageManager::ThreadSafeSettings* StorageManager::rawSettings(const QString &fileName)
{
    ThreadSafeSettings *settings = m_iniFileCache.object(fileName);
    if (settings == NULL) { // not in the cache
        QFileInfo fileInfo(notesDataLocation() % "/" % fileName);
        int fileSize;
        if (!fileInfo.exists()) { // not even in disk
            // create the containing dir so QSettings can create the file
            QDir(notesDataLocation()).mkpath(QFileInfo(fileName).path());
            QDir(qApp->applicationDirPath()).mkpath(fileInfo.path());
            fileSize = 1000; // random guess on file size
        } else {
            fileSize = fileInfo.size();
        }
        settings = new ThreadSafeSettings;
        if (fileName.endsWith(".dat")) {
            settings->settings = new QSettings(notesDataLocation() % "/" % fileName, m_encryptedSettingsFormat);
        } else {
            settings->settings = new QSettings(notesDataLocation() % "/" % fileName, QSettings::IniFormat);
        }
#ifdef THREAD_SAFE_STORE
        settings->cacheLock = &m_cacheLock;
        bool cacheWriteLocked = false;
        if (m_iniFileCache.totalCost() + fileSize >= m_iniFileCache.maxCost()) {
            // this insertion might cause something else in the cache to get deleted.
            // to be safe, we prevent any other method to access anything in the cache during the insertion.
            m_cacheLock.lockForWrite();
            cacheWriteLocked = true;
        }
#endif
        m_iniFileCache.insert(fileName, settings, fileSize);
#ifdef THREAD_SAFE_STORE
        if (cacheWriteLocked) {
            m_cacheLock.unlock();
        }
#endif
    }
    return settings;
}

bool StorageManager::removeSettingsFile(const QString &fileName)
{
    ThreadSafeSettings *settings = m_iniFileCache.object(fileName);
    if (settings != NULL) { // in the cache
        m_cacheLock.lockForWrite();
        m_iniFileCache.remove(fileName);
        m_cacheLock.unlock();
    }
    QString settingsFilePath = notesDataLocation() % "/" % fileName;
    if (QFileInfo(settingsFilePath).exists()) {
        return QFile::remove(settingsFilePath);
    }
    return true;
}

StorageManager::IniFile StorageManager::sessionDataIniFile(const QString &fileName)
{
    return IniFile(rawSettings("Store/Session/" % fileName));
}

QString StorageManager::notesDataFullPath()
{
    return notesDataLocation() % "/" % notesDataRelativePath();
}

QString StorageManager::notesDataRelativePath()
{
    QString userDirName = activeUserDirName();
    if (userDirName.isEmpty()) {
        userDirName = "UNKNOWNUSER"; // Just in case
    }
    QString path = "Store/Data/" % userDirName % "/notedata";
    if (!QFile::exists(notesDataLocation() % "/" % path)) {
        bool ok = QDir(notesDataLocation()).mkpath(path);
        Q_ASSERT(ok);
    }
    return path;
}

StorageManager::IniFile StorageManager::notesDataIniFile(const QString &fileName)
{
    return IniFile(rawSettings(notesDataRelativePath() % "/" % fileName));
}

QString StorageManager::evernoteDataIniFilePath(const QString &fileName)
{
    QString userDirName = activeUserDirName();
    if (userDirName.isEmpty()) {
        userDirName = "UNKNOWNUSER"; // Just in case
    }
    return ("Store/Data/" % userDirName % "/evernote/" % fileName);
}

QString StorageManager::evernoteDataIniFilePathForUser(const QString &username, const QString &fileName)
{
    QString userDirName("UNKNOWNUSER");
    if (!username.isEmpty()) {
        QByteArray userHash = QCryptographicHash::hash(username.toLower().toLatin1(), QCryptographicHash::Sha1).toHex().left(8);
        userDirName = QString("ev" % userHash);
    }
    return ("Store/Data/" % userDirName % "/evernote/" % fileName);
}

StorageManager::IniFile StorageManager::evernoteDataIniFile(const QString &fileName)
{
    return IniFile(rawSettings(evernoteDataIniFilePath(fileName)));
}

StorageManager::IniFile StorageManager::evernoteDataIniFileForUser(const QString &username, const QString &fileName)
{
    return IniFile(rawSettings(evernoteDataIniFilePathForUser(username, fileName)));
}

StorageManager::IniFile StorageManager::preferencesIniFile(const QString &fileName)
{
    return IniFile(rawSettings("Store/Preferences/" % fileName));
}

class StorageManager::IniFileData : public QSharedData {
public:
    IniFileData(StorageManager::ThreadSafeSettings *s)
        : settings(s), isChanged(false)
    {
#ifdef THREAD_SAFE_STORE
        settings->cacheLock->lockForRead();
        settings->mutex.lock();
#endif
    }
    ~IniFileData()
    {
#ifdef THREAD_SAFE_STORE
        settings->mutex.unlock();
        settings->cacheLock->unlock();
#endif
    }

    StorageManager::ThreadSafeSettings *settings;
    bool isChanged;
private:
    Q_DISABLE_COPY(IniFileData) // IniFileData is explicitly shared, and should never be copied
};

StorageManager::IniFile::IniFile(StorageManager::ThreadSafeSettings *settings)
    : d(new StorageManager::IniFileData(settings))
{
}

StorageManager::IniFile::~IniFile()
{
    sync();
}

StorageManager::IniFile::IniFile(const IniFile &other)
    : d(other.d)
{
}

void StorageManager::IniFile::setValue(const QString &key, const QVariant &value)
{
    d->settings->settings->setValue(key, value);
    d->isChanged = true;
}

void StorageManager::IniFile::setValues(const QVariantMap &keyValuePairs)
{
    QMapIterator<QString, QVariant> iter(keyValuePairs);
    while (iter.hasNext()) {
        iter.next();
        const QString &key = iter.key();
        const QVariant &value = iter.value();
        if (!key.isEmpty()) {
            setValue(key, value);
        }
    }
}

QVariant StorageManager::IniFile::value(const QString &key, const QVariant &defaultValue) const
{
    return d->settings->settings->value(key, defaultValue);
}

void StorageManager::IniFile::removeKey(const QString &key)
{
    d->settings->settings->remove(key);
    d->isChanged = true;
}

void StorageManager::IniFile::beginWriteArray(const QString &prefix, int size)
{
    d->settings->settings->beginWriteArray(prefix, size);
}

int StorageManager::IniFile::beginReadArray(const QString &prefix)
{
    return d->settings->settings->beginReadArray(prefix);
}

void StorageManager::IniFile::setArrayIndex(int i)
{
    d->settings->settings->setArrayIndex(i);
}

void StorageManager::IniFile::endArray()
{
    d->settings->settings->endArray();
}

void StorageManager::IniFile::sync()
{
    if (d->isChanged) {
        d->settings->settings->sync();
        d->isChanged = false;
    }
}
