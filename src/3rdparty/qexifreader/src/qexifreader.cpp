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

#include <QFile>
#include <QtEndian>
#include <QVariant>
#include <QVector>
#include <QtDebug>
#include "qexifreader.h"
#include "qexifreader_p.h"

// This code is based on the EXIF format description at
// http://www.media.mit.edu/pia/Research/deepview/exif.html

Q_DECLARE_METATYPE(QVector<int>)
Q_DECLARE_METATYPE(QVector<unsigned int>)
Q_DECLARE_METATYPE(QVector<long>)
Q_DECLARE_METATYPE(QVector<unsigned long>)
Q_DECLARE_METATYPE(QVector<qreal>)

QExifReader::QExifReader(const QString &fileName)
    : m_device(new QFile(fileName))
    , m_isOwnerOfDevice(true)
{
    init();
}

QExifReader::QExifReader(QIODevice *device)
    : m_device(device)
    , m_isOwnerOfDevice(false)
{
    init();
}

QExifReader::~QExifReader()
{
    if (m_isOwnerOfDevice) {
        delete m_device;
    }
}

void QExifReader::init()
{
    m_currentSegmentStart = 0;
    m_currentSegmentSize = 0;
    m_isLittleEndianExifSegment = true;
    m_tiffHeaderPos = 0;
    m_exifSubIFDOffset = 0;
    m_jpegThumbnailOffset = 0;
    m_jpegThumbnailSize = 0;
}

void QExifReader::setIncludeKeys(const QStringList &includeKeys)
{
    m_includeKeys = includeKeys;
}

QVariantMap QExifReader::read()
{
    m_currentSegmentStart = 0;
    m_currentSegmentSize = 0;
    if ((!m_device->isOpen()) || (!m_device->isReadable())) {
        bool ok = m_device->open(QIODevice::ReadOnly);
        if (!ok) {
            return QVariantMap();
        }
    }
    quint16 firstMarker = nextSegmentType();
    if (firstMarker != JPEG_SOI) {
        // Not a JPEG image
        return QVariantMap();
    }
    bool ok = locateExifSegment();
    if (!ok) {
        return QVariantMap();
    }
    readExifSegment();
    readJpegThumbnailData();

    m_device->close();
    return m_exifData;
}

bool QExifReader::locateExifSegment()
{
    for (quint16 segmentType = nextSegmentType(); (segmentType > 0); segmentType = nextSegmentType()) {
        if (segmentType == JPEG_APP1) {
            return true;
        }
    }
    return false;
}

bool QExifReader::readExifSegment()
{
    QByteArray exifHeader = m_device->read(6);
    if (exifHeader != QByteArray::fromPercentEncoding("Exif%00%00")) {
        return false;
    }

    m_tiffHeaderPos = m_device->pos();
    QByteArray tiffHeader1 = m_device->read(2);
    QByteArray tiffHeader2 = m_device->read(2);
    if (tiffHeader1 == "II") { // Intel encoding
        m_isLittleEndianExifSegment = true;
        if (tiffHeader2 != QByteArray::fromHex("2a00")) {
            return false;
        }
    } else if (tiffHeader1 == "MM") { // Motorola encoding
        m_isLittleEndianExifSegment = false;
        if (tiffHeader2 != QByteArray::fromHex("002a")) {
            return false;
        }
    } else {
        return false;
    }
    quint32 offsetToFirstIFD;
    bool ok = read_quint32_exif(&offsetToFirstIFD);
    if (!ok) {
        return false;
    }
    bool ok2 = readIFDs(offsetToFirstIFD);
    if (!ok2) {
        return false;
    }
    // We don't recogonize anything from the EXIF sub-IFD yet
    // if (m_exifSubIFDOffset) {
    //     return readIFDs(m_exifSubIFDOffset);
    // }

    return true;
}

bool QExifReader::readIFDs(qint64 offsetToFirstIFD)
{
    qint64 ifdOffset = offsetToFirstIFD;
    while (ifdOffset > 0) {
        if (!m_device->seek(m_tiffHeaderPos + ifdOffset)) {
            return false;
        }
        quint16 entryCount;
        bool ok = read_quint16_exif(&entryCount);
        if (!ok) {
            return false;
        }
        for (int i = 0; i < entryCount; i++) {
            quint16 tag;
            QVariant value;
            bool ok = readIFDEntry(&tag, &value);
            if (!ok) {
                return false;
            }
            if (tag == EXIF_SUB_IFD_OFFSET_TAG) {
                bool ok;
                m_exifSubIFDOffset = static_cast<qint64>(value.toULongLong(&ok));
                if (!ok) {
                    m_exifSubIFDOffset = 0;
                }
            } else if (tag == JPEG_THUMBNAIL_OFFSET_TAG) {
                bool ok;
                m_jpegThumbnailOffset = static_cast<qint64>(value.toULongLong(&ok));
                if (!ok) {
                    m_jpegThumbnailOffset = 0;
                }
            } else if (tag == JPEG_THUMBNAIL_SIZE_BYTES_TAG) {
                bool ok;
                m_jpegThumbnailSize = static_cast<qint64>(value.toULongLong(&ok));
                if (!ok) {
                    m_jpegThumbnailSize = 0;
                }
            } else {
                QString tagString = stringForTag(tag);
                if (!tagString.isEmpty()) {
                    if (m_includeKeys.isEmpty() || m_includeKeys.contains(tagString)) {
                        m_exifData.insert(tagString, value);
                    }
                }
            }
        }
        quint32 offsetToNextIFD;
        bool ok2 = read_quint32_exif(&offsetToNextIFD);
        if (!ok2) {
            return false;
        }
        ifdOffset = offsetToNextIFD;
    }
}

bool QExifReader::readIFDEntry(quint16 *_tag, QVariant *_value)
{
    quint16 tag;
    quint16 dataFormat;
    quint32 componentCount;
    bool ok1 = read_quint16_exif(&tag);
    bool ok2 = read_quint16_exif(&dataFormat);
    bool ok3 = read_quint32_exif(&componentCount);
    QByteArray valueBa = m_device->read(4);
    bool ok4 = (valueBa.size() == 4);
    qint64 savedPos = m_device->pos();
    if ((!ok1) || (!ok2) || (!ok3) || (!ok4)) {
        return false;
    }
    int bytesPerComponent = bytesPerComponentForDataFormat[dataFormat];
    int bytesRequiredForData = (componentCount * bytesPerComponent);
    if (bytesRequiredForData > 4) {
        // Cannot store this data in a quint32, so what we have is just an offset
        if (!m_device->seek(m_tiffHeaderPos + quint32_exif(valueBa))) {
            return false;
        }
        valueBa = m_device->read(bytesRequiredForData);
        if (valueBa.size() != bytesRequiredForData) {
            return false;
        }
    }
    (*_tag) = tag;
    (*_value) = qVariantFromExifData(valueBa, dataFormat, componentCount);
    if (!m_device->seek(savedPos)) {
        return false;
    }
    return true;
}

bool QExifReader::readJpegThumbnailData()
{
    if (m_tiffHeaderPos == 0 || m_jpegThumbnailOffset == 0 || m_jpegThumbnailSize == 0) {
        return false;
    }
    if (m_includeKeys.isEmpty() || m_includeKeys.contains("JpegThumbnailData")) {
        bool ok = m_device->seek(m_tiffHeaderPos + m_jpegThumbnailOffset);
        if (!ok) {
            return false;
        }
        QByteArray thumbnailData = m_device->read(m_jpegThumbnailSize);
        if (thumbnailData.size() == m_jpegThumbnailSize) {
            m_exifData.insert("JpegThumbnailData", thumbnailData);
            return true;
        }
    }
    return false;
}

quint16 QExifReader::nextSegmentType()
{
    if (!m_device->seek(m_currentSegmentStart + m_currentSegmentSize)) {
        return false;
    }
    quint16 segmentType;
    bool ok = read_quint16(&segmentType);
    if (!ok) {
        m_currentSegmentStart = m_device->pos();
        m_currentSegmentSize = 0;
        return 0;
    }
    if (segmentType == JPEG_SOI || segmentType == JPEG_EOI) {
        // No associated data for SOI and EOI markers
        m_currentSegmentStart = m_device->pos();
        m_currentSegmentSize = 0;
    } else {
        quint16 dataSize;
        bool ok = read_quint16(&dataSize);
        if (!ok) {
            m_currentSegmentStart = m_device->pos();
            m_currentSegmentSize = 0;
            return 0;
        }
        dataSize -= 2; // we shouldn't include the bytes used by the size data itself
        if (dataSize < 0) {
            m_currentSegmentStart = m_device->pos();
            m_currentSegmentSize = 0;
            return 0;
        }
        m_currentSegmentStart = m_device->pos();
        m_currentSegmentSize = dataSize;
    }
    return segmentType;
}

QVariant QExifReader::qVariantFromExifData(const QByteArray &_data, quint16 dataFormat, quint32 componentCount)
{
    int i;
    if (componentCount <= 0) {
        return QVariant();
    }
    int bytesPerComponent = bytesPerComponentForDataFormat[dataFormat];
    QByteArray data = _data.left(bytesPerComponent * componentCount);
    switch (dataFormat) {
    case 1: // unsigned byte
        if (componentCount == 1) {
            if (data.size() < 1) {
                return QVariant();
            }
            return qVariantFromValue(static_cast<unsigned int>(data.at(0)));
        } else {
            QVector<unsigned int> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                value[i] = static_cast<unsigned int>(data.at(i));
            }
            return qVariantFromValue(value);
        }
    case 2: // ascii string
        return qVariantFromValue(QString::fromAscii(data.constData()));
    case 3: // unsigned short
        if (componentCount == 1) {
            return qVariantFromValue(static_cast<unsigned int>(quint16_exif(data)));
        } else {
            QVector<unsigned int> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                value[i] = static_cast<unsigned int>(quint16_exif(data.mid(i * bytesPerComponent, bytesPerComponent)));
            }
            return qVariantFromValue(value);
        }
    case 4: // unsigned long
        if (componentCount == 1) {
            return qVariantFromValue(static_cast<quint32>(quint32_exif(data)));
        } else {
            QVector<unsigned long> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                value[i] = static_cast<quint32>(quint32_exif(data.mid(i * bytesPerComponent, bytesPerComponent)));
            }
            return qVariantFromValue(value);
        }
    case 5: // unsigned rational
        if (componentCount == 1) {
            qreal numerator = qreal(quint32_exif(data.left(4)));
            qreal denominator = qreal(quint32_exif(data.right(4)));
            if (denominator == 0) {
                return QVariant();
            }
            return qVariantFromValue(numerator / denominator);
        } else {
            QVector<qreal> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                qreal numerator = qreal(quint32_exif(data.mid(i * 8, 4)));
                qreal denominator = qreal(quint32_exif(data.mid(i * 8 + 4, 4)));
                if (denominator == 0) {
                    value[i] = 0;
                } else {
                    value[i] = (numerator / denominator);
                }
            }
            return qVariantFromValue(value);
        }
    case 6: // signed byte
        if (componentCount == 1) {
            if (data.size() < 1) {
                return QVariant();
            }
            return qVariantFromValue(static_cast<int>(data.at(0)));
        } else {
            QVector<int> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                value[i] = static_cast<int>(data.at(i));
            }
            return qVariantFromValue(value);
        }
    case 7: // undefined
        return qVariantFromValue(data);
    case 8: // signed short
        if (componentCount == 1) {
            return qVariantFromValue(static_cast<int>(qint16_exif(data)));
        } else {
            QVector<int> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                value[i] = static_cast<int>(qint16_exif(data.mid(i * bytesPerComponent, bytesPerComponent)));
            }
            return qVariantFromValue(value);
        }
    case 9: // signed long
        if (componentCount == 1) {
            return qVariantFromValue(static_cast<qint32>(qint32_exif(data)));
        } else {
            QVector<qint32> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                value[i] = static_cast<qint32>(qint32_exif(data.mid(i * bytesPerComponent, bytesPerComponent)));
            }
            return qVariantFromValue(value);
        }
    case 10: // signed rational
        if (componentCount == 1) {
            qreal numerator = qreal(qint32_exif(data.left(4)));
            qreal denominator = qreal(qint32_exif(data.right(4)));
            if (denominator == 0) {
                return QVariant();
            }
            return qVariantFromValue(numerator / denominator);
        } else {
            QVector<qreal> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                qreal numerator = qreal(qint32_exif(data.mid(i * 8, 4)));
                qreal denominator = qreal(qint32_exif(data.mid(i * 8 + 4, 4)));
                if (denominator == 0) {
                    value[i] = 0;
                } else {
                    value[i] = (numerator / denominator);
                }
            }
            return qVariantFromValue(value);
        }
    case 11: // single float
        if (data.size() != 4 * componentCount) {
            return qVariantFromValue(0);
        }
        if (componentCount == 1) {
            const float *realPtr = reinterpret_cast<const float *>(data.constData());
            return qVariantFromValue(*realPtr);
        } else {
            QVector<qreal> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                const float *realPtr = reinterpret_cast<const float *>(data.constData() + i * 4);
                value[i] = (*realPtr);
            }
            return qVariantFromValue(value);
        }
    case 12: // double float
        if (data.size() != 8 * componentCount) {
            return qVariantFromValue(0);
        }
        if (componentCount == 1) {
            const double *realPtr = reinterpret_cast<const double *>(data.constData());
            return qVariantFromValue(*realPtr);
        } else {
            QVector<qreal> value(componentCount);
            for (i = 0; i < componentCount; i++) {
                const float *realPtr = reinterpret_cast<const float *>(data.constData() + i * 4);
                value[i] = (*realPtr);
            }
            return qVariantFromValue(value);
        }
    default:
        return QVariant();
    }
}

bool QExifReader::read_quint16(quint16 *ptr)
{
    QByteArray ba = m_device->read(2);
    if (ba.size() < 2) {
        return false;
    }
    (*ptr) = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(ba.constData()));
    return true;
}

bool QExifReader::read_quint16_exif(quint16 *ptr)
{
    QByteArray ba = m_device->read(2);
    if (ba.size() < 2) {
        return false;
    }
    if (m_isLittleEndianExifSegment) {
        (*ptr) = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(ba.constData()));
    } else {
        (*ptr) = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(ba.constData()));
    }
    return true;
}

bool QExifReader::read_quint32_exif(quint32 *ptr)
{
    QByteArray ba = m_device->read(4);
    if (ba.size() < 4) {
        return false;
    }
    if (m_isLittleEndianExifSegment) {
        (*ptr) = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(ba.constData()));
    } else {
        (*ptr) = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(ba.constData()));
    }
    return true;
}

quint16 QExifReader::quint16_exif(const QByteArray &ba)
{
    if (ba.size() < 2) {
        return 0;
    }
    if (m_isLittleEndianExifSegment) {
        return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(ba.constData()));
    } else {
        return qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(ba.constData()));
    }
}

quint32 QExifReader::quint32_exif(const QByteArray &ba)
{
    if (ba.size() < 4) {
        return 0;
    }
    if (m_isLittleEndianExifSegment) {
        return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(ba.constData()));
    } else {
        return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(ba.constData()));
    }
}

qint16 QExifReader::qint16_exif(const QByteArray &ba)
{
    if (ba.size() < 2) {
        return 0;
    }
    if (m_isLittleEndianExifSegment) {
        return qFromLittleEndian<qint16>(reinterpret_cast<const uchar*>(ba.constData()));
    } else {
        return qFromBigEndian<qint16>(reinterpret_cast<const uchar*>(ba.constData()));
    }
}

qint32 QExifReader::qint32_exif(const QByteArray &ba)
{
    if (ba.size() < 4) {
        return 0;
    }
    if (m_isLittleEndianExifSegment) {
        return qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(ba.constData()));
    } else {
        return qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(ba.constData()));
    }
}
