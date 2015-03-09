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

#include "qmllocalimagethumbnailprovider.h"
#include "qexifreader.h"
#include <QFile>
#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QImageReader>
#include <QDebug>
#include "qplatformdefs.h"

QmlLocalImageThumbnailProvider::QmlLocalImageThumbnailProvider() : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
{
}

QImage QmlLocalImageThumbnailProvider::requestImage(const QString &id, QSize *size, const QSize &_requestedSize)
{
    QSize requestedSize = _requestedSize;
    if (requestedSize.isEmpty()) { // This size is null in Meego
#if defined(Q_OS_SYMBIAN)
        requestedSize = QSize(160, 160);
#elif defined(MEEGO_EDITION_HARMATTAN)
        requestedSize = QSize(210, 210);
#endif
    }
    const QString &imagePath = id;
    QByteArray embeddedExifThumbnailData;
    int orientationCorrectionAngle = 0;
    getExifData(imagePath, &embeddedExifThumbnailData, &orientationCorrectionAngle);
    QBuffer buf(&embeddedExifThumbnailData);
    QImageReader imageReader;
    if (!embeddedExifThumbnailData.isEmpty()) {
        imageReader.setDevice(&buf);
        imageReader.setAutoDetectImageFormat(false);
        imageReader.setFormat("jpg");
        QSize embeddedThumbnailSize = imageReader.size();
        if (embeddedThumbnailSize.width() < requestedSize.width() && embeddedThumbnailSize.height() < requestedSize.height()) {
            // thumbnail is too small, we should scale down the original image instead of using the embedded thumbnail
            imageReader.setFileName(imagePath);
            imageReader.setAutoDetectImageFormat(true);
        }
    } else {
        imageReader.setFileName(imagePath);
    }

    if (orientationCorrectionAngle == 90 || orientationCorrectionAngle == 270) {
        requestedSize.transpose();
    }
    QSize imageSize = imageReader.size();
    QSize scaledSize = imageSize;
    if (imageSize.width() > requestedSize.width() || imageSize.height() > requestedSize.height()) {
        scaledSize.scale(requestedSize, Qt::KeepAspectRatio);
    }
    (*size) = scaledSize;
    imageReader.setScaledSize(scaledSize);
    QImage img = imageReader.read();
    if (orientationCorrectionAngle != 0) {
        QTransform rotation;
        rotation.rotate(orientationCorrectionAngle);
        img = img.transformed(rotation, Qt::SmoothTransformation);
    }
    return img;
}

void QmlLocalImageThumbnailProvider::getExifData(const QString &path, QByteArray *thumbnailData, int *orientationCorrectionAngle)
{
    QExifReader exifReader(path);
    exifReader.setIncludeKeys(QStringList() << "JpegThumbnailData" << "Orientation");
    QVariantMap exifData = exifReader.read();

    (*thumbnailData) = exifData.value("JpegThumbnailData").toByteArray();

    int orientationCode = exifData.value("Orientation", 0).toInt();
    int correctionAngle = 0;
    if (orientationCode == 3) {
        correctionAngle = 180;
    } else if (orientationCode == 6) {
        correctionAngle = 90;
    } else if (orientationCode == 8) {
        correctionAngle = 270;
    }
    (*orientationCorrectionAngle) = correctionAngle;
}
