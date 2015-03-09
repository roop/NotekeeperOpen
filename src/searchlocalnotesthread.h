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

#ifndef SEARCHLOCALNOTESTHREAD_H
#define SEARCHLOCALNOTESTHREAD_H

#include <QString>
#include <QThread>
#include <QString>
#include <QByteArray>
#include "storage/storagemanager.h"

struct SearchTerm
{
    SearchTerm(const QString &q, const QString &t, bool n) : qualifier(q), text(t), isNegated(n) { }
    bool isTextMatching(const QStringList &words) const;
    QString qualifier;
    QString text;
    bool isNegated;
};

class SearchQuery
{
public:
    SearchQuery(const QString &queryString);
    QList<SearchTerm> searchTerms() const;

private:
    QList<SearchTerm> parseQuery(const QString &queryString);

private:
    QList<SearchTerm> m_searchTerms;
};

class SearchLocalNotesThread : public QThread
{
    Q_OBJECT
public:
    SearchLocalNotesThread(StorageManager *storageManager, const QString &searchQuery, QObject *parent = 0);
    void run();
    void cancel();
signals:
    void searchLocalNotesMatchingNote(const QString &noteId);
    void searchLocalNotesProgressPercentage(int progressPercentage);
    void searchLocalNotesFinished(int unsearchedNotesCount);
private:
    StorageManager * const m_storageManager;
    const QString m_searchQueryString;
    bool m_cancelled;
};

#endif // SEARCHLOCALNOTESTHREAD_H
