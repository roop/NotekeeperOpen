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

#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <QObject>
#include <QCache>
#include <QSettings>
#include <QVariant>
#include <QDeclarativeContext> // for Q_DECLARE_METATYPE(QList<QObject*>)
#include <QSet>
#include <QDeclarativeItem>
#include <QExplicitlySharedDataPointer>
#include <QMutex>
#include <QReadWriteLock>
#include <QTemporaryFile>

#define THREAD_SAFE_STORE

class StorageManager;
class Logger;

class StorageConstants : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS(NotebookType)
    Q_ENUMS(NotesListType)
public:
    enum NotebookType {
        NoNotebook = 0x0,
        NormalNotebook = 0x1,
        FavouritesNotebook = 0x2,
        TrashNotebook = 0x4
    };
    enum NotesListType {
        NoNotes = 0x0,
        AllNotes = 0x1,
        NotesInNotebook = 0x2,
        NotesWithTag = 0x4,
        FavouriteNotes = 0x8,
        TrashNotes = 0x10,
        SearchResultNotes = 0x20
    };
    Q_DECLARE_FLAGS(NotebookTypes, NotebookType)
    Q_DECLARE_FLAGS(NotesListTypes, NotesListType)
};
Q_DECLARE_OPERATORS_FOR_FLAGS(StorageConstants::NotebookTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(StorageConstants::NotesListTypes)

void redirectMessageToLog(QtMsgType type, const char *msg); // message handler

class StorageManager : public QObject
{
    Q_OBJECT
public:
    enum {
        NoteObject,
        NotebookObject
    } ObjectType;

    explicit StorageManager(QObject *parent = 0);

    void writeStorageVersion(const QString &versionString);
    QString notesDataLocation() const;

    // Notes
    QString createNote(const QString &title, const QByteArray &content, const QString &sourceUrl = QString()); // returns noteId
    bool updateNoteTitle(const QString &noteId, const QString &title);
    bool updateNoteTitleAndContent(const QString &noteId, const QString &title, const QByteArray &content,
                                   const QByteArray &contentBaseHash /* value of contentBaseHash when the user opened this note */);
    QVariantList listNotes(StorageConstants::NotesListType whichNotes, const QString &objectId = QString() /* notebook/tag id */);
    QStringList listNoteIds(StorageConstants::NotesListType whichNotes, const QString &objectId = QString() /* notebook/tag id */);
    QVariantMap noteSummaryData(const QString &noteId);
    QVariantList listNotesFromNoteIds(const QStringList &noteIdsList);
    QVariantMap noteData(const QString &noteId); // returns Title, ContentFetched, Content, Favourite, Trashed
    QByteArray enmlContentFromContentIni(const QString &noteId, bool *ok = 0);

    QString setSyncedNoteGist(const QString &guid, const QString &title, qint32 usn, const QByteArray &contentHash, qint64 createdTime, qint64 updatedTime,
                              const QVariantMap &noteAttributes, bool *usnChanged = 0);
    bool setFetchedNoteContent(const QString &noteGuid, const QString &title, const QByteArray &content, const QByteArray &contentHash, qint32 usn, qint64 updatedTime,
                               const QVariantMap &noteAttributes, const QVariantList &attachmentsData);
    bool setPushedNote(const QString &noteId, const QString &title, const QByteArray &content, const QByteArray &contentHash, qint32 usn, qint64 updatedTime);
    int noteInsertionPositionInNotesList(const QString &noteId, const QStringList &noteIdsList);
    void updateNotesListOrder(const QString &noteId);
    bool isNoteContentUpToDate(const QString &noteId);

    void setNoteGuid(const QString &noteId, const QString &guid);
    QString guidForNoteId(const QString &noteId);
    QString noteIdForGuid(const QString &guid);

    QString createNotebook(const QString &name, const QString &firstNoteId = QString()); // returns notebookId
    QString setSyncedNotebookData(const QString &guid, const QString &name); // return notebookId
    bool setNotebookForNote(const QString &noteId, const QString &notebookId);
    QString notebookForNote(const QString &noteId); // return notebookId
    QStringList listNormalNotebookIds();
    QVariantList listNotebooks(StorageConstants::NotebookTypes notebookTypes);
    bool notebookExists(const QString &notebookId);
    void setNotebookName(const QString &notebookId, const QString &name);
    QString notebookName(const QString &notebookId);
    QString notebookIdForNotebookName(const QString &notebookName);
    bool renameNotebook(const QString &notebookId, const QString &name);
    void setNotebookGuid(const QString &notebookId, const QString &guid);
    QString guidForNotebookId(const QString &notebookId);
    QString notebookIdForGuid(const QString &guid);

    void setDefaultNotebookId(const QString &notebookId);
    QString defaultNotebookId();
    QString defaultNotebookName();
    void setOfflineNotebookIds(const QStringList &offlineNotebookIds);
    QStringList offlineNotebookIds();

    QString createTag(const QString &name, const QString &firstNoteId = QString()); // returns tagId
    QString setSyncedTagData(const QString &guid, const QString &name); // returns tagId
    bool setTagsOnNote(const QString &noteId, const QStringList &tagIds);
    bool setTagsOnNote(const QString &noteId, const QString &tagIds /* comma separated */);
    void addTagToNote(const QString &noteId, const QString &tagId);
    QVariantList listTagsWithTagsOnNoteChecked(const QString &noteId);
    QVariantList listTags();
    bool tagExists(const QString &tagId);
    QStringList tagsOnNote(const QString &noteId);
    void setTagName(const QString &tagId, const QString &name);
    QString tagName(const QString &tagId);
    bool renameTag(const QString &tagId, const QString &name);
    void setTagGuid(const QString &tagId, const QString &guid);
    QString guidForTagId(const QString &tagId);
    QString tagIdForGuid(const QString &guid);

    bool setFavouriteNote(const QString &noteId, bool isFavourite);
    bool isFavouriteNote(const QString &noteId);

    bool moveNoteToTrash(const QString &noteId);
    bool restoreNoteFromTrash(const QString &noteId);
    bool isTrashNote(const QString &noteId);
    bool expungeNoteFromTrash(const QString &noteId);
    void expungeNote(const QString &noteId);
    void expungeNotebook(const QString &notebookId);
    void expungeTag(const QString &tag);

    bool setNoteThumbnail(const QString &noteId, const QImage &image, const QByteArray &sourceImageHash = QByteArray());
    bool noteThumbnailExists(const QString &noteId);
    QByteArray noteThumbnailSourceHash(const QString &noteId);

    void saveSetting(const QString &key, bool value);
    bool retrieveSetting(const QString &key);
    void saveStringSetting(const QString &key, const QString &value);
    QString retrieveStringSetting(const QString &key);

    void setNoteHasUnpushedChanges(const QString &noteId, bool hasUnpushedChanges = true);
    QStringList notesWithUnpushedChanges();
    bool noteHasUnpushedChanges(const QString &noteId);
    bool noteHasUnpushedContentChanges(const QString &noteId);
    void setNotebookHasUnpushedChanges(const QString &notebookId, bool hasUnpushedChanges = true);
    QStringList notebooksWithUnpushedChanges();
    void setTagHasUnpushedChanges(const QString &tagId, bool hasUnpushedChanges = true);
    QStringList tagsWithUnpushedChanges();

    void setAttachmentsDataFromServer(const QString &noteId, const QVariantList &attachmentsData, bool isAfterPush, int *removedImagesCount = 0);
    void addAttachmentData(const QString &noteId, const QVariantMap &attachmentData);
    bool removeAttachmentData(const QString &noteId, const QByteArray &md5Hash);
    QVariantList attachmentsData(const QString &noteId);
    qint64 noteAttachmentsTotalSize(const QString &noteId);

    bool saveCheckboxStates(const QString &noteId, const QVariantList &checkboxStates, const QByteArray &enmlContent, const QByteArray &previousBaseContentHash);
    bool appendTextToNote(const QString &noteId, const QString &text, const QByteArray &enmlContent, const QByteArray &previousBaseContentHash, QByteArray *updatedEnml, QByteArray *htmlToAdd);
    bool appendAttachmentToNote(const QString &noteId, const QString &filePath, const QVariantMap &attributes, const QByteArray &enmlContent, const QByteArray &baseContentHash,
                                QByteArray *updatedEnml, QByteArray *htmlToAdd, QByteArray *attachmentHash, QString *absoluteFilePath);
    bool removeAttachmentFile(const QString &noteId, const QVariantMap &attachmentData);

    bool setUnremovedTempFile(const QString &tempFilePath, bool unremoved = true);
    QStringList unremovedTempFiles();

    // Offline notes
    bool noteNeedsToBeAvailableOffline(const QString &noteId);
    bool noteContentAvailableOffline(const QString &noteId);
    bool noteAttachmentsAvailableOffline(const QString &noteId);
    void setOfflineStatusChangeUnresolvedNote(const QString &noteId, bool changed = true);
    QStringList offlineStatusChangeUnresolvedNotes();
    bool tryMakeNoteContentAvailableOfflineWithCachedData(const QString &noteId);
    void makeNoteContentUnavailableOfflineIfUnedited(const QString &noteId);
    bool tryMakeNoteAttachmentsAvailableOfflineWithCachedData(const QString &noteId);
    void makeNoteAttachmentsUnavailableOffline(const QString &noteId);
    void tryResolveOfflineStatusChange(const QString &noteId, bool *requiredOffline = 0, bool *madeOffline = 0);
    void tryResolveOfflineStatusChanges();
    bool setOfflineNoteAttachmentContent(const QString &noteId, const QByteArray &attachmentHash, QTemporaryFile *attachmentFile);
    void setOfflineStatusChangeUnresolvedNotebook(const QString &notebookId, bool changed = true);
    QStringList offlineStatusChangeUnresolvedNotebooks();

    // For storing Evernote auth credentials
    void saveEvernoteAuthData(const QString &key, const QString &value);
    QString retrieveEvernoteAuthData(const QString &key);
    void saveEncryptedEvernoteAuthData(const QString &key, const QString &value);
    QString retrieveEncryptedEvernoteAuthData(const QString &key);
    bool isAuthDataValid();
    void purgeEvernoteAuthData();

    // Bookkeeping information used during Evernote sync
    void saveEvernoteSyncData(const QString &key, const QVariant &value);
    QVariant retrieveEvernoteSyncData(const QString &key);
    QVariant retrieveEvernoteSyncDataForUser(const QString &username, const QString &key);
    bool addToEvernoteSyncIdsList(const QString &key, const QString &id);
    QStringList retrieveEvernoteSyncIdsList(const QString &key);
    void clearEvernoteSyncIdsList(const QString &key);

    // User management: Different users have stuff stored separately

    void setActiveUser(const QString &username);
    QString activeUser();
    QString activeUserDirName();
    void purgeActiveUserData();
    void setReadyForCreateNoteRequests(bool isReady);

    // Logging
    void log(const QString &message);
    QString loggedText();
    void clearLog();

    // Remembering recent searches
    void setRecentSearchQuery(const QString &searchQuery);
    QStringList recentSearchQueries();
    void clearRecentSearchQueries();

signals:
    void notesListChanged(StorageConstants::NotesListType whichNotes, const QString &objectId /* notebook/tag id */);
    void notebooksListChanged();
    void tagsListChanged();
    void noteCreated(const QString &noteId);
    void noteExpunged(const QString &noteId);
    void noteDisplayDataChanged(const QString &noteId, bool timestampChanged);
    void notebookForNoteChanged(const QString &noteId, const QString &notebookId);
    void tagsForNoteChanged(const QString &noteId, const QStringList &tagIds);
    void noteFavouritenessChanged(const QString &noteId, bool isFavourite);
    void noteTrashednessChanged(const QString &noteId, bool isTrashed);
    void textAddedToLog(const QString &text);
    void logCleared();

private:

    struct ThreadSafeSettings {
        QSettings *settings;
#ifdef THREAD_SAFE_STORE
        QMutex mutex; // Makes sure that that the same file's data is not accessed by two threads at the same time
        QReadWriteLock *cacheLock; // A pointer to StorageManager::m_cacheLock
#endif
        ~ThreadSafeSettings() { delete settings; }
    };

    class IniFileData;
    class IniFile {
    public:
        IniFile(const IniFile &other);
        ~IniFile();
        void setValue(const QString &key, const QVariant &value);
        void setValues(const QVariantMap &keyValuePairs);
        QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
        void removeKey(const QString &key);
        void beginWriteArray(const QString &prefix, int size = -1);
        int beginReadArray(const QString &prefix);
        void setArrayIndex(int i);
        void endArray();
        void sync();
    private:
        explicit IniFile(ThreadSafeSettings *settings);
        QExplicitlySharedDataPointer<IniFileData> d;
        friend class StorageManager;
    };

    QString createStorageObject(const QString &basePath, const QString &localObjectPrefix,
                                const QString &metaFileName, const QString &metaCounterKey, const QString &metaListKey,
                                const QString &dataFileName1 = QString(), const QVariantMap &keyValuePairs1 = QVariantMap(),
                                const QString &dataFileName2 = QString(), const QVariantMap &keyValuePairs2 = QVariantMap());
    QStringList listNoteIds(const QString &metaListFile, const QString &metaListKey);
    QVariantList listNotes(const QString &metaListFile, const QString &metaListKey);
    QStringList idsList(const QString &metaListFile, const QString &metaListKey);
    QVariantList listTagsChecked(const QStringList &tagIdsList, const QSet<QString> &checkedTagIds = QSet<QString>());
    bool addNoteIdToNotebookData(const QString &noteId, const QString &notebookId);
    bool removeNoteIdFromNotebookData(const QString &noteId, const QString &notebookId);
    bool addNoteIdToTagData(const QString &noteId, const QString &tagId);
    bool removeNoteIdFromTagData(const QString &noteId, const QString &tagId);
    bool addObjectIdToCollectionData(const QString &collectionIniFilename, const QString &objectListKey, const QString &objectId);
    bool removeObjectIdFromCollectionData(const QString &collectionIniFilename, const QString &objectListKey, const QString &objectId);
    void removeCollectionKey(const QString &collectionIniFilename, const QString &objectListKeyToRemove);
    void setGuidMapping(const QString &guidMapFile, const QString &guid, const QString &localId);
    void removeGuidMapping(const QString &guidMapFile, const QString &guid);
    QString localIdForGenericGuid(const QString &guidMapFile, const QString &guid);
    void removeNoteReferences(const QString &noteId, StorageConstants::NotesListTypes referencesInWhatLists);

    IniFile sessionDataIniFile(const QString &fileName);
    QString notesDataRelativePath();
    QString notesDataFullPath(); // = notesDataLocation() + notesDataRelativePath()
    IniFile notesDataIniFile(const QString &fileName);
    QString evernoteDataIniFilePath(const QString &fileName);
    QString evernoteDataIniFilePathForUser(const QString &username, const QString &fileName);
    IniFile evernoteDataIniFile(const QString &fileName);
    IniFile evernoteDataIniFileForUser(const QString &username, const QString &fileName);
    IniFile preferencesIniFile(const QString &fileName);
    ThreadSafeSettings* rawSettings(const QString &fileName);
    bool removeSettingsFile(const QString &fileName);

    // FIXME: Add consts to methods appropriately after const_casting this ptr in iniFile()

    enum LoggingEnabledStatus {
        LoggingEnabledStatusUnknown = -1,
        LoggingDisabled = 0,
        LoggingEnabled = 1
    };

    QCache<QString, ThreadSafeSettings> m_iniFileCache;
#ifdef THREAD_SAFE_STORE
    QReadWriteLock m_cacheLock; // Ensures that the cache retains the data used in the IniFile object
                                // for the lifetime of the IniFile object
#endif
    QString m_activeUserDirName;
    const QSettings::Format m_encryptedSettingsFormat;
    LoggingEnabledStatus m_loggingEnabledStatus;

    static Logger *s_logger;                                           // static so that it can be used ...
    friend void redirectMessageToLog(QtMsgType type, const char *msg); // ... in this message handler
};

#endif // STORAGEMANAGER_H
