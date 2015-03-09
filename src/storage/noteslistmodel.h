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

#ifndef NOTESLISTMODEL_H
#define NOTESLISTMODEL_H

#include <QAbstractListModel>
#include <QMutex>
#include <QDateTime>
#include <QTimer>
#include "storage/storagemanager.h"

class NotesListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    Q_PROPERTY(int count READ noteCount NOTIFY noteCountChanged)
    NotesListModel(StorageManager *storageManager, QObject *parent = 0);
    void setNotesListQuery(StorageConstants::NotesListType type, const QString &queryString);
    void clearNotesListQuery();
    int appendNoteIds(const QStringList &noteIds); // returns the total number of noteIds after appending
    Q_INVOKABLE void clear();
    int noteCount() const;

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

public slots:
    void noteCreated(const QString &noteId);
    void noteExpunged(const QString &noteId);
    void noteDisplayDataChanged(const QString &noteId, bool timestampChanged);
    void notebookForNoteChanged(const QString &noteId, const QString &notebookId);
    void tagsForNoteChanged(const QString &noteId, const QStringList &tagIds);
    void noteFavouritenessChanged(const QString &noteId, bool isFavourite);
    void noteTrashednessChanged(const QString &noteId, bool isTrashed);
    QVariantMap get(int index) const;

signals:
    void noteCountChanged();

private slots:
    void updateCurrentDate();

private:
    void load();
    void connectTypeDependantSlots();
    void disconnectTypeDependantSlots();
    void insertNote(const QString &noteId);
    void removeNoteAt(int pos);
    QString timestampSectionName(const QDateTime &noteTimestamp) const;

    StorageManager *m_storageManager;
    StorageConstants::NotesListType m_notesListType;
    QString m_notesListQueryString;
    QStringList m_noteIdsList;

    QDateTime m_currentDate; // kept up-to-date even if the app remains running for >1 day
    QTimer m_refreshCurrentDateTimer;

    mutable QMutex m_mutex;
};

#endif // NOTESLISTMODEL_H
