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

#include "shareddiskcache.h"
#include "qplatformdefs.h"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QtDebug>
#include <QDesktopServices>
#include <QStringBuilder>
#include <QDateTime>
#include <QNetworkDiskCache>

// singleton

SharedDiskCache* SharedDiskCache::s_instance = 0;
qint64 SharedDiskCache::s_diskCacheSize = 20 << 20; // default 20 MB

SharedDiskCache* SharedDiskCache::instance()
{
    if (s_instance == 0) {
        s_instance = new SharedDiskCache;
    }
    return s_instance;
}

void SharedDiskCache::setSharedDiskCacheSize(qint64 size)
{
    if (s_instance) {
        s_instance->setMaximumCacheSize(size);
    }
    s_diskCacheSize = size;
}

SharedDiskCache::SharedDiskCache() :
    QAbstractNetworkCache(0)
{
    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);

    QString appExecName = "notekeeper-open";

#ifdef MEEGO_EDITION_HARMATTAN
    QString cacheDirectory = QDesktopServices::storageLocation(QDesktopServices::CacheLocation) % "/" % appExecName % "/Cache";
#else
    QString cacheDirectory = QDir::currentPath() % "/Store/Cache";
#endif
    diskCache->setCacheDirectory(cacheDirectory);
    diskCache->setMaximumCacheSize(SharedDiskCache::s_diskCacheSize);
    m_diskCache = diskCache;
}

void SharedDiskCache::setMaximumCacheSize(qint64 capacity)
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    QNetworkDiskCache *diskCache = qobject_cast<QNetworkDiskCache*>(m_diskCache);
    if (diskCache) {
        diskCache->setMaximumCacheSize(capacity);
    }
}

qint64 SharedDiskCache::maximumCacheSize() const
{
    QMutexLocker mutexLocker(const_cast<QMutex*>(&m_mutex));
    Q_UNUSED(mutexLocker);
    QNetworkDiskCache *diskCache = qobject_cast<QNetworkDiskCache*>(m_diskCache);
    if (diskCache) {
        return diskCache->maximumCacheSize();
    }
    return 0;
}

// the network cache interface

qint64 SharedDiskCache::cacheSize() const
{
    QMutexLocker mutexLocker(const_cast<QMutex*>(&m_mutex));
    Q_UNUSED(mutexLocker);
    return m_diskCache->cacheSize();
}

QIODevice *SharedDiskCache::data(const QUrl &url)
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    return m_diskCache->data(url);
}

void SharedDiskCache::insert(QIODevice *device)
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    m_diskCache->insert(device);
}

QNetworkCacheMetaData SharedDiskCache::metaData(const QUrl &url)
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    return m_diskCache->metaData(url);
}

QIODevice *SharedDiskCache::prepare(const QNetworkCacheMetaData &_metaData)
{
    QNetworkCacheMetaData metaData = _metaData;
    if (metaData.expirationDate().isValid()) {
        // unset expiry date
        metaData.setExpirationDate(QDateTime());
    }
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    return m_diskCache->prepare(metaData);
}

bool SharedDiskCache::remove(const QUrl &url)
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    return m_diskCache->remove(url);
}

void SharedDiskCache::updateMetaData(const QNetworkCacheMetaData &metaData)
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    m_diskCache->updateMetaData(metaData);
}

void SharedDiskCache::clear()
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    m_diskCache->clear();
}

// for note contents

bool SharedDiskCache::insertNoteContent(const QString &guid, const QByteArray &data, const QByteArray &md5)
{
    QByteArray md5hash = md5;
    if (md5hash.isEmpty()) {
        md5hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    }

    QUrl url("notecontent://evernote/noteguid/" + guid);
    QNetworkCacheMetaData meta;
    meta.setUrl(url);
    QNetworkCacheMetaData::AttributesMap attributes = meta.attributes();
    attributes[static_cast<QNetworkRequest::Attribute>(NoteContentMd5Hash)] = md5hash;
    meta.setAttributes(attributes);

    QIODevice *cacheSaveDevice = prepare(meta);
    if (!cacheSaveDevice || (cacheSaveDevice && !cacheSaveDevice->isOpen())) {
        remove(url);
        return false;
    }
    cacheSaveDevice->write(data);
    insert(cacheSaveDevice); // the cache owns the QIODevice* now
    return true;
}

bool SharedDiskCache::containsNoteContent(const QString &guid, const QByteArray &md5hash)
{
    QUrl url("notecontent://evernote/noteguid/" + guid);
    QNetworkCacheMetaData meta = metaData(url);
    if (!meta.isValid()) {
        return false;
    }
    if (md5hash.isEmpty()) {
        return true;
    }
    QByteArray cachedMd5Hash = meta.attributes().value(static_cast<QNetworkRequest::Attribute>(NoteContentMd5Hash)).toByteArray();
    return (md5hash == cachedMd5Hash);
}

bool SharedDiskCache::retrieveNoteContent(const QString &guid, QByteArray *content, QByteArray *md5)
{
    QUrl url("notecontent://evernote/noteguid/" + guid);
    QNetworkCacheMetaData meta = metaData(url);
    if (!meta.isValid()) {
        return false;
    }
    if (md5) {
        (*md5) = meta.attributes().value(static_cast<QNetworkRequest::Attribute>(NoteContentMd5Hash)).toByteArray();
    }
    QScopedPointer<QIODevice> cacheAccessDevice(data(url));
    if (cacheAccessDevice.data() == 0 || (cacheAccessDevice.data() != 0 && !cacheAccessDevice->isOpen())) {
        remove(url);
        return false;
    }
    if (content) {
        (*content) = cacheAccessDevice->readAll();
    }
    return true;
}

bool SharedDiskCache::removeNoteContent(const QString &guid)
{
    QUrl url("notecontent://evernote/noteguid/" + guid);
    return remove(url);
}

static qint64 copyData(QIODevice *src, QIODevice *dst)
{
    QByteArray buffer(2 << 15 /*64 kb*/, '\0');
    qint64 bytesCopied = 0;
    qint64 bytesRead = src->read(buffer.data(), buffer.size());
    while (bytesRead > 0) {
        qint64 bytesWritten = dst->write(buffer.constData(), bytesRead);
        bytesCopied += bytesWritten;
        if (bytesWritten != bytesRead) {
            return bytesCopied;
        }
        bytesRead = src->read(buffer.data(), buffer.size());
    }
    return bytesCopied;
}

bool SharedDiskCache::insertEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid, QFile *source, qint64 size)
{
    QUrl attachmentUrl = QUrl(QString("%1res/%2").arg(webApiUrlPrefix).arg(guid));

    QNetworkCacheMetaData meta;
    meta.setUrl(attachmentUrl);

    QIODevice *cacheSaveDevice = prepare(meta);
    if (!cacheSaveDevice || (cacheSaveDevice && !cacheSaveDevice->isOpen())) {
        remove(attachmentUrl);
        return false;
    }

    if (!source->isOpen()) {
        if (!source->open(QIODevice::ReadOnly)) {
            return false;
        }
    }
    Q_ASSERT(source->isReadable());

    qint64 bytesCopied = copyData(source, cacheSaveDevice);
    source->close();
    if (bytesCopied != size) {
        return false;
    }

    insert(cacheSaveDevice); // the cache owns the QIODevice* now
    return true;
}

bool SharedDiskCache::containsEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid)
{
    QUrl attachmentUrl = QUrl(QString("%1res/%2").arg(webApiUrlPrefix).arg(guid));
    QNetworkCacheMetaData meta = metaData(attachmentUrl);
    return meta.isValid();
}

bool SharedDiskCache::retrieveEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid, QFile *target, qint64 size)
{
    QUrl attachmentUrl = QUrl(QString("%1res/%2").arg(webApiUrlPrefix).arg(guid));

    QNetworkCacheMetaData meta = metaData(attachmentUrl);
    if (!meta.isValid()) {
        return false;
    }

    QIODevice *cacheAccessDevice = data(attachmentUrl);
    if (cacheAccessDevice == 0 || (cacheAccessDevice != 0 && !cacheAccessDevice->isOpen())) {
        remove(attachmentUrl);
        return false;
    }

    if (!target->isOpen()) {
        if (!target->open(QFile::WriteOnly)) {
            return false;
        }
    }
    Q_ASSERT(target->isWritable());

    qint64 bytesCopied = copyData(cacheAccessDevice, target);
    target->close();
    delete cacheAccessDevice;

    return (bytesCopied == size);
}

bool SharedDiskCache::removeEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid)
{
    QUrl attachmentUrl = QUrl(QString("%1res/%2").arg(webApiUrlPrefix).arg(guid));
    return remove(attachmentUrl);
}
