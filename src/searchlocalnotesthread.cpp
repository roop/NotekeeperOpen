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

#include <QStringList>
#include "searchlocalnotesthread.h"
#include "evernotemarkup.h"

SearchQuery::SearchQuery(const QString &queryString)
{
    m_searchTerms = parseQuery(queryString);
}

QList<SearchTerm> SearchQuery::parseQuery(const QString &queryString)
{
    QList<SearchTerm> terms;
    QString qualifier, text, currentWord;
    bool isWithinQuotes = false;
    bool isNegatedTerm = false;
    for (int i = 0; i < queryString.length(); i++) {
        const QChar c = queryString.at(i);
        bool isSeparator = (!c.isLetterOrNumber() && c != '_' && c != '"' && c != '-' && c != '*');
        if (c == '"') {
            bool isQuoteEscaped = (i > 0 && queryString.at(i - 1) == '\\');
            if (!isQuoteEscaped) {
                isWithinQuotes = (!isWithinQuotes);
            }
        } else if (!isWithinQuotes && c == '-' && qualifier.isEmpty() && currentWord.isEmpty()) {
            isNegatedTerm = true;
        } else if (!isWithinQuotes && c == ':') {
            qualifier = currentWord;
            currentWord.clear();
        } else if (!isWithinQuotes && isSeparator) {
            text = currentWord;
            currentWord.clear();
            if (!qualifier.isEmpty() || !text.isEmpty()) {
                terms.append(SearchTerm(qualifier, text, isNegatedTerm));
                qualifier.clear();
                text.clear();
                isNegatedTerm = false;
            }
        } else {
            currentWord.append(c);
        }
    }
    text = currentWord;
    currentWord.clear();
    if (!qualifier.isEmpty() || !text.isEmpty()) {
        terms.append(SearchTerm(qualifier, text, isNegatedTerm));
    }
    return terms;
}

QList<SearchTerm> SearchQuery::searchTerms() const
{
    return m_searchTerms;
}

static QStringList words(const QString &text)
{
    QStringList wordList;
    QString word;
    for (int i = 0; i < text.length(); i++) {
        const QChar c = text.at(i);
        bool isSeparatorChar = (c.isSpace() || c.isPunct() || c.isSymbol());
        if (isSeparatorChar) {
            if (!word.isEmpty()) {
                wordList << word;
            }
            word.clear();
        } else {
            word.append(c);
        }
    }
    if (!word.isEmpty()) {
        wordList << word;
    }
    return wordList;
}

static bool isWordMatch(const QString &termWord, const QString &noteWord, bool wordPrefixMatch)
{
    if (wordPrefixMatch) {
        return noteWord.simplified().startsWith(termWord.simplified(), Qt::CaseInsensitive);
    } else {
        return (noteWord.simplified().compare(termWord.simplified(), Qt::CaseInsensitive) == 0);
    }
    return false;
}

static bool isSequenceMatch(const QStringList &termWordSequence, const QStringList &noteWordSequence, int ni, bool sequencePrefixMatch)
{
    if (termWordSequence.length() > (noteWordSequence.length() - ni)) {
        return false;
    }
    int i = 0;
    forever {
        if (i >= termWordSequence.length()) {
            break;
        }
        if ((ni + i) >= noteWordSequence.length()) {
            return false; // note has ended while there's still stuff in the term
        }
        const QString &termWord = termWordSequence[i];
        const QString &noteWord = noteWordSequence[ni + i];
        bool matching = isWordMatch(termWord, noteWord, (sequencePrefixMatch && (i == termWordSequence.length() - 1)));
        if (!matching) {
            return false;
        }
        i++;
    }
    return true;
}

bool SearchTerm::isTextMatching(const QStringList &noteWordSequence) const
{
    if (text.isEmpty()) {
        return false;
    }
    bool sequencePrefixMatch = text.endsWith('*');
    QStringList termWordSequence = words(text);
    if (termWordSequence.isEmpty()) {
        return false;
    }
    bool matchFound = false;
    for (int ni = 0; ni < noteWordSequence.length(); ni++) {
        bool isWordMatching = isWordMatch(termWordSequence[0], noteWordSequence[ni], (sequencePrefixMatch && termWordSequence.length() == 1));
        if (isWordMatching) {
            bool isSequenceMatching = isSequenceMatch(termWordSequence, noteWordSequence, ni, sequencePrefixMatch);
            if (isSequenceMatching) {
                matchFound = true;
                break;
            }
        }
    }
    return matchFound;
}

SearchLocalNotesThread::SearchLocalNotesThread(StorageManager *storageManager, const QString &searchQuery, QObject *parent)
    : QThread(parent), m_storageManager(storageManager), m_searchQueryString(searchQuery), m_cancelled(false)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater())); // auto-delete
}

void SearchLocalNotesThread::cancel()
{
    m_cancelled = true;
}

void SearchLocalNotesThread::run()
{
    SearchQuery searchQuery(m_searchQueryString);
    QStringList allNoteIds = m_storageManager->listNoteIds(StorageConstants::AllNotes);
    QStringList noteIdsToSearchIn;

    // Process "notebook:" terms first - they're a special case
    QStringList notebooksInSearchQuery;
    foreach (const SearchTerm &term, searchQuery.searchTerms()) {
        if (term.qualifier.compare("notebook", Qt::CaseInsensitive) == 0) {
            notebooksInSearchQuery << term.text.simplified();
        }
    }
    if (notebooksInSearchQuery.count() > 1) {
        // No notes will match
        emit searchLocalNotesFinished(allNoteIds.count());
        return;
    } else if (notebooksInSearchQuery.count() == 1) {
        // Can restrict search to one notebook
        QString notebookId = m_storageManager->notebookIdForNotebookName(notebooksInSearchQuery.first());
        if (notebookId.isEmpty()) {
            emit searchLocalNotesFinished(allNoteIds.count());
            return;
        }
        QStringList noteIds = m_storageManager->listNoteIds(StorageConstants::NotesInNotebook, notebookId);
        if (noteIds.count() <= 1) {
            noteIdsToSearchIn = noteIds;
        } else {
            // reorder notes in noteIds based on the ordering in allNoteIds
            QStringList orderedIdsList = allNoteIds;
            QSet<QString> chosenIds = noteIds.toSet();
            QMutableStringListIterator iter(orderedIdsList);
            while (iter.hasNext()) {
                QString id = iter.next();
                if (!chosenIds.contains(id)) {
                    iter.remove();
                }
            }
            noteIdsToSearchIn = orderedIdsList;
        }
    } else {
        // notebook: is not specified, so we should search in all notes
        noteIdsToSearchIn = allNoteIds;
    }

    // Process rest of the search terms
    int unsearchedNotesCount = 0;
    int searchedNotesCount = 0;
    foreach (const QString &noteId, noteIdsToSearchIn) {
        if (noteId.isEmpty()) {
            continue;
        }
        QVariantMap dataMap = m_storageManager->noteData(noteId);
        bool contentAvailableLocally = dataMap.value("ContentDataAvailable").toBool();
        bool matchedTermExists = false;
        bool unmatchedTermExists = false;
        bool anyTermCanMatch = false;
        foreach (const SearchTerm &term, searchQuery.searchTerms()) {
            bool isTermTextMatched = false;
            if (term.qualifier.compare("any", Qt::CaseInsensitive) == 0) {
                anyTermCanMatch = true;
            } else if (term.qualifier.compare("notebook", Qt::CaseInsensitive) == 0) {
                // We've already taken care of "notebook:" terms, so it ought to match
                isTermTextMatched = true;
            } else if (term.qualifier.compare("tag", Qt::CaseInsensitive) == 0) {
                QString tagNamesStr = dataMap.value("TagNames").toString();
                QStringList tagNames;
                if (!tagNamesStr.isEmpty()) {
                    tagNames = tagNamesStr.split(',');
                }
                foreach (const QString &tagName, tagNames) {
                    if (term.isTextMatching(words(tagName))) {
                        isTermTextMatched = true;
                        break;
                    }
                }
            } else if (term.qualifier.compare("intitle", Qt::CaseInsensitive) == 0) {
                QString title = dataMap.value("Title").toString();
                isTermTextMatched = term.isTextMatching(words(title));
            } else if (term.qualifier.isEmpty()) { // simple text search
                QString title = dataMap.value("Title").toString();
                QByteArray content("");
                if (contentAvailableLocally) {
                    content = dataMap.value("Content").toByteArray();
                }
                QStringList wordsInNote = EvernoteMarkup::plainTextWordsFromEnml(title, content);
                isTermTextMatched = term.isTextMatching(wordsInNote);
            }
            if (term.isNegated) {
                if (isTermTextMatched) {
                    unmatchedTermExists = true;
                } else {
                    matchedTermExists = true;
                }
            } else {
                if (isTermTextMatched) {
                    matchedTermExists = true;
                } else {
                    unmatchedTermExists = true;
                }
            }
            if (!anyTermCanMatch && unmatchedTermExists) {
                break;
            }
            if (anyTermCanMatch && matchedTermExists) {
                break;
            }
        }
        bool noteMatched = ((anyTermCanMatch && matchedTermExists) || (!unmatchedTermExists));
        if (!contentAvailableLocally && !noteMatched) {
            unsearchedNotesCount++;
        }
        if (m_cancelled) {
            emit searchLocalNotesFinished(allNoteIds.count());
            return;
        }
        if (noteMatched) {
            emit searchLocalNotesMatchingNote(noteId);
        }
        searchedNotesCount++;
        emit searchLocalNotesProgressPercentage(searchedNotesCount * 100 / noteIdsToSearchIn.count());
    }
    emit searchLocalNotesFinished(unsearchedNotesCount);
}
