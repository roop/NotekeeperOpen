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

#include "evernoteoauth.h"
#include "evernoteaccess.h"
#include "storagemanager.h"
#include "connectionmanager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDateTime>
#include <QStringBuilder>
#include <stdlib.h>
#include <QCryptographicHash>

#include "notekeeper_config.h"

#define CALLBACK_URL "http://www.notekeeperapp.com/support"

// #define OAUTH_DEBUG

#ifdef OAUTH_DEBUG
#include <QDebug>
#endif

EvernoteOAuth::EvernoteOAuth(StorageManager *storageManager, ConnectionManager *connectionManager, QObject *parent)
    : QObject(parent)
    , m_qnam(0)
    , m_storageManager(storageManager)
    , m_connectionManager(connectionManager)
#ifndef QT_SIMULATOR
    , m_evernoteAccess(new EvernoteAccess(storageManager, connectionManager, this))
#endif
    , m_isRequestingTempToken(false)
{
    connect(m_connectionManager, SIGNAL(connected()), SLOT(networkConnected()));
    connect(m_connectionManager, SIGNAL(disconnected()), SLOT(networkDisconnected()));
}

void EvernoteOAuth::setSslConfiguration(const QSslConfiguration &sslConf)
{
#ifndef QT_SIMULATOR
    m_evernoteAccess->setSslConfiguration(sslConf);
#endif
}

void EvernoteOAuth::startAuthentication()
{
    m_storageManager->log("EvernoteOAuth::startAuthentication()");
#ifndef QT_SIMULATOR
    if (!m_isRequestingTempToken) {
        m_isRequestingTempToken = true;
        bool alreadyConnected;
        bool requestOk = m_connectionManager->requestConnection(&alreadyConnected);
        if (alreadyConnected) {
            m_storageManager->log("startAuthentication() Already connected");
        } else if (requestOk) {
            m_storageManager->log("startAuthentication() Connection request succeeded");
        } else {
            m_storageManager->log("startAuthentication() Connection request failed");
        }
        if (alreadyConnected) {
            networkConnected(); // else, this slot will get called when it does gets connected
        } else if (!requestOk) {
            m_isRequestingTempToken = false;
            emit error("Not connected to the internet");
        }
    }
#else
    m_isRequestingTempToken = true;
    networkConnected();
#endif
}

void EvernoteOAuth::networkConnected()
{
    m_storageManager->log(QString("EvernoteOAuth::networkConnected() m_isRequestingTempToken=%1").arg(m_isRequestingTempToken));
    if (m_isRequestingTempToken) {
        qsrand(QDateTime::currentMSecsSinceEpoch() % RAND_MAX);
        if (m_qnam == 0) {
            m_qnam = new QNetworkAccessManager(this);
        }
        requestTempCredentials();
    }
}

void EvernoteOAuth::networkDisconnected()
{
    m_storageManager->log(QString("EvernoteOAuth::networkDisconnected() m_isRequestingTempToken=%1").arg(m_isRequestingTempToken));
    if (m_isRequestingTempToken) {
        emit error("Not connected to the internet");
    }
}

static QByteArray hmacSha1(const QByteArray &_key, const QByteArray &baseString)
{
    // Derived from http://qt-project.org/wiki/HMAC-SHA1
    QByteArray key = _key;
    const int blockSize = 64;
    if (key.length() > blockSize) {
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    }
    QByteArray innerPadding(blockSize, char(0x36));
    QByteArray outerPadding(blockSize, char(0x5c));
    for (int i = 0; i < key.length(); i++) {
        innerPadding[i] = innerPadding[i] ^ key.at(i);
        outerPadding[i] = outerPadding[i] ^ key.at(i);
    }
    QByteArray total = outerPadding;
    QByteArray part = innerPadding;
    part.append(baseString);
    total.append(QCryptographicHash::hash(part, QCryptographicHash::Sha1));
    QByteArray hashed = QCryptographicHash::hash(total, QCryptographicHash::Sha1);
    QByteArray base64encoded = hashed.toBase64();
    return base64encoded;
}

static QByteArray hmacSignature(const QByteArray &key, const QByteArray &requestMethod, const QString &urlBase, const QMap<QByteArray, QString> &params)
{
    QByteArray baseString = requestMethod.toUpper() + "&" + urlBase.toLower().toUtf8().toPercentEncoding() + "&";
    QByteArray parametersString("");
    QMapIterator<QByteArray, QString> paramsIter(params);
    while (paramsIter.hasNext()) {
        paramsIter.next();
        parametersString += paramsIter.key().toPercentEncoding() + "=" + paramsIter.value().toUtf8().toPercentEncoding();
        if (paramsIter.hasNext()) {
            parametersString += "&";
        }
    }
    baseString += parametersString.toPercentEncoding();
#ifdef OAUTH_DEBUG
    qDebug() << "HMACSHA1 Key:" << key;
    qDebug() << "HMACSHA1 Base string: " << baseString;
#endif
    return hmacSha1(key, baseString);
}

static QByteArray authHeaderValue(const QMap<QByteArray, QString> &params, const QByteArray &tokenSecret)
{
    QByteArray headerValue("");
    QMapIterator<QByteArray, QString> paramsIter(params);
    while (paramsIter.hasNext()) {
        paramsIter.next();
        headerValue += paramsIter.key().toPercentEncoding() + "=\"" + paramsIter.value().toUtf8().toPercentEncoding() + "\"";
        headerValue += ", ";
    }
    QByteArray signingKey = QByteArray(EVERNOTE_OAUTH_CONSUMER_SECRET).toPercentEncoding() + "&" + tokenSecret.toPercentEncoding();
    QByteArray signature = hmacSignature(signingKey, "GET", QString("https://%1/oauth").arg(EVERNOTE_API_HOST), params);
    headerValue += QByteArray("oauth_signature=\"") + signature.toPercentEncoding() + "\"";
    return headerValue;
}

void EvernoteOAuth::requestTempCredentials()
{
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString nonce = timestamp % "-" % QString::number(qrand(), 16);

    QMap<QByteArray, QString> params;
    params.insert("oauth_consumer_key", EVERNOTE_OAUTH_CONSUMER_KEY);
    params.insert("oauth_timestamp", timestamp);
    params.insert("oauth_nonce", nonce);
    params.insert("oauth_signature_method", "HMAC-SHA1");
    params.insert("oauth_callback", CALLBACK_URL);
    QByteArray authHeader = authHeaderValue(params, "");

#ifdef OAUTH_DEBUG
    qDebug() << "authHeader = >" + authHeader + "<";
#endif
    QNetworkRequest request(QUrl(QString("https://%1/oauth").arg(EVERNOTE_API_HOST)));
    request.setRawHeader("Authorization", authHeader);
    QNetworkReply *reply = m_qnam->get(request);
    connect(reply, SIGNAL(finished()), SLOT(gotTempCredentials()));
}

static QMap<QByteArray, QByteArray> replyTextToMap(const QByteArray &replyText)
{
    QMap<QByteArray, QByteArray> replyData;
    foreach (const QByteArray &replyComponent, replyText.split('&')) {
        QList<QByteArray> replyComponentParts = replyComponent.split('=');
        if (replyComponentParts.length() == 2) {
            replyData.insert(replyComponentParts.at(0), QByteArray::fromPercentEncoding(replyComponentParts.at(1)));
        }
    }
    return replyData;
}

void EvernoteOAuth::gotTempCredentials()
{
    m_isRequestingTempToken = false;
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    QByteArray replyText = reply->readAll();
    reply->deleteLater();

#ifdef OAUTH_DEBUG
    qDebug() << "reply: " << replyText;
#endif

    QMap<QByteArray, QByteArray> replyData = replyTextToMap(replyText);
    QByteArray tempToken = replyData.value("oauth_token");
    bool isCallbackConfirmed = (replyData.value("oauth_callback_confirmed").toLower() == "true");
    QByteArray tempTokenSecret = replyData.value("oauth_token_secret");
    if (tempToken.isEmpty() || !isCallbackConfirmed) {
        if (tempToken.isEmpty()) { qDebug() << "EvernoteOAuth: Empty temp token"; }
        if (!isCallbackConfirmed) { qDebug() << "EvernoteOAuth: Callback was not confirmed"; }
        emit error("Could not contact Evernote");
        return;
    }

    QString urlString(QString("https://%1/OAuth.action?oauth_token=%2").arg(EVERNOTE_API_HOST).arg(QString(tempToken)));
    emit openOAuthUrl(urlString, CALLBACK_URL, tempToken, tempTokenSecret);
}

void EvernoteOAuth::continueAuthentication(const QByteArray &tempToken, const QByteArray &tempTokenSecret, const QByteArray &oauthVerifier)
{
    requestAuthToken(tempToken, tempTokenSecret, oauthVerifier);
}

void EvernoteOAuth::requestAuthToken(const QByteArray &tempToken, const QByteArray &tempTokenSecret, const QByteArray &oauthVerifier)
{
    if (oauthVerifier.isEmpty()) {
        emit error("Notekeeper was declined access to your notes");
        return;
    }
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString nonce = timestamp % "-" % QString::number(qrand(), 16);

    QMap<QByteArray, QString> params;
    params.insert("oauth_consumer_key", EVERNOTE_OAUTH_CONSUMER_KEY);
    params.insert("oauth_timestamp", timestamp);
    params.insert("oauth_nonce", nonce);
    params.insert("oauth_signature_method", "HMAC-SHA1");
    params.insert("oauth_token", tempToken);
    params.insert("oauth_verifier", oauthVerifier);
    QByteArray authHeader = authHeaderValue(params, tempTokenSecret);

#ifdef OAUTH_DEBUG
    qDebug() << "authHeader = >" + authHeader + "<";
#endif
    QNetworkRequest request(QUrl(QString("https://%1/oauth").arg(EVERNOTE_API_HOST)));
    request.setRawHeader("Authorization", authHeader);
    QNetworkReply *reply = m_qnam->get(request);
    connect(reply, SIGNAL(finished()), SLOT(gotAuthToken()));
}

void EvernoteOAuth::gotAuthToken()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    QByteArray replyText = reply->readAll();
    reply->deleteLater();
    QMap<QByteArray, QByteArray> replyData = replyTextToMap(replyText);

#ifdef OAUTH_DEBUG
    qDebug() << "reply: " << replyText;
    qDebug() << "Auth token: " << replyData.value("oauth_token");
    qDebug() << "NoteStore url: " << replyData.value("edam_noteStoreUrl");
    qDebug() << "UserId: " << replyData.value("edam_userId");
#endif

    m_authToken = QString(replyData.value("oauth_token"));
    m_noteStoreUrlString = QString(replyData.value("edam_noteStoreUrl"));
    m_webApiUrlPrefix = QString(replyData.value("edam_webApiUrlPrefix"));
    if (m_authToken.isEmpty() || m_noteStoreUrlString.isEmpty()) {
        if (m_authToken.isEmpty()) { qDebug() << "EvernoteOAuth: Empty auth token"; }
        if (m_noteStoreUrlString.isEmpty()) { qDebug() << "EvernoteOAuth: Empty noteStoreUrl"; }
        emit error("Could not contact Evernote");
        return;
    }
    requestUsername(m_authToken);
}

void EvernoteOAuth::requestUsername(const QString &authToken)
{
    EvernoteGetUsernameThread *getUsernameThread = new EvernoteGetUsernameThread(m_evernoteAccess, authToken, this);
    connect(getUsernameThread, SIGNAL(gotUsername(QString)), SLOT(gotUsername(QString)));
    getUsernameThread->start();
}

void EvernoteOAuth::gotUsername(const QString &username)
{
#ifndef QT_SIMULATOR
    if (m_evernoteAccess->isSslErrorFound()) {
        emit error("There were SSL errors when talking to Evernote");
        return;
    }
#endif
    if (username.isEmpty() || m_authToken.isEmpty() || m_noteStoreUrlString.isEmpty()) {
        emit error("Invalid user");
        return;
    }
    m_storageManager->setActiveUser(username);
    m_storageManager->saveEvernoteAuthData("username", username);
    m_storageManager->saveEncryptedEvernoteAuthData("authToken", m_authToken);
    m_storageManager->saveEvernoteAuthData("noteStoreUrl", m_noteStoreUrlString);
    m_storageManager->saveEvernoteAuthData("webApiUrlPrefix", m_webApiUrlPrefix);

    if (!m_storageManager->retrieveEncryptedEvernoteAuthData("password").isEmpty()) {
        m_storageManager->saveEncryptedEvernoteAuthData("password", ""); // clear any password stored by v1.x
    }
    m_storageManager->saveEvernoteAuthData("authenticationMethod", "OAuth"); // not used now, but just in case

    emit loggedIn();
}

void EvernoteOAuth::endAuthentication()
{
    m_qnam->deleteLater();
    m_qnam = 0;
}

EvernoteGetUsernameThread::EvernoteGetUsernameThread(EvernoteAccess *evernoteAccess, const QString &authToken, QObject *parent)
    : QThread(parent)
    , m_evernoteAccess(evernoteAccess)
    , m_authToken(authToken)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void EvernoteGetUsernameThread::run()
{
    QString username("");
#ifndef QT_SIMULATOR
    username = m_evernoteAccess->fetchUserInfo(m_authToken);
#else
    username = "roop";
#endif
    emit gotUsername(username);
}
