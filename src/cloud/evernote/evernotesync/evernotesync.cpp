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
#include "connectionmanager.h"
#include "evernotesync.h"

EvernoteSync::EvernoteSync(StorageManager *storageManager, ConnectionManager *connectionManager, QObject *parent)
    : QObject(parent)
    , m_storageManager(storageManager)
    , m_connectionManager(connectionManager)
#ifndef QT_SIMULATOR
    , m_evernoteAccess(new EvernoteAccess(storageManager, connectionManager, this))
#endif
    , m_isSyncInProgress(false)
    , m_isThreadRunning(false)
    , m_syncProgress(0)
    , m_isRateLimitingInEffect(false)
{
    connect(m_connectionManager, SIGNAL(connected()), SLOT(networkConnected()));
    connect(m_connectionManager, SIGNAL(disconnected()), SLOT(networkDisconnected()));
    connect(&m_rateLimitTimer, SIGNAL(timeout()), SLOT(rateLimitReset()));
}

void EvernoteSync::setSslConfiguration(const QSslConfiguration &sslConf)
{
#ifndef QT_SIMULATOR
    m_evernoteAccess->setSslConfiguration(sslConf);
#endif
}

void EvernoteSync::cancel()
{
#ifndef QT_SIMULATOR
    bool syncWasRunning = m_evernoteAccess->cancel();
    if (!syncWasRunning) {
        m_isSyncInProgress = false;
        emit isSyncingChanged();
    }
#endif
}

void EvernoteSync::startSync()
{
    m_storageManager->log("EvernoteSync::startSync()");
#ifndef QT_SIMULATOR
    if (!m_isSyncInProgress) {
        updateSyncStatusMessage("");
        m_isSyncInProgress = true;
        m_syncProgress = 0;
        emit isSyncingChanged();
        emit syncProgressChanged();
        bool alreadyConnected;
        bool requestOk = m_connectionManager->requestConnection(&alreadyConnected);
        if (alreadyConnected) {
            m_storageManager->log("startSync() Already connected");
        } else if (requestOk) {
            m_storageManager->log("startSync() Connection request succeeded");
        } else {
            m_storageManager->log("startSync() Connection request failed");
        }
        if (alreadyConnected) {
            networkConnected(); // else, this slot will get called when it does gets connected
        } else if (!requestOk) {
            m_isSyncInProgress = false;
            emit syncFinished(false, "Not connected to the internet");
        }
    }
#else
    m_isSyncInProgress = true;
    m_syncProgress = 0;
    emit isSyncingChanged();
    emit syncProgressChanged();
    networkConnected();
#endif
}

void EvernoteSync::networkConnected()
{
    m_storageManager->log(QString("EvernoteSync::networkConnected() sync_in_progress=%1 thread_running=%2").arg(m_isSyncInProgress).arg(m_isThreadRunning));
    if (m_isSyncInProgress && !m_isThreadRunning) {
        bool isSyncOverMobileDataAllowed = (m_storageManager->retrieveSetting("Sync/syncUsingMobileData") || (!m_storageManager->retrieveEvernoteSyncData("FullSyncDone").toBool()));
        if ((!m_connectionManager->isIapConfigurationWifi()) && (!isSyncOverMobileDataAllowed)) {
            m_isSyncInProgress = false;
            emit isSyncingChanged();
            emit syncFinished(false, "Syncing over mobile data is disabled");
            return;
        }
        startSyncThreaded();
        m_isThreadRunning = true;
    }
}

void EvernoteSync::networkDisconnected()
{
    m_storageManager->log(QString("EvernoteSync::networkDisconnected() sync_in_progress=%1 thread_running=%2").arg(m_isSyncInProgress).arg(m_isThreadRunning));
    if (m_isSyncInProgress) {
        cancel();
        emit syncFinished(false, "Not connected to the internet");
    }
}

void EvernoteSync::startSyncThreaded()
{
    m_storageManager->log("EvernoteSync::startSyncThreaded()");
    EvernoteSyncThread *enSyncThread = new EvernoteSyncThread(m_evernoteAccess, this);
    connect(enSyncThread, SIGNAL(finished()), SLOT(threadFinished()));
    connect(enSyncThread, SIGNAL(syncFinished(bool,QString)), SIGNAL(syncFinished(bool,QString)));
    connect(enSyncThread, SIGNAL(loginError(QString)), SIGNAL(loginError(QString)));
    connect(enSyncThread, SIGNAL(firstChunkDone()), SIGNAL(firstChunkDone()));
    connect(enSyncThread, SIGNAL(syncProgressChanged(int)), SLOT(updateSyncProgress(int)));
    connect(enSyncThread, SIGNAL(syncStatusMessage(QString)), SLOT(updateSyncStatusMessage(QString)));
    connect(enSyncThread, SIGNAL(fullSyncDoneChanged()), SIGNAL(fullSyncDoneChanged()));
    connect(enSyncThread, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));
    connect(enSyncThread, SIGNAL(rateLimitReached(int)), SLOT(rateLimitReached(int)));
    updateSyncStatusMessage("Starting sync");
    enSyncThread->start(QThread::LowPriority);
    emit syncStarted();
}

void EvernoteSync::threadFinished()
{
    m_storageManager->log("EvernoteSync::threadFinished()");
    updateSyncStatusMessage("");
    m_isSyncInProgress = false;
    m_isThreadRunning = false;
    emit isSyncingChanged();
    emit lastSyncTimeChanged();
}

void EvernoteSync::updateSyncProgress(int progressPercentage)
{
    if (m_syncProgress != progressPercentage) {
        m_syncProgress = progressPercentage;
        emit syncProgressChanged();
    }
}

void EvernoteSync::updateSyncStatusMessage(const QString &message)
{
    if (m_syncStatusMessage != message) {
        m_syncStatusMessage = message;
        emit syncStatusMessageChanged();
    }
}

void EvernoteSync::rateLimitReached(int secondsToResetLimit)
{
    m_rateLimitEndTime = QDateTime::currentDateTime().addSecs(secondsToResetLimit);
    m_rateLimitTimer.start(secondsToResetLimit * 1000 /* msecs */);
    m_isRateLimitingInEffect = true;
    emit rateLimitEndTimeChanged();
    emit isRateLimitingInEffectChanged();
}

void EvernoteSync::rateLimitReset()
{
    m_isRateLimitingInEffect = false;
    emit isRateLimitingInEffectChanged();
}

bool EvernoteSync::isRateLimitingInEffect() const
{
    return m_isRateLimitingInEffect;
}

QDateTime EvernoteSync::rateLimitEndTime() const
{
    return m_rateLimitEndTime;
}

bool EvernoteSync::isSyncing() const
{
    return m_isSyncInProgress;
}

QDateTime EvernoteSync::lastSyncTime() const
{
    qint64 msecsSinceEpoch = m_storageManager->retrieveEvernoteSyncData("LastSyncedTime").toLongLong();
    return QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch);
}

int EvernoteSync::syncProgress() const
{
    return m_syncProgress;
}

bool EvernoteSync::isFullSyncDone() const
{
    return m_storageManager->retrieveEvernoteSyncData("FullSyncDone").toBool();
}

QString EvernoteSync::syncStatusMessage() const
{
    return m_syncStatusMessage;
}

EvernoteSyncThread::EvernoteSyncThread(EvernoteAccess *evernoteAccess, QObject *parent)
    : QThread(parent), m_evernoteAccess(evernoteAccess)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void EvernoteSyncThread::run()
{
#ifndef QT_SIMULATOR
    connect(m_evernoteAccess, SIGNAL(finished(bool,QString)), SIGNAL(syncFinished(bool,QString)));
    connect(m_evernoteAccess, SIGNAL(loginError(QString)), SIGNAL(loginError(QString)));
    connect(m_evernoteAccess, SIGNAL(firstChunkDone()), SIGNAL(firstChunkDone()));
    connect(m_evernoteAccess, SIGNAL(syncProgressChanged(int)), SIGNAL(syncProgressChanged(int)));
    connect(m_evernoteAccess, SIGNAL(syncStatusMessage(QString)), SIGNAL(syncStatusMessage(QString)));
    connect(m_evernoteAccess, SIGNAL(fullSyncDoneChanged()), SIGNAL(fullSyncDoneChanged()));
    connect(m_evernoteAccess, SIGNAL(authTokenInvalid()), SIGNAL(authTokenInvalid()));
    connect(m_evernoteAccess, SIGNAL(rateLimitReached(int)), SIGNAL(rateLimitReached(int)));
    m_evernoteAccess->synchronize();
    m_evernoteAccess->disconnect(this);
#else
    emit syncProgressChanged(100);
    emit syncFinished(true, QLatin1String("Dummy sync"));
#endif
}
