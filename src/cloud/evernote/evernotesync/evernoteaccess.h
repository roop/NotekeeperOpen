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

#ifndef EVERNOTEACCESS_H
#define EVERNOTEACCESS_H

#include <QObject>
#include <QSslConfiguration>
#include <QNetworkConfiguration>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QtNetwork/QSslError>

class StorageManager;
class ConnectionManager;
class QThriftHttpClient;

namespace evernote {
    namespace edam {
        class UserStoreClient;
        class NoteStoreClient;
        class User;
        class AuthenticationResult;
        class SyncState;
        class SyncChunk;
        class Note;
        class NoteList;
        class NoteFilter;
        class Notebook;
        class Tag;
    }
}
using namespace evernote;

// EvernoteAccess: A synchronous interface between the local storage
// of notes and the cloud storage of Evernote.
// This class is re-entrant.

class EvernoteAccess : public QObject
{
    Q_OBJECT
public:
    EvernoteAccess(StorageManager *storageManager, ConnectionManager *manager, QObject *parent = 0);
    void setSslConfiguration(const QSslConfiguration &sslConf);

    bool fetchNote(const QString &noteId, QVariantMap *noteData = 0);
    bool fetchNoteThumbnail(const QString &noteId, const QString &noteGuid, QNetworkAccessManager *nwAccessManager);
    bool fetchOfflineNoteAttachment(const QString &noteId, const QString &attachmentGuid, const QByteArray &attachmentHash, QNetworkAccessManager *nwAccessManager);
    bool fetchEvernoteWebResource(const QUrl &url, QNetworkAccessManager *nwAccessManager, QTemporaryFile* webResourceFile, bool emitProgress = false);

    void synchronize();

    QStringList searchNotes(const QString &words, const QString &notebookId = QString(), const QString &tagId = QString()); // returns note ids

    bool cancel();

    QString fetchUserInfo(const QString &authToken); // finds out the user info for this AuthToken, saves it, returns the username

    bool isSslErrorFound() const;

signals:
    void authTokenInvalid();
    void firstChunkDone();
    void loginError(const QString &message);
    void finished(bool success, const QString &message);
    void cancelled();
    void sslErrorsFound();
    void syncProgressChanged(int progressPercentage);
    void syncStatusMessage(const QString &message);
    void fullSyncDoneChanged();
    void rateLimitReached(int secondsToResetLimit);
    void webResourceFetchProgressed(qint64 bytesDownloaded);
    void serverSearchMatchingNotes(const QStringList &notesList, int progressPercentage);

public slots:
    void handleSslErrors(const QList<QSslError> &errors);

private:
    enum NotePushResult {
        NotePushSuccess,
        NotePushCancelled,
        NotePushTimedOut,
        NotePushLoginError,
        NotePushUnknownError,
        NotePushQuotaReachedError,
        NotePushRateLimitReachedError
    };

    bool fetchNote(edam::NoteStoreClient *noteStore, const QString &noteId, QNetworkAccessManager *nwAccessManager, QVariantMap *noteData = 0);
    bool updateNoteThumbnail(const QString &noteId, const QString &noteGuid, bool hasImages, bool imagesRemoved, QNetworkAccessManager *nwAccessManager);
    bool fetchNoteAndAttachmentsForOfflineAccess(edam::NoteStoreClient *noteStore, const QString &noteId, QNetworkAccessManager *nwAccessManager);

    // thin-wrappers around EDAM function calls
    bool edamCheckVersion(edam::UserStoreClient *userStore);
    bool edamGetUser(edam::UserStoreClient *userStore,
                     edam::User *user,
                     const QString &authToken);
    bool edamAuthenticate(edam::UserStoreClient *userStore,
                          edam::AuthenticationResult *authenticationResult,
                          const QString &username, const QString &password,
                          const std::string &consumerSecret, const std::string &consumerKey);
    bool edamGetSyncState(edam::NoteStoreClient *noteStore,
                          edam::SyncState *syncState, const QString &authToken);
    bool edamGetSyncChunk(edam::NoteStoreClient *noteStore,
                          edam::SyncChunk *syncChunk,
                          const QString &authToken, int usn, int maxEntries, bool fullSync);
    bool edamGetNote     (edam::NoteStoreClient *noteStore,
                          edam::Note *note,
                          const QString &authToken, const QString &guid,
                          bool withContent, bool withResources,
                          bool withResourcesRecognition, bool withResourcesAlternateData);

    NotePushResult edamCreateNote  (edam::NoteStoreClient *noteStore,
                          edam::Note *note,
                          const QString &authToken, const edam::Note &noteData);
    NotePushResult edamUpdateNote  (edam::NoteStoreClient *noteStore,
                          edam::Note *note,
                          const QString &authToken, const edam::Note &noteData);

    bool edamFindNotes   (edam::NoteStoreClient *noteStore,
                          edam::NoteList *noteList,
                          const QString &authToken, const edam::NoteFilter &noteFilter, qint32 offset, qint32 maxNotes);
    bool edamCreateNotebook(edam::NoteStoreClient *noteStore,
                          edam::Notebook *notebook,
                          const QString &authToken, const edam::Notebook &notebookData);
    bool edamUpdateNotebook(edam::NoteStoreClient *noteStore,
                            qint32 *usn,
                            const QString &authToken, const edam::Notebook &notebookData);
    bool edamCreateTag   (edam::NoteStoreClient *noteStore,
                          edam::Tag *tag,
                          const QString &authToken, const edam::Tag &tagData);
    bool edamUpdateTag(edam::NoteStoreClient *noteStore,
                          qint32 *usn,
                          const QString &authToken, const edam::Tag &tagData);

    StorageManager *m_storageManager;
    ConnectionManager *m_connectionManager;
    QSslConfiguration m_sslConfig;
    QThriftHttpClient *m_userStoreHttpClient, *m_noteStoreHttpClient;
    bool m_cancelled;
    bool m_isFetchingWebResource;
    bool m_isSslErrorFound;
};

#endif // EVERNOTEACCESS_H
