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

#include "crypto.h"
#include "qblowfish.h"
#include <QByteArray>
#include <QBuffer>
#include <QtDebug>
#include <QCryptographicHash>

#include "notekeeper_config.h"

#define CRYPTO_MARKER_32 0xb10f1501

namespace Crypto {

static QBlowfish *s_qblowfish = 0;

static QBlowfish *createBlowfishInstance()
{
    QByteArray key = QByteArray::fromHex(ENCRYPTION_KEY); // Defined in notekeeper_config.h
    Q_ASSERT(key.length() >= 0);
    QBlowfish *qblowfish = new QBlowfish(key);
    qblowfish->setPaddingEnabled(true);
    qblowfish->init();
    return qblowfish;
}

bool writeEncryptedSettings(QIODevice &device, const QSettings::SettingsMap &map)
{
    QDataStream out(&device);
    quint32 marker = CRYPTO_MARKER_32;
    quint32 dataStreamerVersion = out.version();
    out << marker;
    out << dataStreamerVersion;

    QBuffer clearTextBuffer;
    bool opened = clearTextBuffer.open(QIODevice::WriteOnly);
    if (!opened) {
        return false;
    }
    QDataStream clearTextData(&clearTextBuffer);
    QMapIterator<QString, QVariant> iter(map);
    while (iter.hasNext()) {
        iter.next();
        QString key = iter.key();
        QString value = iter.value().toString();
        if (!key.isEmpty()) {
            clearTextData << key.toUtf8();
            clearTextData << value.toUtf8();
        }
    }
    clearTextBuffer.close();

    QByteArray clearTextBa = clearTextBuffer.buffer();
    QByteArray sha1sum = QCryptographicHash::hash(clearTextBa, QCryptographicHash::Sha1);
    clearTextBa.prepend(sha1sum);

    if (s_qblowfish == 0) {
        s_qblowfish = createBlowfishInstance();
    }
    Q_ASSERT(s_qblowfish != 0);
    QByteArray encryptedBa = s_qblowfish->encrypted(clearTextBa);
    out << encryptedBa;

    return true;
}

bool readEncryptedSettings(QIODevice &device, QSettings::SettingsMap &map)
{
    QDataStream in(&device);
    quint32 marker;
    in >> marker;
    quint32 dataStreamerVersion;
    in >> dataStreamerVersion;
    in.setVersion(dataStreamerVersion);
    if (marker == CRYPTO_MARKER_32) {
        if (s_qblowfish == 0) {
            s_qblowfish = createBlowfishInstance();
        }
        Q_ASSERT(s_qblowfish != 0);
        QByteArray encryptedBa;
        in >> encryptedBa;
        QByteArray ba = s_qblowfish->decrypted(encryptedBa);
        QByteArray sha1sum = ba.mid(0, 20);
        ba = ba.mid(20);
        if (sha1sum.isEmpty() || sha1sum != QCryptographicHash::hash(ba, QCryptographicHash::Sha1)) {
            return false;
        }
        QBuffer clearTextBuffer(&ba);
        bool opened = clearTextBuffer.open(QIODevice::ReadOnly);
        if (!opened) {
            return false;
        }
        QDataStream clearTextData(&clearTextBuffer);
        while (!clearTextData.atEnd()) {
            QByteArray key, value;
            clearTextData >> key;
            clearTextData >> value;
            QString keyString = QString::fromUtf8(key.constData(), key.length());
            QString valueString = QString::fromUtf8(value.constData(), value.length());
            if (!keyString.isEmpty()) {
                map.insert(keyString, valueString);
            }
        }
        return true;
    }
    return false;
}

}
