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

#ifndef EDAMUTILS_H
#define EDAMUTILS_H

#include <QString>
#include <QStringList>
#include <QLatin1String>
#include <QByteArray>
#include <QVariantMap>

namespace evernote {
    namespace edam {
        class Note;
        class Notebook;
        class Tag;
        class NoteAttributes;
        class Resource;
    }
}

using namespace evernote;

// converting to/from std::string
QLatin1String latin1StringFromStdString(const std::string &str);
std::string stdStringFromLatin1String(const QLatin1String &str);
std::string stdStringFromLatin1String(const QString &str);
QByteArray byteArrayFromStdString(const std::string &str);
std::string stdStringFromByteArray(const QByteArray &ba);
QString utf8StringFromStdString(const std::string &str);
std::string stdStringFromUtf8String(const QString &str);

#ifndef QT_SIMULATOR

// populating edam::Note
void setEdamNoteGuid(edam::Note *note, const QString guid);
void setEdamNoteTitle(edam::Note *note, const QString &title);
void setEdamNoteContent(edam::Note *note, const QByteArray &content);
void setEdamNoteContentHash(edam::Note *note, const QByteArray &contentHash);
void setEdamNoteCreatedTime(edam::Note *note, qint64 createdTime);
void setEdamNoteUpdatedTime(edam::Note *note, qint64 updatedTime);
void setEdamNoteNotebookGuid(edam::Note *note, const QString &notebookGuid);
void setEdamNoteTagGuids(edam::Note *note, const QStringList &tagGuids);
void setEdamNoteIsActive(edam::Note *note, bool isActive);

// populating edam::Notebook
void setEdamNotebookName(edam::Notebook *notebook, const QString &name);
void setEdamNotebookGuid(edam::Notebook *notebook, const QString &guid);

// populating edam::Tag
void setEdamTagName(edam::Tag *tag, const QString &name);
void setEdamTagGuid(edam::Tag *tag, const QString &guid);

// edam::NoteAttributes
QVariantMap getEdamNoteAttributes(const edam::Note &note);
void setEdamNoteAttributes(edam::Note *note, const QVariantMap &map);

// edam::Resource
bool setEdamNoteResources(edam::Note *note, const QVariantList &attachmentMaps, const QString &noteGuid);

#endif // ifndef QT_SIMULATOR

#endif // EDAMUTILS_H
