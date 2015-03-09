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

Item {
    id: root
    anchors.fill: parent
    property variant window;
    property variant pageStack;

    UiStyle {
        id: _UI
    }

    ListView {
        id: listView
        anchors { fill: parent; }
        clip: true
        header: Component {
            Column {
                spacing: _UI.paddingLarge
                anchors { left: parent.left; right: parent.right; }
                GenericSearchBar {
                    anchors { left: parent.left; right: parent.right; }
                    id: searchbox
                    placeHolderText: "Search notes"
                    Keys.onPressed: {
                        if (event.key == Qt.Key_Enter) {
                            root.startSearch(searchbox.searchText);
                        }
                    }
                }
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    id: searchButton
                    text: "Search"
                    onClicked: root.startSearch(searchbox.searchText);
                }
                Item { // spacer
                    anchors { left: parent.left; right: parent.right; }
                    height: 20
                }
                GenericListHeading {
                    text: "Recent searches"
                    horizontalAlignment: Qt.AlignLeft
                    opacity: (internal.hasRecentSearches? 1 : 0)
                }
            }
        }
        delegate: Component {
            Item {
                height: listItem.height
                width: listView.width
                GenericListItem {
                    id: listItem
                    width: listView.width
                    subItemIndicator: true
                    Item {
                        anchors { fill: listItem; margins: _UI.paddingLarge; }
                        clip: true
                        TextLabel {
                            id: textLabel
                            anchors { verticalCenter: parent.verticalCenter; left: parent.left; }
                            width: parent.width - _UI.graphicSizeSmall
                            text: model.modelData
                            elide: Text.ElideRight
                        }
                    }
                    onClicked: root.startSearch(model.modelData)
                }
            }
        }
        model: internal.recentSearches
        footer: Component {
            Column {
                anchors { left: parent.left; right: parent.right; }
                Column {
                    anchors { left: parent.left; right: parent.right; }
                    opacity: (internal.hasRecentSearches? 1 : 0)
                    height: (internal.hasRecentSearches? implicitHeight : 0)
                    Item { // spacer
                        anchors { left: parent.left; right: parent.right; }
                        height: 20
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        id: clearRecentSearchesButton
                        text: "Clear recent searches"
                        enabled: internal.hasRecentSearches
                        onClicked: {
                            clearRecentSearchesConfirmDialog.open();
                        }
                    }
                    Item { // spacer
                        anchors { left: parent.left; right: parent.right; }
                        height: 20
                    }
                }
                Column {
                    spacing: 0
                    anchors { left: parent.left; right: parent.right; }
                    GenericListHeading {
                        text: "Help"
                        horizontalAlignment: Qt.AlignLeft
                    }
                    GenericListItem {
                        id: searchHelplistItem
                        width: listView.width
                        subItemIndicator: true
                        Item {
                            anchors { fill: searchHelplistItem; margins: _UI.paddingLarge; }
                            clip: true
                            TextLabel {
                                id: textLabel
                                anchors { verticalCenter: parent.verticalCenter; left: parent.left; }
                                width: parent.width - _UI.graphicSizeSmall
                                text: "Help on searching"
                                elide: Text.ElideRight
                            }
                        }
                        onClicked: { root.pageStack.push(Qt.resolvedUrl("SearchHelpPage.qml"), { window: root.window }); }
                    }
                }
            }
        }
    }

    QtObject {
        id: internal
        property variant recentSearches;
        property bool hasRecentSearches: ((internal.recentSearches && internal.recentSearches.length > 0)? true : false);
    }

    QueryDialog {
        id: clearRecentSearchesConfirmDialog
        titleText: "Clearing recent searches"
        message: "Remove all recent searches?"
        acceptButtonText: "Clear"
        rejectButtonText: "No"
        onAccepted: {
            qmlDataAccess.clearRecentSearchQueries();
            internal.recentSearches = "";
        }
        height: 150
    }

    Timer {
        id: delayedRecentSearchesLoadTimer
        running: false
        repeat: false
        interval: 300
        onTriggered: {
            internal.recentSearches = qmlDataAccess.recentSearchQueries();
            listView.positionViewAtBeginning();
        }
    }

    function startSearch(searchText) {
        if (searchText != "") {
            var page = root.pageStack.push(Qt.resolvedUrl("NotesListPage.qml"), { window: root.window });
            page.loadSearchLocalNotesResult(searchText);
            qmlDataAccess.setRecentSearchQuery(searchText);
            delayedRecentSearchesLoadTimer.start();
        }
    }

    Component.onCompleted: {
        delayedRecentSearchesLoadTimer.start();
    }
}
