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

#ifndef EVERNOTEMARKUP_H
#define EVERNOTEMARKUP_H

#include <QObject>
#include <QVariantList>
#include <QStringList>

namespace EvernoteMarkup
{
    // plainTextToEvernote: Converts plaintext to ENML format
    void enmlFromPlainText(const QString &text, const QVariantList &attachmentsData, QString *title, QByteArray *enmlContent);

    // parseEvernoteMarkup: Converts the note in ENML format to either
    // plaintext or HTML, based on the note contents
    bool parseEvernoteMarkup(const QByteArray &enml, bool *isPlainTextContent, QString *content, bool shouldReturnHtml = false, const QVariantList &attachmentsData = QVariantList(), const QString &attachmentUrlBase = QString(), const QString &qmlResourceBase = QString(), QVariantList *allAttachments = 0, QVariantList *imageAttachments = 0, QVariantList *checkboxStates = 0, QByteArray *convertedEnml = 0);

    // mergeEvernoteMarkup: Merge content in case of a conflict
    QByteArray resolveConflict(const QByteArray &newContentEnml, const QByteArray &oldContentEnml, qint64 oldTimestamp, const QVariantList &attachmentsData, bool *ok);

    // get first few words of the note as summary
    QString plainTextFromEnml(const QByteArray &enml, int maxLength = -1);

    // get all words in the note, used when searching for a note
    QStringList plainTextWordsFromEnml(const QString &title, const QByteArray &enml);

    // update checkbox states in the enml
    bool updateCheckboxStatesInEnml(const QByteArray &enmlContent, const QVariantList &checkboxStates, QByteArray *updatedEnml);

    // append text to enml
    bool appendTextToEnml(const QByteArray &enmlContent, const QString &text, QByteArray *updatedEnml, QByteArray *appendedHtml);

    // append attachment to enml
    bool appendAttachmentToEnml(const QByteArray &enmlContent, const QString &fileUrl, const QVariantMap &properties, QByteArray *updatedEnml, QByteArray *appendedHtml);
}

#endif // EVERNOTEMARKUP_H
