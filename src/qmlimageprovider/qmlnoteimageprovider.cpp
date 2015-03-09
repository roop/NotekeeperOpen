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

#include "qmlnoteimageprovider.h"
#include "storagemanager.h"
#include "connectionmanager.h"
#include "storage/diskcache/shareddiskcache.h"
#include "notekeeper_config.h"
#include <QUrl>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCacheMetaData>
#include <QNetworkCookieJar>
#include <QEventLoop>
#include <QTimer>
#include <QtDebug>

QmlNoteImageProvider::QmlNoteImageProvider(StorageManager *storageManager, ConnectionManager *connectionManager, const QSslConfiguration &sslConf)
    : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
    , m_storageManager(storageManager)
    , m_connectionManager(connectionManager)
    , m_sslConfg(sslConf)
{
}

QImage QmlNoteImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QUrl imageUrl(id);
    QImageReader imageReader;
    QString localFileName = imageUrl.toLocalFile();
    QIODevice *cachedData = 0;
    if (!localFileName.isEmpty()) {
        // It's a local file
        imageReader.setFileName(localFileName);
    } else {
        // It's a web URL
        cachedData = SharedDiskCache::instance()->data(imageUrl);
        if (cachedData) {
            imageReader.setDevice(cachedData);
        }
        if (!imageReader.size().isValid()) {
            // Cache does not have valid data for this URL
            // So let's fetch it and insert it into the cache
            if (cachedData) {
                // Cache has data, but the data is invalid
                SharedDiskCache::instance()->remove(imageUrl);
            }
            QNetworkCacheMetaData cacheMetaData;
            cacheMetaData.setUrl(imageUrl);
            QIODevice *targetDevice = SharedDiskCache::instance()->prepare(cacheMetaData);
            bool ok = fetchFromEvernoteWeb(imageUrl, targetDevice);
            if (!ok) {
                return QImage();
            }
            SharedDiskCache::instance()->insert(targetDevice);
            cachedData = SharedDiskCache::instance()->data(imageUrl);
            imageReader.setDevice(cachedData);
        }
    }
    QSize imageSize = imageReader.size();
    QSize scaledSize = imageSize;
    if (imageSize.width() > requestedSize.width() || imageSize.height() > requestedSize.height()) {
        scaledSize.scale(requestedSize, Qt::KeepAspectRatio);
    }
    (*size) = scaledSize;
    imageReader.setScaledSize(scaledSize);
    QImage img = imageReader.read();
    delete cachedData;
    return img;
}

bool QmlNoteImageProvider::fetchFromEvernoteWeb(const QUrl &url, QIODevice* targetDevice)
{
    QString authToken = m_storageManager->retrieveEncryptedEvernoteAuthData("authToken");
    QNetworkAccessManager nwAccessManager;
    nwAccessManager.setConfiguration(m_connectionManager->selectedConfiguration());

    QUrl genericUrl(QString("https://%1").arg(EVERNOTE_API_HOST));
    QNetworkCookie cookie("auth", authToken.toLatin1());
    cookie.setDomain(genericUrl.host());
    if (genericUrl.scheme() == "https") {
        cookie.setSecure(true);
    }
    QNetworkCookieJar *cookieJar = new QNetworkCookieJar;
    cookieJar->setCookiesFromUrl(QList<QNetworkCookie>() << cookie, genericUrl);
    nwAccessManager.setCookieJar(cookieJar);

    if (!targetDevice->isOpen()) {
        targetDevice->open(QIODevice::WriteOnly);
    }

    QNetworkRequest request(url);
    request.setSslConfiguration(m_sslConfg);

    QNetworkReply *reply = nwAccessManager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(60 * 1000); // 1 min timeout
    timer.setSingleShot(true);
    QObject::connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    qint64 bytesDownloaded = 0;
    do {
        timer.start();
        loop.exec(); // blocking wait
        if (!timer.isActive()) { // timed out
            return false;
        }
        while (reply->bytesAvailable() > 0) {
            QByteArray data = reply->read(1 << 18); // max 256 kb
            bytesDownloaded += targetDevice->write(data);
        }
    } while(!reply->isFinished());

    if (reply->bytesAvailable() > 0) {
        targetDevice->write(reply->readAll());
    }
    reply->deleteLater();
    return true;
}
