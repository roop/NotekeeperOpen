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

#ifndef QMLNOTEIMAGEPROVIDER_H
#define QMLNOTEIMAGEPROVIDER_H

#include <QDeclarativeImageProvider>
#include <QSslConfiguration>
#include <QUrl>

class StorageManager;
class ConnectionManager;
class QTemporaryFile;

class QmlNoteImageProvider : public QDeclarativeImageProvider
{
public:
    QmlNoteImageProvider(StorageManager *storageManager, ConnectionManager *connectionManager, const QSslConfiguration &sslConf);
    virtual QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    bool fetchFromEvernoteWeb(const QUrl &url, QIODevice* targetDevice);

    StorageManager *m_storageManager;
    ConnectionManager *m_connectionManager;
    QSslConfiguration m_sslConfg;
};

#endif // QMLNOTEIMAGEPROVIDER_H
