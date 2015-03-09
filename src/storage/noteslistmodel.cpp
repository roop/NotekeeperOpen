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

#include "noteslistmodel.h"
#include <QMutexLocker>
#include <QDate>

NotesListModel::NotesListModel(StorageManager *storageManager, QObject *parent)
    : QAbstractListModel(parent)
    , m_storageManager(storageManager)
    , m_notesListType(StorageConstants::NoNotes)
    , m_notesListQueryString("")
{
    connect(m_storageManager, SIGNAL(noteDisplayDataChanged(QString,bool)), SLOT(noteDisplayDataChanged(QString,bool)));
    connect(m_storageManager, SIGNAL(noteTrashednessChanged(QString,bool)), SLOT(noteTrashednessChanged(QString,bool)));
    connect(m_storageManager, SIGNAL(noteExpunged(QString)), SLOT(noteExpunged(QString)));
    connect(&m_refreshCurrentDateTimer, SIGNAL(timeout()), SLOT(updateCurrentDate()));
    updateCurrentDate();
}

void NotesListModel::setNotesListQuery(StorageConstants::NotesListType type, const QString &queryString)
{
    if (m_notesListType != type || m_notesListQueryString != queryString) {
        disconnectTypeDependantSlots();
        m_notesListType = type;
        m_notesListQueryString = queryString;
        load();
        connectTypeDependantSlots();
    }
}

void NotesListModel::clearNotesListQuery()
{
    clear();
    m_notesListType = StorageConstants::NoNotes;
    m_notesListQueryString = "";
}

void NotesListModel::load()
{
    bool beginRemoveCalled = false;
    bool beginInsertCalled = false;

    if (!m_noteIdsList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_noteIdsList.length() - 1);
        beginRemoveCalled = true;
    }
    m_mutex.lock();
    m_noteIdsList.clear();
    m_mutex.unlock();
    if (beginRemoveCalled) {
        endRemoveRows();
    }

    QStringList allNotesNoteIdsList, newIdsList;
    if (m_notesListType != StorageConstants::NoNotes && m_notesListType != StorageConstants::TrashNotes) {
        allNotesNoteIdsList = m_storageManager->listNoteIds(StorageConstants::AllNotes);
    }
    if (m_notesListType == StorageConstants::NoNotes) {
        return;
    } else if (m_notesListType == StorageConstants::AllNotes) {
        newIdsList = allNotesNoteIdsList;
    } else if (m_notesListType == StorageConstants::NotesInNotebook) {
        if (m_notesListQueryString.isEmpty() || !m_storageManager->notebookExists(m_notesListQueryString)) {
            return;
        }
        newIdsList = m_storageManager->listNoteIds(m_notesListType, m_notesListQueryString);
    } else if (m_notesListType == StorageConstants::NotesWithTag) {
        if (m_notesListQueryString.isEmpty()) {
            return;
        }
        newIdsList = m_storageManager->listNoteIds(m_notesListType, m_notesListQueryString);
    } else if (m_notesListType == StorageConstants::FavouriteNotes) {
        newIdsList = m_storageManager->listNoteIds(m_notesListType);
    } else if (m_notesListType == StorageConstants::TrashNotes) {
        newIdsList = m_storageManager->listNoteIds(m_notesListType);
    }

    QStringList updatedNoteIdsList;
    if (m_notesListType == StorageConstants::TrashNotes) {
        updatedNoteIdsList = newIdsList;
    } else if (m_notesListType == StorageConstants::AllNotes) {
        updatedNoteIdsList = allNotesNoteIdsList;
    } else {
        // reorder notes in newIdsList based on the ordering in allNotesNoteIdsList
        QStringList orderedIdsList = allNotesNoteIdsList;
        QSet<QString> chosenIds = newIdsList.toSet();
        QMutableStringListIterator iter(orderedIdsList);
        while (iter.hasNext()) {
            QString id = iter.next();
            if (!chosenIds.contains(id)) {
                iter.remove();
            }
        }
        updatedNoteIdsList = orderedIdsList;
    }
    if (!updatedNoteIdsList.isEmpty()) {
        beginInsertRows(QModelIndex(), 0, updatedNoteIdsList.length() - 1);
        beginInsertCalled = true;
    }
    m_mutex.lock();
    m_noteIdsList = updatedNoteIdsList;
    m_mutex.unlock();
    if (beginInsertCalled) {
        endInsertRows();
    }
    emit noteCountChanged();
}

int NotesListModel::appendNoteIds(const QStringList &_noteIds)
{
    bool beginInsertCalled = false;
    int noteCountAfterAppending = 0;
    QStringList noteIds = _noteIds;
    // remove duplicate noteIds
    m_mutex.lock();
    int currentNoteCount = m_noteIdsList.count();
    if (currentNoteCount > 0) {
        QMutableStringListIterator iter(noteIds);
        while (iter.hasNext()) {
            QString noteId = iter.next();
            if (m_noteIdsList.contains(noteId)) {
                iter.remove();
            }
        }
    }
    m_mutex.unlock();
    // append noteIds
    if (!noteIds.isEmpty()) {
        beginInsertRows(QModelIndex(), currentNoteCount, currentNoteCount + noteIds.length() - 1);
        beginInsertCalled = true;
    }
    m_mutex.lock();
    m_noteIdsList << noteIds;
    noteCountAfterAppending = m_noteIdsList.count();
    m_mutex.unlock();
    if (beginInsertCalled) {
        endInsertRows();
    }
    emit noteCountChanged();
    return noteCountAfterAppending;
}

void NotesListModel::clear()
{
    bool beginRemoveCalled = false;

    if (!m_noteIdsList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_noteIdsList.length() - 1);
        beginRemoveCalled = true;
    }
    m_mutex.lock();
    m_noteIdsList.clear();
    m_mutex.unlock();
    if (beginRemoveCalled) {
        endRemoveRows();
    }
    emit noteCountChanged();
}

int NotesListModel::noteCount() const
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    return m_noteIdsList.count();
}

int NotesListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return noteCount();
}

QVariantMap NotesListModel::get(int index) const
{
    QMutexLocker mutexLocker(&m_mutex);
    Q_UNUSED(mutexLocker);
    QString noteId = m_noteIdsList.at(index);
    QVariantMap dataMap = m_storageManager->noteSummaryData(noteId);
    QString timestampSection = timestampSectionName(dataMap.value(QString("Timestamp")).toDateTime());
    dataMap.insert(QString("TimestampSectionName"), timestampSection);
    return dataMap;
}

QVariant NotesListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        return get(index.row());
    }
    return QVariant();
}

QString NotesListModel::timestampSectionName(const QDateTime &noteTimestamp) const
{
    int numOfDaysOld = noteTimestamp.daysTo(m_currentDate);
    if (numOfDaysOld >= 0) {
        if (numOfDaysOld <= 0) {
            return QString("Today");
        } else if (numOfDaysOld <= 1) {
            return QString("Yesterday");
        }
    }

    QDate noteTimestampDate = noteTimestamp.date();
    if (numOfDaysOld >= 0) {
        if (numOfDaysOld <= (noteTimestampDate.dayOfWeek() - 1)) {
            return QString("This week");
        } else if (numOfDaysOld <= (noteTimestampDate.day() - 1)) {
            return QString("This month");
        }
    }

    return QString("%1 %2").arg(QDate::shortMonthName(noteTimestampDate.month())).arg(noteTimestampDate.year());
}

void NotesListModel::updateCurrentDate()
{
    QDateTime now = QDateTime::currentDateTime();
    m_currentDate = QDateTime(now.date()); // snap to the last passed midnight
    int numberOfMSecsInADay = 1000 * 60 * 60 * 24;
    int elapsedMSecsToday = m_currentDate.msecsTo(now);
    int msecsRemainingToday = numberOfMSecsInADay - elapsedMSecsToday;
    if (msecsRemainingToday <= 0) {
        msecsRemainingToday += numberOfMSecsInADay;
    }
    m_refreshCurrentDateTimer.setInterval(msecsRemainingToday + 1000);
    m_refreshCurrentDateTimer.setSingleShot(true);
    m_refreshCurrentDateTimer.start();
}

void NotesListModel::connectTypeDependantSlots()
{
    if (m_notesListType == StorageConstants::AllNotes) {
        connect(m_storageManager, SIGNAL(noteCreated(QString)), this, SLOT(noteCreated(QString)));
    } else if (m_notesListType == StorageConstants::NotesInNotebook) {
        connect(m_storageManager, SIGNAL(notebookForNoteChanged(QString,QString)), this, SLOT(notebookForNoteChanged(QString,QString)));
    } else if (m_notesListType == StorageConstants::NotesWithTag) {
        connect(m_storageManager, SIGNAL(tagsForNoteChanged(QString,QStringList)), this, SLOT(tagsForNoteChanged(QString,QStringList)));
    } else if (m_notesListType == StorageConstants::FavouriteNotes) {
        connect(m_storageManager, SIGNAL(noteFavouritenessChanged(QString,bool)), this, SLOT(noteFavouritenessChanged(QString,bool)));
    } else if (m_notesListType == StorageConstants::TrashNotes) {
        // no new connections to add
    }
}

void NotesListModel::disconnectTypeDependantSlots()
{
    if (m_notesListType == StorageConstants::AllNotes) {
        disconnect(m_storageManager, SIGNAL(noteCreated(QString)), this, SLOT(noteCreated(QString)));
    } else if (m_notesListType == StorageConstants::NotesInNotebook) {
        disconnect(m_storageManager, SIGNAL(notebookForNoteChanged(QString,QString)), this, SLOT(notebookForNoteChanged(QString,QString)));
    } else if (m_notesListType == StorageConstants::NotesWithTag) {
        disconnect(m_storageManager, SIGNAL(tagsForNoteChanged(QString,QStringList)), this, SLOT(tagsForNoteChanged(QString,QStringList)));
    } else if (m_notesListType == StorageConstants::FavouriteNotes) {
        disconnect(m_storageManager, SIGNAL(noteFavouritenessChanged(QString,bool)), this, SLOT(noteFavouritenessChanged(QString,bool)));
    } else if (m_notesListType == StorageConstants::TrashNotes) {
    }
}

void NotesListModel::insertNote(const QString &noteId)
{
    int insertionPos = m_storageManager->noteInsertionPositionInNotesList(noteId, m_noteIdsList);
    beginInsertRows(QModelIndex(), insertionPos, insertionPos);
    m_mutex.lock();
    m_noteIdsList.insert(insertionPos, noteId);
    m_mutex.unlock();
    endInsertRows();
    emit noteCountChanged();
}

void NotesListModel::removeNoteAt(int pos)
{
    beginRemoveRows(QModelIndex(), pos, pos);
    m_mutex.lock();
    m_noteIdsList.removeAt(pos);
    m_mutex.unlock();
    endRemoveRows();
    emit noteCountChanged();
}

void NotesListModel::noteCreated(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return;
    }
    if (m_notesListType != StorageConstants::AllNotes) {
        return;
    }
    if (m_noteIdsList.contains(noteId)) {
        return;
    }
    insertNote(noteId);
}

void NotesListModel::noteExpunged(const QString &noteId)
{
    if (noteId.isEmpty()) {
        return;
    }
    int pos = m_noteIdsList.indexOf(noteId);
    if (pos < 0) {
        return;
    }
    removeNoteAt(pos);
}

void NotesListModel::noteDisplayDataChanged(const QString &noteId, bool timestampChanged)
{
    if (noteId.isEmpty()) {
        return;
    }
    int pos = m_noteIdsList.indexOf(noteId);
    if (pos < 0) {
        return;
    }
    if (timestampChanged) {
        // if timestamp is changed, the note should also be moved to the right position in the list;
        // the list is always kept in order, sorted on timestamp, latest first
        removeNoteAt(pos);
        insertNote(noteId);
    } else {
        // timestamp is unchanged, so just signal that the note data has changed in-place
        QModelIndex changedModelIndex = createIndex(pos, 0);
        emit dataChanged(changedModelIndex, changedModelIndex);
    }
}

void NotesListModel::notebookForNoteChanged(const QString &noteId, const QString &notebookId)
{
    if (noteId.isEmpty() || notebookId.isEmpty()) {
        return;
    }
    if (m_notesListType != StorageConstants::NotesInNotebook) {
        return;
    }
    int pos = m_noteIdsList.indexOf(noteId);
    if (pos >= 0 && notebookId != m_notesListQueryString) {
        // the note is in our list, but the notebookId is not ours
        // => this note was removed from our notebook
        removeNoteAt(pos);
    }
    if (pos < 0 && notebookId == m_notesListQueryString) {
        // the notebook is ours, but this note is not in our list
        // => this note was added to our notebook
        insertNote(noteId);
    }
}

void NotesListModel::tagsForNoteChanged(const QString &noteId, const QStringList &tagIds)
{
    if (noteId.isEmpty()) {
        return;
    }
    if (m_notesListType != StorageConstants::NotesWithTag) {
        return;
    }
    int pos = m_noteIdsList.indexOf(noteId);
    bool tagIdFoundInQueryString = tagIds.contains(m_notesListQueryString);
    if (pos >= 0 && !tagIdFoundInQueryString) {
        // the note is in our list, but the tag is not ours
        // => this note was removed from our tag
        removeNoteAt(pos);
    }
    if (pos < 0 && tagIdFoundInQueryString) {
        // the tag is ours, but this note is not in our list
        // => this note was added to our tag
        insertNote(noteId);
    }
}

void NotesListModel::noteFavouritenessChanged(const QString &noteId, bool isFavourite)
{
    if (noteId.isEmpty()) {
        return;
    }
    if (m_notesListType != StorageConstants::FavouriteNotes) {
        return;
    }
    int pos = m_noteIdsList.indexOf(noteId);
    if (pos >= 0 && !isFavourite) {
        // the note is in our list, but it's no longer a favourite note
        removeNoteAt(pos);
    }
    if (pos < 0 && isFavourite) {
        // the note is now a favourite note, but this note is not in our list yet
        insertNote(noteId);
    }
}

void NotesListModel::noteTrashednessChanged(const QString &noteId, bool isTrashed)
{
    if (noteId.isEmpty()) {
        return;
    }
    int pos = m_noteIdsList.indexOf(noteId);
    if (m_notesListType == StorageConstants::TrashNotes) {
        if (pos >= 0 && !isTrashed) {
            // the note is in our list, but it's no longer a trashed note
            removeNoteAt(pos);
        }
        if (pos < 0 && isTrashed) {
            // the note is now a trashed note, but this note is not in our list yet
            insertNote(noteId);
        }
    } else {
        if (pos >= 0 && isTrashed) {
            // the note is in our list, but it has been trashed
            removeNoteAt(pos);
        } else if (pos < 0 && !isTrashed) {
            // the note is not in our list, and the note has been untrashed;
            // add the note if applicable to us
            if (m_notesListType == StorageConstants::NotesInNotebook &&
                m_storageManager->notebookForNote(noteId) == m_notesListQueryString) {
                insertNote(noteId);
            } else if (m_notesListType == StorageConstants::NotesWithTag &&
                       m_storageManager->tagsOnNote(noteId).contains(m_notesListQueryString)) {
                insertNote(noteId);
            } else if (m_notesListType == StorageConstants::FavouriteNotes &&
                       m_storageManager->isFavouriteNote(noteId)) {
                insertNote(noteId);
            } else if (m_notesListType == StorageConstants::TrashNotes &&
                       m_storageManager->isTrashNote(noteId)) {
                insertNote(noteId);
            }
        }
    }
}
