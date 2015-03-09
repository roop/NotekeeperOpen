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

#ifndef SHAREDDISKCACHE_H
#define SHAREDDISKCACHE_H

#include <QAbstractNetworkCache>
#include <QMutex>
#include <QFile>

// A disk cache to store note contents and note attachments
// Thread-safe, Singleton

class SharedDiskCache : public QAbstractNetworkCache
{
    Q_OBJECT
public:
    // singleton
    static SharedDiskCache *instance();
    static void setSharedDiskCacheSize(qint64 size);

    // the network cache interface
    virtual qint64 cacheSize() const;
    virtual QIODevice *data(const QUrl &url);
    virtual void insert(QIODevice *device);
    virtual QNetworkCacheMetaData metaData(const QUrl &url);
    virtual QIODevice *prepare(const QNetworkCacheMetaData &metaData);
    virtual bool remove(const QUrl &url);
    virtual void updateMetaData(const QNetworkCacheMetaData &metaData);

    // get/set cache size like in QNetworkDiskCache
    void setMaximumCacheSize(qint64 capacity);
    qint64 maximumCacheSize() const;

public slots:
    virtual void clear();

public:
    // for note contents
    bool insertNoteContent(const QString &guid, const QByteArray &data, const QByteArray &md5 = QByteArray());
    bool containsNoteContent(const QString &guid, const QByteArray &md5hash = QByteArray());
    bool retrieveNoteContent(const QString &guid, QByteArray *contents, QByteArray *md5);
    bool removeNoteContent(const QString &guid);

    // for note attachments
    bool insertEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid, QFile *source, qint64 size);
    bool containsEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid);
    bool retrieveEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid, QFile *target, qint64 size);
    bool removeEvernoteNoteAttachment(const QString &webApiUrlPrefix, const QString &guid);

private:
    enum CustomCacheAttribute {
        NoteContentMd5Hash = QNetworkRequest::User + 100
    };

    // singleton
    static SharedDiskCache *s_instance;
    static qint64 s_diskCacheSize;
    SharedDiskCache();

    // private data
    QAbstractNetworkCache *m_diskCache;
    QMutex m_mutex;
};

#endif // SHAREDDISKCACHE_H
