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

#include "edamutils.h"
#include <QFile>
#include <QSize>
#include <QtDebug>

// c++ stl
#ifdef Q_OS_SYMBIAN
#include <stdapis/stlportv5/string>
#include <stdapis/stlportv5/vector>
#include <arpa/inet.h>
#endif

#ifndef QT_SIMULATOR
// evernote
#include "NoteStore.h"
#endif

QLatin1String latin1StringFromStdString(const std::string &str)
{
    return QLatin1String(str.c_str());
}

std::string stdStringFromLatin1String(const QLatin1String &str)
{
    return std::string(str.latin1());
}

std::string stdStringFromLatin1String(const QString &str)
{
    return stdStringFromByteArray(str.toLatin1());
}

QByteArray byteArrayFromStdString(const std::string &str)
{
    return QByteArray(str.data(), str.size());
}

std::string stdStringFromByteArray(const QByteArray &ba)
{
    return std::string(ba.constData(), ba.size());
}

QString utf8StringFromStdString(const std::string &str)
{
    return QString::fromUtf8(str.c_str(), str.length());
}

std::string stdStringFromUtf8String(const QString &str)
{
    return stdStringFromByteArray(str.toUtf8());
}

#ifndef QT_SIMULATOR

void setEdamNoteGuid(edam::Note *note, const QString guid)
{
    note->guid = stdStringFromLatin1String(guid);
    note->__isset.guid = true;
}

void setEdamNoteTitle(edam::Note *note, const QString &title)
{
    note->title = stdStringFromUtf8String(title);
    note->__isset.title = true;
}

void setEdamNoteContent(edam::Note *note, const QByteArray &content)
{
    note->content = stdStringFromByteArray(content);
    note->__isset.content = true;
}

void setEdamNoteContentHash(edam::Note *note, const QByteArray &contentHash)
{
    note->contentHash = stdStringFromByteArray(contentHash);
    note->__isset.contentHash = true;
}

void setEdamNoteCreatedTime(edam::Note *note, qint64 createdTime)
{
    note->created = createdTime;
    note->__isset.created = true;
}

void setEdamNoteUpdatedTime(edam::Note *note, qint64 updatedTime)
{
    note->updated = updatedTime;
    note->__isset.updated = true;
}

void setEdamNoteNotebookGuid(edam::Note *note, const QString &notebookGuid)
{
    note->notebookGuid = stdStringFromLatin1String(notebookGuid);
    note->__isset.notebookGuid = true;
}

void setEdamNoteTagGuids(edam::Note *note, const QStringList &tagGuids)
{
    note->tagGuids.clear();
    foreach (const QString &tagGuid, tagGuids) {
        note->tagGuids.push_back(stdStringFromLatin1String(tagGuid));
    }
    note->__isset.tagGuids = true;
}

void setEdamNoteIsActive(edam::Note *note, bool isActive)
{
    note->active = isActive;
    note->__isset.active = true;
}

void setEdamNotebookName(edam::Notebook *notebook, const QString &name)
{
    notebook->name = stdStringFromUtf8String(name);
    notebook->__isset.name = true;
}

void setEdamNotebookGuid(edam::Notebook *notebook, const QString &guid)
{
    notebook->guid = stdStringFromLatin1String(guid);
    notebook->__isset.guid = true;
}

void setEdamTagName(edam::Tag *tag, const QString &name)
{
    tag->name = stdStringFromUtf8String(name);
    tag->__isset.name = true;
}

void setEdamTagGuid(edam::Tag *tag, const QString &guid)
{
    tag->guid = stdStringFromLatin1String(guid);
    tag->__isset.guid = true;
}

QVariantMap getEdamNoteAttributes(const edam::Note &note)
{
    QVariantMap map;
    if (!note.__isset.attributes) {
        return map;
    }
    const edam::NoteAttributes &noteAttributes = note.attributes;
    if (noteAttributes.__isset.subjectDate) {
        map["Attributes/SubjectDate"] = qint64(noteAttributes.subjectDate);
    }
    if (noteAttributes.__isset.latitude) {
        map["Attributes/Latitude"] = double(noteAttributes.latitude);
    }
    if (noteAttributes.__isset.longitude) {
        map["Attributes/Longitude"] = double(noteAttributes.longitude);
    }
    if (noteAttributes.__isset.author) {
        map["Attributes/Author"] = utf8StringFromStdString(noteAttributes.author);
    }
    if (noteAttributes.__isset.source) {
        map["Attributes/Source"] = utf8StringFromStdString(noteAttributes.source);
    }
    if (noteAttributes.__isset.sourceURL) {
        map["Attributes/SourceUrl"] = latin1StringFromStdString(noteAttributes.sourceURL);
    }
    if (noteAttributes.__isset.sourceApplication) {
        map["Attributes/SourceApplication"] =  utf8StringFromStdString(noteAttributes.sourceApplication);
    }
    if (noteAttributes.__isset.shareDate) {
        map["Attributes/ShareDate"] = qint64(noteAttributes.shareDate);
    }
    if (noteAttributes.__isset.contentClass) {
        map["Attributes/ContentClass"] = utf8StringFromStdString(noteAttributes.contentClass);
    }
    return map;
}

void setEdamNoteAttributes(edam::Note *note, const QVariantMap &map)
{
    qint64 subjectDate = map.value("Attributes/SubjectDate", 0).toLongLong();
    double latitude = map.value("Attributes/Latitude", 0).toDouble();
    double longitude = map.value("Attributes/Longitude", 0).toDouble();
    QString author = map.value("Attributes/Author", QString()).toString();
    QString source = map.value("Attributes/Source", QString()).toString();
    QString sourceUrl = map.value("Attributes/SourceUrl", QString()).toString();
    QString sourceApp = map.value("Attributes/SourceApplication", QString()).toString();
    qint64 shareDate = map.value("Attributes/ShareDate", 0).toLongLong();
    edam::NoteAttributes &noteAttributes = note->attributes;
    bool noteAttributeAdded = false;
    if (subjectDate > 0) {
        noteAttributes.subjectDate = subjectDate;
        noteAttributes.__isset.subjectDate = true;
        noteAttributeAdded = true;
    }
    if (latitude > 0) {
        noteAttributes.latitude = latitude;
        noteAttributes.__isset.latitude = true;
        noteAttributeAdded = true;
    }
    if (longitude > 0) {
        noteAttributes.longitude = longitude;
        noteAttributes.__isset.longitude = true;
        noteAttributeAdded = true;
    }
    if (author.length() > 0) {
        noteAttributes.author = stdStringFromUtf8String(author);
        noteAttributes.__isset.author = true;
        noteAttributeAdded = true;
    }
    if (source.length() > 0) {
        noteAttributes.source = stdStringFromUtf8String(source);
        noteAttributes.__isset.source = true;
        noteAttributeAdded = true;
    }
    if (sourceUrl.length() > 0) {
        noteAttributes.sourceURL = stdStringFromLatin1String(sourceUrl);
        noteAttributes.__isset.sourceURL = true;
        noteAttributeAdded = true;
    }
    if (sourceApp.length() > 0) {
        noteAttributes.sourceApplication = stdStringFromUtf8String(sourceApp);
        noteAttributes.__isset.sourceApplication = true;
        noteAttributeAdded = true;
    }
    if (shareDate > 0) {
        noteAttributes.shareDate = shareDate;
        noteAttributes.__isset.shareDate = true;
        noteAttributeAdded = true;
    }
    if (noteAttributeAdded) {
        note->__isset.attributes = true;
    }
}

bool setEdamNoteResources(edam::Note *note, const QVariantList &attachmentMaps, const QString &noteGuid)
{
    note->resources.clear();
    note->resources.resize(attachmentMaps.count());
    for (int i = 0; i < attachmentMaps.count(); i++) {
        const QVariantMap &map = attachmentMaps.at(i).toMap();
        edam::Resource &resource = note->resources[i];
        QByteArray md5Hash = map.value("Hash").toByteArray();
        QString mimeType = map.value("MimeType").toString();
        QString originalFileName = map.value("FileName").toString();
        if (md5Hash.isEmpty() || mimeType.isEmpty()) {
            qDebug() << "No md5hash or mime found for attachment:" << originalFileName;
            return false;
        }
        QString attachmentGuid = map.value("guid").toString();
        qint32 attachmentSize = map.value("Size").toInt();
        QString filePath = map.value("FilePath").toString();
        QSize dimensions = map.value("Dimensions").toSize();
        if (!noteGuid.isEmpty()) {
            resource.noteGuid = stdStringFromLatin1String(noteGuid);
            resource.__isset.noteGuid = true;
        }
        resource.data.bodyHash = stdStringFromByteArray(QByteArray::fromHex(md5Hash));
        resource.data.__isset.bodyHash = true;
        resource.data.size = attachmentSize;
        resource.data.__isset.size = true;
        if (attachmentGuid.isEmpty()) { // send actual file contents only if the server doesn't already have it
            if (filePath.isEmpty()) {
                qDebug() << "No filePath found for attachment:" << originalFileName;
                return false;
            }
            QFile attachmentFile(filePath);
            qint32 fileSize = static_cast<qint32>(attachmentFile.size());
            if ((fileSize != attachmentSize) || (!attachmentFile.open(QFile::ReadOnly))) {
                qDebug() << "Could not open attachment:" << originalFileName;
                return false;
            }
            try {
                resource.data.body.reserve(fileSize);
            } catch(...) {
                qDebug() << "std::string.reserve() threw an exception. Attachment:" << originalFileName;
            }
            if (resource.data.body.capacity() < fileSize) {
                qDebug() << "Not enough memory for pushing attachment " + originalFileName + " to Evernote";
                return false;
            }
            qint64 totalBytesWritten = 0;
            while (!attachmentFile.atEnd()) {
                QByteArray data = attachmentFile.read(1 << 18); // max 256 kb
                resource.data.body.append(data.constData(), data.length());
                totalBytesWritten += data.length();
            }
            if (totalBytesWritten != fileSize) {
                qDebug() << "Could not fully read attachment:" << originalFileName;
                return false;
            }
            resource.data.__isset.body = true;
        }
        resource.__isset.data = true;
        resource.mime = stdStringFromUtf8String(mimeType);
        resource.__isset.mime = true;
        if (dimensions.isValid()) {
            resource.width = static_cast<qint16>(dimensions.width());
            resource.__isset.width = true;
            resource.height = static_cast<qint16>(dimensions.height());
            resource.__isset.height = true;
        }
        resource.attributes.fileName = stdStringFromUtf8String(originalFileName);
        resource.attributes.__isset.fileName = true;
        resource.__isset.attributes = true;
    }
    note->__isset.resources = true;
    return true;
}

#endif // ifndef QT_SIMULATOR
