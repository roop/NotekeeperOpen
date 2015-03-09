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

#ifndef EVERNOTESYNC_H
#define EVERNOTESYNC_H

#include <QThread>
#include <QDateTime>
#include <QMutex>
#include <QTimer>

#include "evernoteaccess.h"

class StorageManager;
class ConnectionManager;

class EvernoteSync : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDateTime lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY isSyncingChanged)
    Q_PROPERTY(int syncProgress READ syncProgress NOTIFY syncProgressChanged)
    Q_PROPERTY(bool isFullSyncDone READ isFullSyncDone NOTIFY fullSyncDoneChanged)
    Q_PROPERTY(QString syncStatusMessage READ syncStatusMessage NOTIFY syncStatusMessageChanged)
    Q_PROPERTY(bool isRateLimitingInEffect READ isRateLimitingInEffect NOTIFY isRateLimitingInEffectChanged)
    Q_PROPERTY(QDateTime rateLimitEndTime READ rateLimitEndTime NOTIFY rateLimitEndTimeChanged)
public:
    EvernoteSync(StorageManager *storageManager, ConnectionManager *manager, QObject *parent = 0);

    void setSslConfiguration(const QSslConfiguration &sslConf);

public slots:
    void startSync();
    void cancel();
    QDateTime lastSyncTime() const;
    bool isSyncing() const;
    int syncProgress() const;
    bool isFullSyncDone() const;
    QString syncStatusMessage() const;
    bool isRateLimitingInEffect() const;
    QDateTime rateLimitEndTime() const;

private slots:
    void networkConnected();
    void networkDisconnected();
    void threadFinished();
    void updateSyncProgress(int progressPercentage);
    void updateSyncStatusMessage(const QString &message);
    void rateLimitReached(int secondsToResetLimit);
    void rateLimitReset();

signals:
    void syncStarted();
    void firstChunkDone();
    void loginError(QString message);
    void syncFinished(bool success, QString message);
    void lastSyncTimeChanged();
    void isSyncingChanged();
    void syncProgressChanged();
    void fullSyncDoneChanged();
    void syncStatusMessageChanged();
    void authTokenInvalid();
    void isRateLimitingInEffectChanged();
    void rateLimitEndTimeChanged();

private:
    void startSyncThreaded();
    StorageManager *m_storageManager;
    ConnectionManager *m_connectionManager;
    EvernoteAccess *m_evernoteAccess;
    bool m_isSyncInProgress, m_isThreadRunning;
    int m_syncProgress;
    QString m_syncStatusMessage;
    bool m_isRateLimitingInEffect;
    QDateTime m_rateLimitEndTime;
    QTimer m_rateLimitTimer;
};

class EvernoteSyncThread : public QThread
{
    Q_OBJECT
public:
    EvernoteSyncThread(EvernoteAccess *evernoteAccess, QObject *parent = 0);
    void run();
signals:
    void firstChunkDone();
    void loginError(QString message);
    void syncFinished(bool success, QString message);
    void syncProgressChanged(int progressPercentage);
    void syncStatusMessage(const QString &message);
    void fullSyncDoneChanged();
    void authTokenInvalid();
    void rateLimitReached(int secondsToResetLimit);
private:
    EvernoteAccess *m_evernoteAccess;
};

#endif // EVERNOTESYNC_H
