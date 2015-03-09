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

#ifndef EVERNOTEOAUTH_H
#define EVERNOTEOAUTH_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <QByteArray>
#include <QThread>
#include <QSslConfiguration>

class EvernoteAccess;
class StorageManager;
class ConnectionManager;

class EvernoteOAuth : public QObject
{
    Q_OBJECT
public:
    explicit EvernoteOAuth(StorageManager *storageManager, ConnectionManager *connectionManager, QObject *parent = 0);
    void setSslConfiguration(const QSslConfiguration &sslConf);

public slots:
    void startAuthentication();
    void continueAuthentication(const QByteArray &tempToken, const QByteArray &tempTokenSecret, const QByteArray &oauthVerifier);
    void endAuthentication();

private:
    void requestTempCredentials();
    void requestAuthToken(const QByteArray &tempToken, const QByteArray &tempTokenSecret, const QByteArray &oauthVerifier);
    void requestUsername(const QString &authToken);

private slots:
    void gotTempCredentials();
    void gotAuthToken();
    void gotUsername(const QString &username);
    void networkConnected();
    void networkDisconnected();

signals:
    void openOAuthUrl(const QString &urlString, const QString &callbackUrlString, const QByteArray &tempToken, const QByteArray &tempTokenSecret);
    void loggedIn();
    void error(const QString &message);

private:
    QNetworkAccessManager *m_qnam;
    StorageManager *m_storageManager;
    ConnectionManager *m_connectionManager;
    EvernoteAccess *m_evernoteAccess;
    QString m_authToken, m_noteStoreUrlString, m_webApiUrlPrefix;
    bool m_isRequestingTempToken;
};

class EvernoteGetUsernameThread : public QThread
{
    Q_OBJECT
public:
    EvernoteGetUsernameThread(EvernoteAccess *evernoteAccess, const QString &authToken, QObject *parent);
    void run();
signals:
    void gotUsername(const QString &username);
private:
    EvernoteAccess *m_evernoteAccess;
    QString m_authToken;
};

#endif // EVERNOTEOAUTH_H
