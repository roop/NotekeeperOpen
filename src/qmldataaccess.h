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

#ifndef QMLDATAACCESS_H
#define QMLDATAACCESS_H

#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QSslConfiguration>
#include <QTimer>

#include "storage/storagemanager.h"

class ConnectionManager;
class EvernoteAccess;
class EvernoteSync;
class NotesListModel;
class SearchLocalNotesThread;
class ImageReadCancelHelper;

// Used by the QML code to access notes/notebooks/tags data.

class QmlDataAccess : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int searchResultsCount READ searchResultsCount NOTIFY searchResultsCountChanged)
    Q_PROPERTY(int unsearchedNotesCount READ unsearchedNotesCount NOTIFY unsearchedNotesCountChanged)
    Q_PROPERTY(int searchProgressPercentage READ searchProgressPercentage NOTIFY searchProgressPercentageChanged)
    Q_PROPERTY(int offlineNotebooksCount READ offlineNotebooksCount NOTIFY offlineNotebooksCountChanged)
    Q_PROPERTY(qint64 attachmentDownloadedBytes READ attachmentDownloadedBytes NOTIFY attachmentDownloadedBytesChanged)
    Q_PROPERTY(QVariantMap addingAttachmentStatus READ addingAttachmentStatus NOTIFY addingAttachmentStatusChanged)
    Q_PROPERTY(bool isDarkColorSchemeEnabled READ isDarkColorSchemeEnabled WRITE setDarkColorSchemeEnabled NOTIFY isDarkColorSchemeEnabledChanged)
    Q_PROPERTY(int qtVersionRuntimeMinor READ qtVersionRuntimeMinor NOTIFY qtVersionRuntimeMinorChanged)
public:
    QmlDataAccess(StorageManager *storageManager, ConnectionManager *connectionManager, QObject *parent = 0);
    void setSslConfiguration(const QSslConfiguration &sslConf);

    NotesListModel *allNotesListModel() const;
    NotesListModel *notebookNotesListModel() const;
    NotesListModel *tagNotesListModel() const;
    NotesListModel *searchNotesListModel() const;

    int searchResultsCount() const;
    int unsearchedNotesCount() const;
    int searchProgressPercentage() const;

    void setDarkColorSchemeEnabled(bool enabled);
    bool isDarkColorSchemeEnabled() const;

    int qtVersionRuntimeMinor() const;

signals:
    void notesListChanged(int /*StorageConstants::NotesListType*/ whichNotes, const QString &objectId /* notebook/tag id */);
    void notebooksListChanged();
    void tagsListChanged();
    void unsearchedNotesCountChanged();
    void searchResultsCountChanged();
    void searchProgressPercentageChanged();
    void offlineNotebooksCountChanged();
    void isDarkColorSchemeEnabledChanged();
    void qtVersionRuntimeMinorChanged(); // never emitted

public slots:
    QVariant saveNote(const QString &noteId, const QString &titleAndContent, const QVariantList &attachmentsData,
                      const QString &previousTitle, const QString &previousPlainTextContent,
                      const QByteArray &previousBaseContentHash /* value of contentBaseHash when the user opened this note */);
    QString createNoteWithUrl(const QString &titleAndContent, const QString &url);

    bool saveCheckboxStates(const QString &noteId, const QVariantList &checkboxStates, const QByteArray &enmlContent, const QByteArray &previousBaseContentHash);
    QVariant appendTextToNote(const QString &noteId, const QString &text, const QByteArray &enmlContent, const QByteArray &baseContentHash); // returns html to append
    QVariantMap removeAttachmentInstanceFromNote(const QString &noteId, const QString &titleAndContent, const QVariantList &attachmentsData, int attachmentIndexToRemove, const QByteArray &baseContentHash);

    bool setNoteTitle(const QString &noteId, const QString &title);

    QString createNotebook(const QString &name, const QString &firstNoteId = QString()); // returns notebookId
    bool setNotebookForNote(const QString &noteId, const QString &notebookId);
    QString notebookForNote(const QString &noteId); // return notebookId
    bool renameNotebook(const QString &notebookId, const QString &name);
    QString defaultNotebookName();
    void setOfflineNotebookIds(const QString &offlineNotebookIdsStr);
    QStringList offlineNotebookIds();
    int offlineNotebooksCount();

    QString createTag(const QString &name, const QString &firstNoteId = QString()); // returns tagId
    bool setTagsOnNote(const QString &noteId, const QString &tagIds /* comma separated */);
    bool renameTag(const QString &tagId, const QString &name);

    bool setFavouriteNote(const QString &noteId, bool isFavourite);
    bool moveNoteToTrash(const QString &noteId);
    bool restoreNoteFromTrash(const QString &noteId);
    bool expungeNoteFromTrash(const QString &noteId);

    void saveSetting(const QString &key, bool value);
    bool retrieveSetting(const QString &key);
    void saveStringSetting(const QString &key, const QString &value);
    QString retrieveStringSetting(const QString &key);

    void saveEvernoteUsernamePassword(const QString &username, const QString &password);
    bool isAuthTokenValid();

    bool isLoggedIn();
    bool isLoggedOutBecauseLoginMethodIsObsolete();
    bool hasUserLoggedInBefore();
    bool hasUserLoggedInBeforeByName(const QString &username);
    void setActiveUser(const QString &username);
    QString activeUser();
    void logout();
    void clearCookieJar();
    void purgeActiveUserData();
    void setReadyForCreateNoteRequests(bool isReady); // related to share-ui plugin

    QString loggedText();
    void clearLog();

    QVariantMap userAccountData();

    qint64 attachmentDownloadedBytes() const;
    QVariantMap addingAttachmentStatus() const;
    bool removeTempFile(const QString &tempFile);
    void removeAllPendingTempFiles();
    bool createDir(const QString &drive, const QString &path);
    bool fileExistsInFull(const QString &path, qint64 size);

    QVariantMap diskCacheInfo();
    void clearDiskCache();
    void setDiskCacheMaximumSize(qint64 bytes);

    void setRecentSearchQuery(const QString &searchQuery);
    QStringList recentSearchQueries();
    void clearRecentSearchQueries();

    bool resetPreeditText() const;
    int rotationForCorrectOrientation(const QString &imageUrlString) const;

    QString harmattanPicturesDir() const;
    QString harmattanDocumentsDir() const;
    QVariantMap harmattanReadAndRemoveCreateNoteRequest() const;
    bool harmattanRemoveCreateNoteRequest() const;

public slots:
    void startListNotes(int /*StorageConstants::NotesListType*/ whichNotes, const QString &objectId /* notebook/tag id */);
    void startListNotebooks(int /*StorageConstants::NotebookTypes*/ notebookTypes);
    void startListTags();
    void startListTagsWithTagsOnNoteChecked(const QString &noteId);
    void startGetNoteData(const QString &noteId);
    void startSearchLocalNotes(const QString &words);
    void startSearchNotes(const QString &words);
    void startAttachmentDownload(const QString &urlString,
                                 const QString &fileName = QString(),       // target fileName, pass empty string to use a temp fileName
                                 const QString &fileExtension = QString()); // when using temp fileName, file extension to use, if any
    bool startAppendImageToNote(const QString &noteId, const QString &imageUrl, const QByteArray &enmlContent, const QByteArray &baseContentHash);
    bool startAppendAttachmentsToNote(const QString &noteId, const QVariantList &attachments, const QByteArray &enmlContent, const QByteArray &baseContentHash);

    bool cancel();
    void cancelAddingAttachment();

    void tryResolveOfflineStatusChanges();
    void unscheduleTryResolveOfflineStatusChanges();

signals:
    void gotNotesList();
    void gotAllNotesList();
    void gotNotebooksList(int /*StorageConstants::NotebookTypes*/ notebookTypes, const QVariantList &notebooksList);
    void gotTagsList(const QVariantList &tagsList);
    void gotTagsListWithTagsOnNoteChecked(const QVariantList &tagsList);
    void gotNoteTitle(const QString &noteTitle);
    void gotNoteData(const QVariantMap &noteData);
    void searchLocalNotesFinished();
    void searchServerNotesFinished();
    void errorFetchingNote(const QString &message);
    void aboutToQuit();
    void aboutToLogout();
    void textAddedToLog(const QString &text);
    void logCleared();
    void attachmentDownloadedBytesChanged();
    void attachmentDownloaded(const QString &downloadedFilePath);
    void addingAttachmentStatusChanged();
    void addedAttachment(const QVariantMap &result);
    void doneAddingAttachments(bool success, const QString &message, const QString &erroredFileName);
    void authTokenInvalid();

    void cancelAddingAttachmentPrivateSignal();

private slots:
    void storageManagerNotesListChanged(StorageConstants::NotesListType whichNotes, const QString &objectId);
    void searchLocalNotesMatchObtained(const QString &noteId);
    void searchLocalNotesProgressPercentageChanged(int searchProgressPercentage);
    void searchLocalNotesThreadFinished(int unsearchedNotesCount);
    void searchServerNotesResultObtained(const QStringList &noteIds, int searchProgressPercentage);
    void fetchNoteDataFinished(bool success, const QString &message);
    void attachmentDownloadProgressed(qint64 bytesDownloaded);
    void updateAddingAttachmentStatus(const QVariantMap &status);

private:
    StorageManager *m_storageManager;
    EvernoteAccess *m_evernoteAccess;
    QTimer m_offlineStatusChangedTimer;
    NotesListModel *m_allNotesListModel, *m_notebookNotesListModel, *m_tagNotesListModel, *m_searchNotesListModel;
    SearchLocalNotesThread *m_searchLocalNotesThread;
    int m_searchResultsCount, m_unsearchedNotesCount, m_searchProgressPercentage;
    qint64 m_attachmentDownloadedBytes;
    QVariantMap m_addingAttachmentStatus;
    bool m_isDarkColorSchemeEnabled;
};

class ListNotesThread : public QThread
{
    Q_OBJECT
public:
    ListNotesThread(NotesListModel *notesListModel, StorageConstants::NotesListType whichNotes, const QString &objectId, QObject *parent = 0);
    void run();
signals:
    void gotNotesList();
private:
    NotesListModel *m_notesListModel;
    const StorageConstants::NotesListType m_whichNotes;
    const QString m_objectId;
};

class ListNotebooksThread : public QThread
{
    Q_OBJECT
public:
    ListNotebooksThread(StorageManager *storageManager, StorageConstants::NotebookTypes whichNotebooks, QObject *parent = 0);
    void run();
signals:
    void gotNotebooksList(int /*StorageConstants::NotebookTypes*/ notebookTypes, const QVariantList &notebooksList);
private:
    StorageManager * const m_storageManager;
    const StorageConstants::NotebookTypes m_whichNotebooks;
};

class ListTagsThread : public QThread
{
    Q_OBJECT
public:
    ListTagsThread(StorageManager *storageManager, QObject *parent = 0);
    void run();
signals:
    void gotTagsList(const QVariantList &tagsList);
private:
    StorageManager * const m_storageManager;
};

class ListTagsWithTagsOnNoteCheckedThread : public QThread
{
    Q_OBJECT
public:
    ListTagsWithTagsOnNoteCheckedThread(StorageManager *storageManager, const QString &noteId, QObject *parent = 0);
    void run();
signals:
    void gotTagsListWithTagsOnNoteChecked(const QVariantList &tagsList);
private:
    StorageManager * const m_storageManager;
    const QString m_noteId;
};

class GetNoteDataThread : public QThread
{
    Q_OBJECT
public:
    GetNoteDataThread(StorageManager *storageManager, EvernoteAccess *evernoteAccess, const QString &noteId, QObject *parent = 0);
    void run();
signals:
    void gotNoteTitle(const QString &noteTitle);
    void gotNoteData(const QVariantMap &noteData);
    void fetchNoteDataFinished(bool, QString);
    void authTokenInvalid();
private:
    StorageManager * const m_storageManager;
    EvernoteAccess * const m_evernoteAccess;
    const QString m_noteId;
};

class SearchServerNotesThread : public QThread
{
    Q_OBJECT
public:
    SearchServerNotesThread(StorageManager *storageManager, EvernoteAccess *evernoteAccess, const QString &words, QObject *parent = 0);
    void run();
signals:
    void searchServerNotesResultObtained(const QStringList &notesList, int progressPercentage);
    void searchServerNotesFinished();
    void authTokenInvalid();
private:
    StorageManager * const m_storageManager;
    EvernoteAccess * const m_evernoteAccess;
    const QString m_words;
};

class AttachmentDownloadThread : public QThread
{
    Q_OBJECT
public:
    AttachmentDownloadThread(StorageManager *storageManager, EvernoteAccess *evernoteAccess, const QString &urlStr, const QString &fileName, const QString &fileExtension, QObject *parent = 0);
    void run();
signals:
    void attachmentDownloaded(const QString &downloadedFilePath);
    void attachmentDownloadProgressed(qint64 bytesDownloaded);
    void authTokenInvalid();
private:
    StorageManager * const m_storageManager;
    EvernoteAccess * const m_evernoteAccess;
    const QString m_urlStr;
    const QString m_fileName;
    const QString m_fileExtension;
};

class AddAttachmentsThread : public QThread
{
    Q_OBJECT
public:
    AddAttachmentsThread(StorageManager *storageManager, const QString &noteId, const QVariantList &attachmentsToAdd, const QByteArray &enmlContent, const QByteArray &baseContentHash, QObject *parent = 0);
    void run();
public slots:
    void cancel();
signals:
    void addingAttachmentStatusChanged(const QVariantMap &result);
    void addedAttachment(const QVariantMap &result);
    void doneAddingAttachments(bool success, const QString &message, const QString &erroredFileName);
private slots:
    void resetAddingAttachmentStatus();
private:
    StorageManager * const m_storageManager;
    const QString m_noteId;
    QVariantList m_attachmentsToAdd;
    const QByteArray m_enmlContent;
    const QByteArray m_baseContentHash;
    QSharedPointer<ImageReadCancelHelper> m_imageResizeHelper;
    bool m_cancelled;
};

#endif // QMLDATAACCESS_H
