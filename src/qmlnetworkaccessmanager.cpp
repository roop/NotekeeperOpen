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

#include "qmlnetworkaccessmanager.h"
#include "storage/storagemanager.h"
#include "qmldataaccess.h"
#include "evernoteoauth.h"
#include "qmlnetworkdiskcache.h"
#include "qmlnetworkcookiejar.h"
#include "notekeeper_config.h"
#include "loginstatustracker.h"
#include "diskcache/shareddiskcache.h"
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QNetworkDiskCache>

QmlNetworkAccessManager::QmlNetworkAccessManager(StorageManager *storageManager, LoginStatusTracker *loginStatusTracker, QObject *parent)
    : QNetworkAccessManager(parent)
    , m_storageManager(storageManager)
{
    setCacheAndCookieJarEnabled(loginStatusTracker->isLoggedIn());
    connect(loginStatusTracker, SIGNAL(isLoggedInChanged(bool)), SLOT(setCacheAndCookieJarEnabled(bool)));
}

void QmlNetworkAccessManager::setCacheAndCookieJarEnabled(bool enabled)
{
    if (m_isCacheAndCookieJarEnabled == enabled) {
        return;
    }

    if (enabled) {
        // ensure auth token is included in the request (while GETting images, for eg.)
        QString authToken = m_storageManager->retrieveEncryptedEvernoteAuthData("authToken");
        QUrl url;
        url.setScheme("https");
        url.setHost(EVERNOTE_API_HOST);
        QNetworkCookie cookie("auth", authToken.toLatin1());
        cookie.setDomain(EVERNOTE_API_HOST);
        cookie.setSecure(true);

        // set cookie jar
        QmlNetworkCookieJar *cookieJar = QmlNetworkCookieJar::instance();
        cookieJar->setCookiesFromUrl(QList<QNetworkCookie>() << cookie, url);
        setCookieJar(cookieJar);
        cookieJar->setParent(0); // it's a shared cookie jar

        // set disk cache
        setCache(new QmlNetworkDiskCache); // nwAccessManager takes ownership

        m_isCacheAndCookieJarEnabled = true;

    } else {

        setCookieJar(new QNetworkCookieJar); // a private non-shared cookie jar
        setCache(0); // no caching

        m_isCacheAndCookieJarEnabled = false;
    }
}

QNetworkReply *QmlNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &_req, QIODevice *outgoingData)
{
    QNetworkRequest req(_req);
    if (SharedDiskCache::instance()->metaData(req.url()).isValid()) {
        // if available in the cache, force our qnam to use only the cache, and not access the network at all
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
        req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
    } else {
        req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    }
    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}
