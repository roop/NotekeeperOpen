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
    width: 100
    height: 62
    anchors.fill: parent
    property variant containingPage; // the page in which this item is used in

    UiStyle {
        id: _UI
    }

    Item {
        id: listLoader
        anchors { fill: parent; }
        property variant listModel: undefined
        BusyIndicator {
            anchors.centerIn: parent
            running: (parent.listModel == undefined)
            opacity: (parent.listModel == undefined? 1.0 : 0.0)
            colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
        }
        ListView {
            id: listView
            clip: true
            anchors { fill: parent }
            model: parent.listModel
            delegate: listDelegate
            opacity: (parent.listModel == undefined? 0.0 : 1.0)
        }
        ScrollDecorator {
            flickableItem: listView
        }
    }

    Component {
        id: listDelegate
        Item {
            height: listItem.height
            width: listView.width
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0; color: (_UI.isDarkColorScheme? "#000" : "#fff"); }
                    GradientStop { position: 0.4; color: (_UI.isDarkColorScheme? "#000" : "#fff"); }
                    GradientStop { position: 1; color: (_UI.isDarkColorScheme? "#000" : "#eaeaea"); }
                }
            }
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
                        text: model.modelData.Name
                    }
                }
                onClicked: root.tagClicked(model.modelData)
            }
        }
    }

    function loadAllTags() {
        loadTagsList();
        qmlDataAccess.tagsListChanged.connect(loadTagsList);
    }

    function loadTagsList() {
        qmlDataAccess.startListTags();
    }

    function tagClicked(modelData) {
        var page = containingPage.pageStack.push(Qt.resolvedUrl("NotesListPage.qml"), { window: containingPage.window });
        page.loadNotesWithTag(modelData.TagId, modelData.Name);
    }

    function gotTagsList(tagsList) {
        listLoader.listModel = tagsList;
    }

    Component.onCompleted: {
        qmlDataAccess.gotTagsList.connect(gotTagsList);
        qmlDataAccess.tagsListChanged.connect(loadTagsList);
    }

    Component.onDestruction: {
        // If we don't disconnect, the slots get called even after the page is popped off the pageStack
        qmlDataAccess.gotTagsList.disconnect(gotTagsList);
        qmlDataAccess.tagsListChanged.disconnect(loadTagsList);
    }
}
