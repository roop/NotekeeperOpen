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

import QtQuick 1.1
import "../native"
import StorageConstants 1.0

Item {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    property bool is24hourTimeFormat: false;
    property variant containingPage; // the page in which this item is used in

    signal searchFinished();

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property int notesListType: -1;
        property string collectionObjectId: "";  // "" / noteId / tagId
        property variant millisecsSinceEpoch: 0; // won't work after May 18, 2033 :)
        property variant millisecsPassedToday: 0;
        property int searchState: _SEARCH_NOT_STARTED;

        // enums
        property int _SEARCH_NOT_STARTED: 0
        property int _LOCAL_SEARCH_IN_PROGRESS: 1
        property int _LOCAL_SEARCH_FINISHED: 2
        property int _REMOTE_SEARCH_IN_PROGRESS: 3
        property int _REMOTE_SEARCH_FINISHED: 4
    }

    Loader {
        id: listLoader
        anchors { fill: parent; bottomMargin: searchProgressBarContainer.height; }
        property variant listModel: undefined
        sourceComponent: {
            if (listModel == undefined) {
                return busyIndicatorComponent;
            } else if ((listModel.count == 0) && (internal.notesListType != StorageConstants.SearchResultNotes)) {
                return noNotesComponent;
            }
            if (_UI.isPortrait) {
                if (internal.notesListType == StorageConstants.NotesInNotebook ||
                        internal.notesListType == StorageConstants.NotesWithTag) {
                    return notesListViewWithSectionScrollComponent;
                }
            }
            return notesListViewComponent;
        }
    }

    Rectangle {
        id: searchProgressBarContainer
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; }
        property bool isSearchInProgress: (internal.searchState == internal._LOCAL_SEARCH_IN_PROGRESS || internal.searchState == internal._REMOTE_SEARCH_IN_PROGRESS)
        height: (isSearchInProgress? _UI.graphicSizeSmall : 0)
        opacity: (isSearchInProgress? 1 : 0)
        color: "#000"
        Row {
            anchors.centerIn: parent
            spacing: 20
            width: searchingText.width + searchProgressBar.width + spacing
            TextLabel {
                id: searchingText;
                color: "#fff";
                text: "Searching";
                font.pixelSize: _UI.fontSizeSmall;
            }
            ProgressBar {
                id: searchProgressBar
                anchors.verticalCenter: parent.verticalCenter
                width: 150
                minimumValue: 0
                maximumValue: 100
                value: qmlDataAccess.searchProgressPercentage
            }
        }
    }

    Component {
        id: busyIndicatorComponent
        Item {
            anchors.fill: parent
            BusyIndicator {
                anchors.centerIn: parent
                running: true
                colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
            }
        }
    }

    Component {
        id: noNotesComponent
        TextLabel {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: "No notes"
            wrapMode: Text.Wrap
            maximumLineCount: 1
            font.pixelSize: _UI.fontSizeLarge
        }
    }

    Component {
        id: notesListViewComponent
        Item {
            property alias header: listView.header;
            property alias footer: listView.footer;
            anchors.fill: parent
            ListView {
                id: listView
                clip: true
                anchors.fill: parent
                model: listLoader.listModel
                delegate: (_UI.isPortrait ? listDelegatePortrait : listDelegateLandscape)
                orientation: (_UI.isPortrait? ListView.Vertical : ListView.Horizontal)
                cacheBuffer: (_UI.screenPortraitHeight * 4)
            }
            ScrollDecorator {
                anchors.fill: parent
                flickableItem: listView
            }
        }
    }

    Component {
        id: notesListViewWithSectionScrollComponent
        Item {
            property alias header: listView.header;
            property alias footer: listView.footer;
            anchors.fill: parent
            ListView {
                id: listView
                clip: true
                anchors.fill: parent
                model: listLoader.listModel
                delegate: (_UI.isPortrait ? listDelegatePortrait : listDelegateLandscape)
                orientation: (_UI.isPortrait? ListView.Vertical : ListView.Horizontal)
                cacheBuffer: (_UI.screenPortraitHeight * 4)
                section.property: "TimestampSectionName"
                section.criteria: ViewSection.FullString
            }
            SectionScroller {
                anchors.fill: parent
                listView: listView
            }
        }
    }

    Component {
        id: listDelegatePortrait
        GenericListItem {
            id: listItem
            width: listLoader.width
            height: Math.max(_UI.fontSizeSmall + _UI.fontSizeMedium * 3 + _UI.paddingLarge * (is_harmattan? 5 : 4), 80)
            Row {
                id: listItemPadded
                anchors { fill: parent; margins: _UI.paddingLarge; }
                Item {
                    height: parent.height
                    width: parent.width - thumbnailLoader.width - (thumbnailLoader.width > 0? _UI.paddingLarge : 0)
                    clip: true
                    Column {
                        anchors { top: parent.top; left: parent.left; right: parent.right; }
                        TextLabel {
                            id: timestampDisplay
                            width: parent.width
                            font.pixelSize: _UI.fontSizeSmall
                            color: _UI.colorNoteTimestamp
                            text: root.timestampDisplayString(model.display.Timestamp, model.display.MillisecondsSinceEpoch, ", ")
                            maximumLineCount: 1
                        }
                        TextLabel {
                            id: titleDisplay
                            width: parent.width
                            font.pixelSize: _UI.fontSizeMedium
                            font.bold: true
                            text: model.display.Title;
                            elide: Text.ElideRight
                            wrapMode: Text.Wrap
                            maximumLineCount: 1
                        }
                        TextLabel {
                            id: contentDisplay
                            width: parent.width
                            font.pixelSize: _UI.fontSizeSmall
                            text: model.display.ContentSummary;
                            elide: Text.ElideRight
                            wrapMode: Text.Wrap
                            color: _UI.colorNoteContentSummary
                            maximumLineCount: 2
                            height: (listItemPadded.height - (timestampDisplay.height + titleDisplay.height))
                        }
                    }
                }
                spacing: _UI.paddingLarge
                Loader {
                    id: thumbnailLoader
                    property string thumbnailPath: model.display.ThumbnailPath
                    height: parent.height
                    width: (thumbnailPath != ""? 80 : 0)
                    sourceComponent: (thumbnailPath != ""? thumbnailComponent : undefined)
                    Component {
                        id: thumbnailComponent
                        Item {
                            anchors.fill: parent
                            Image {
                                id: thumbnail
                                anchors.centerIn: parent
                                source: thumbnailLoader.thumbnailPath
                                width: Math.min(model.display.ThumbnailWidth, 75)
                                height: Math.min(model.display.ThumbnailHeight, 75)
                                smooth: true
                            }
                        }
                    }
                }
            }
            onClicked: root.noteClicked(model.display.NoteId, model.display.Title)
        }
    }

    Component {
        id: listDelegateLandscape
        GenericListItem {
            id: listItem
            width: (is_harmattan? 200 : (_UI.isNokiaE6? 180 : 150))
            height: listLoader.height
            Column {
                id: listItemPadded
                anchors { fill: parent; margins: _UI.paddingLarge; }
                Column {
                    anchors { left: parent.left; right: parent.right; }
                    TextLabel {
                        id: timestampDisplay
                        width: parent.width
                        font.pixelSize: _UI.fontSizeSmall
                        color: _UI.colorNoteTimestamp
                        text: root.timestampDisplayString(model.display.Timestamp, model.display.MillisecondsSinceEpoch, "\n")
                        maximumLineCount: 2
                    }
                    TextLabel {
                        id: titleDisplay
                        width: parent.width
                        font.pixelSize: _UI.fontSizeMedium
                        font.bold: true
                        text: model.display.Title;
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
                        maximumLineCount: 2
                    }
                    TextLabel {
                        id: contentDisplay
                        width: parent.width
                        font.pixelSize: _UI.fontSizeSmall
                        text: model.display.ContentSummary;
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
                        color: _UI.colorNoteContentSummary
                        height: (listItemPadded.height - (timestampDisplay.height + titleDisplay.height + thumbnailLoader.height) - (thumbnailLoader.height > 0? _UI.paddingLarge : 0))
                        maximumLineCount: (contentDisplay.height / lineHeight)
                        lineHeightMode: Text.FixedHeight
                        lineHeight: font.pixelSize + 2
                    }
                }
                spacing: _UI.paddingLarge
                Loader {
                    id: thumbnailLoader
                    property string thumbnailPath: model.display.ThumbnailPath
                    height: (thumbnailPath != ""? 80 : 0)
                    width: parent.width
                    sourceComponent: (thumbnailPath != ""? thumbnailComponent : undefined)
                    Component {
                        id: thumbnailComponent
                        Item {
                            anchors.fill: parent
                            Image {
                                id: thumbnail
                                anchors.centerIn: parent
                                source: thumbnailLoader.thumbnailPath
                                width: Math.min(model.display.ThumbnailWidth, 75)
                                height: Math.min(model.display.ThumbnailHeight, 75)
                                smooth: true
                            }
                        }
                    }
                }
            }
            Rectangle {
                id: rightBorder
                anchors { right: listItem.right; top: listItem.top; bottom: listItem.bottom; }
                width: 1;
                color: "gray";
            }
            onClicked: root.noteClicked(model.display.NoteId, model.display.Title)
        }
    }

    Component {
        id: searchResultsHeaderComponent
        Rectangle {
            id: listItem
            width: (_UI.isPortrait? listLoader.width : 0)
            height: (_UI.isPortrait? (text.height + _UI.paddingSmall * 2) : listLoader.height)
            color: "black"
            opacity: (_UI.isPortrait? 1 : 0)
            TextLabel {
                anchors { top: parent.top; topMargin: _UI.paddingSmall; left: parent.left; right: parent.right; }
                id: text
                width: parent.width
                text: (qmlDataAccess.searchResultsCount + " matching " + (qmlDataAccess.searchResultsCount == 1? "note" : "notes"));
                color: "white"
                font.pixelSize: _UI.fontSizeSmall
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                maximumLineCount: 10
            }
        }
    }

    Component {
        id: searchResultsFooterComponent
        GenericListItem {
            id: listItem
            width: (_UI.isPortrait? listLoader.width : 200)
            height: (_UI.isPortrait? (column.height + _UI.paddingLarge * 2) : listLoader.height)
            property int leftRightTextMargin: (_UI.isPortrait? _UI.paddingLarge * 2 : _UI.paddingMedium)
            subItemIndicator: false
            bottomBorder: false
            onClicked: { root.loadSearchNotesResult(); }

            BusyIndicator {
                anchors.centerIn: parent
                running: (internal.searchState == internal._LOCAL_SEARCH_IN_PROGRESS || internal.searchState == internal._REMOTE_SEARCH_IN_PROGRESS)
                opacity: (running? 1 : 0)
                colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
                size: "medium"
            }

            Column {
                id: column
                anchors { top: (_UI.isPortrait? parent.top : undefined); topMargin: (_UI.isPortrait? _UI.paddingLarge : 0);
                          verticalCenter: (_UI.isPortrait? undefined : parent.verticalCenter);
                          left: parent.left; leftMargin: listItem.leftRightTextMargin;
                          right: parent.right; rightMargin: listItem.leftRightTextMargin; }
                TextLabel {
                    width: parent.width
                    text: "Searched locally available notes only. There are " + qmlDataAccess.unsearchedNotesCount + " more notes in the server.";
                    font.pixelSize: _UI.fontSizeSmall
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: 10
                }
                TextLabel {
                    width: parent.width
                    text: "Tap here to search in the server";
                    font.pixelSize: _UI.fontSizeMedium
                    font.bold: true
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: 10
                }
                opacity: ((internal.searchState == internal._LOCAL_SEARCH_FINISHED)? 1 : 0)
            }

            Rectangle {
                anchors { left: (_UI.isPortrait? parent.left : undefined);
                          right: parent.right;
                          top: (_UI.isPortrait? undefined : parent.top);
                          bottom: parent.bottom;
                        }
                height: (_UI.isPortrait? 1 : undefined)
                width: (_UI.isPortrait? undefined : 1)
                color: (_UI.isDarkColorScheme? "lightgray" : "black")
            }
        }
    }

    function loadAllNotes() {
        internal.notesListType = StorageConstants.AllNotes;
        loadNotesList(internal.notesListType, internal.collectionObjectId);
    }

    function loadNotesInNotebook(notebookId, notebookName) {
        if (notebookId) {
            internal.notesListType = StorageConstants.NotesInNotebook;
            internal.collectionObjectId = notebookId;
            loadNotesList(internal.notesListType, internal.collectionObjectId);
        }
    }

    function loadNotesWithTag(tagId, tagName) {
        if (tagId) {
            internal.notesListType = StorageConstants.NotesWithTag;
            internal.collectionObjectId = tagId;
            loadNotesList(internal.notesListType, internal.collectionObjectId);
        }
    }

    function loadFavouriteNotes() {
        internal.notesListType = StorageConstants.FavouriteNotes;
        loadNotesList(internal.notesListType, internal.collectionObjectId);
    }

    function loadTrashNotes() {
        internal.notesListType = StorageConstants.TrashNotes;
        loadNotesList(internal.notesListType, internal.collectionObjectId);
    }

    function loadSearchLocalNotesResult(words) {
        internal.notesListType = StorageConstants.SearchResultNotes;
        internal.collectionObjectId = words;
        searchNotesListModel.clear();
        qmlDataAccess.startSearchLocalNotes(words);
        listLoader.listModel = searchNotesListModel;
        listLoader.item.header = searchResultsHeaderComponent;
        listLoader.item.footer = searchResultsFooterComponent;
        internal.searchState = internal._LOCAL_SEARCH_IN_PROGRESS;
    }

    function loadSearchNotesResult() {
        var words = internal.collectionObjectId;
        if (words) {
            internal.notesListType = StorageConstants.SearchResultNotes;
            qmlDataAccess.startSearchNotes(words);
            internal.searchState = internal._REMOTE_SEARCH_IN_PROGRESS;
        }
    }

    function noteClicked(noteId, title) {
        var page = root.containingPage.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.containingPage.window });
        page.delayedLoad(noteId, title);
    }

    function loadNotesList(notesListType, objectId) {
        if (internal.notesListType < 0 || notesListType < 0)
            return;
        if (notesListType != internal.notesListType)
            return;
        if (notesListType == StorageConstants.NotesInNotebook || notesListType == StorageConstants.NotesWithTag) {
            if (!objectId)
                return;
            if (!internal.collectionObjectId)
                return;
            if (objectId != internal.collectionObjectId)
                return;
        }
        listLoader.listModel = undefined;
        qmlDataAccess.startListNotes(internal.notesListType, internal.collectionObjectId);
    }

    function gotNotesList(notesList) {
        if (internal.notesListType == StorageConstants.NotesInNotebook ||
            internal.notesListType == StorageConstants.FavouriteNotes ||
            internal.notesListType == StorageConstants.TrashNotes) {
            listLoader.listModel = notebookNotesListModel;
        } else if (internal.notesListType == StorageConstants.NotesWithTag) {
            listLoader.listModel = tagNotesListModel;
        }
    }

    function gotAllNotesList() {
        if (internal.notesListType == StorageConstants.AllNotes) {
            listLoader.listModel = allNotesListModel;
        }
    }

    function searchLocalNotesFinished() {
        if (internal.notesListType == StorageConstants.SearchResultNotes) {
            internal.searchState = internal._LOCAL_SEARCH_FINISHED;
            root.searchFinished(); // emit signal
        }
    }

    function searchNotesFinished() {
        if (internal.notesListType == StorageConstants.SearchResultNotes) {
            listLoader.listModel = searchNotesListModel;
            listLoader.item.footer = null;
            internal.searchState = internal._REMOTE_SEARCH_FINISHED;
            root.searchFinished(); // emit signal
        }
    }

    function isSearchInProgress() {
        return ((internal.searchState == internal._LOCAL_SEARCH_IN_PROGRESS) || (internal.searchState == internal._REMOTE_SEARCH_IN_PROGRESS));
    }

    function cancelSearch() {
        if ((internal.notesListType == StorageConstants.SearchResultNotes) && isSearchInProgress()) {
            qmlDataAccess.cancel();
        }
    }

    function timestampDisplayString(timestamp, msFromEpochTillNoteTimestamp, joinString) {
        var diff = internal.millisecsSinceEpoch - msFromEpochTillNoteTimestamp;
        var timeFormatStr = (root.is24hourTimeFormat? "hh:mm" : "h:mm ap");
        if (diff < internal.millisecsPassedToday) {
            return "Today" + joinString + Qt.formatTime(timestamp, timeFormatStr);
        } else if (diff < internal.millisecsPassedToday + (24 * 60 * 60 * 1000) /*msecs per day*/) {
            return "Yesterday" + joinString + Qt.formatTime(timestamp, timeFormatStr);
        }
        return Qt.formatDateTime(timestamp, "MMM d, yyyy" + joinString + timeFormatStr);
    }

    Component.onCompleted: {
        qmlDataAccess.gotNotesList.connect(gotNotesList);
        qmlDataAccess.gotAllNotesList.connect(gotAllNotesList);
        qmlDataAccess.notesListChanged.connect(loadNotesList);

        qmlDataAccess.searchLocalNotesFinished.connect(searchLocalNotesFinished);
        qmlDataAccess.searchServerNotesFinished.connect(searchNotesFinished);

        var now = new Date;
        internal.millisecsSinceEpoch = now.getTime();
        internal.millisecsPassedToday = ((now.getHours() * 60 + now.getMinutes()) * 60 + now.getSeconds()) * 1000;
    }

    Component.onDestruction: {
        // If we don't disconnect, the slots get called even after the page is popped off the pageStack
        qmlDataAccess.gotNotesList.disconnect(gotNotesList);
        qmlDataAccess.gotAllNotesList.disconnect(gotAllNotesList);
        qmlDataAccess.notesListChanged.disconnect(loadNotesList);
        qmlDataAccess.searchLocalNotesFinished.disconnect(searchLocalNotesFinished);
        qmlDataAccess.searchServerNotesFinished.disconnect(searchNotesFinished);
    }
}
