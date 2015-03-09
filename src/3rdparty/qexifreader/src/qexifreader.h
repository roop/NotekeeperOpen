/*
  This file is part of QExifReader and is licensed under the MIT License

  Copyright (C) 2013 Roopesh Chander <roop@forwardbias.in>

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

#ifndef QEXIFREADER_H
#define QEXIFREADER_H

#include <QVariantMap>
#include <QStringList>
#include <QFile>

class QExifReader
{
public:
    QExifReader(const QString &fileName);
    QExifReader(QIODevice *device);
    ~QExifReader();

    void setIncludeKeys(const QStringList &includeKeys);
    QVariantMap read();

private:
    void init();

    bool locateExifSegment();
    bool readExifSegment();
    bool readIFDs(qint64 offsetToFirstIFD);
    bool readIFDEntry(quint16 *tag, QVariant *value);
    bool readJpegThumbnailData();

    quint16 nextSegmentType();
    QVariant qVariantFromExifData(const QByteArray &data, quint16 dataFormat, quint32 componentCount);

    bool read_quint16(quint16 *ptr);
    bool read_quint16_exif(quint16 *ptr);
    bool read_quint32_exif(quint32 *ptr);
    quint16 quint16_exif(const QByteArray &ba);
    quint32 quint32_exif(const QByteArray &ba);
    qint16 qint16_exif(const QByteArray &ba);
    qint32 qint32_exif(const QByteArray &ba);

    QIODevice *m_device;
    bool m_isOwnerOfDevice;
    qint64 m_currentSegmentStart;
    quint16 m_currentSegmentSize;
    bool m_isLittleEndianExifSegment;
    qint64 m_tiffHeaderPos;
    QStringList m_includeKeys;
    QVariantMap m_exifData;
    qint64 m_exifSubIFDOffset;
    qint64 m_jpegThumbnailOffset;
    qint64 m_jpegThumbnailSize;
};

#endif // QEXIFREADER_H
