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

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    property variant window;
    property variant notePage;

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property string noteId: ""
        property bool isLoaded: false;
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Select a Notebook"
        Behavior on height { NumberAnimation { duration: 200; } }
    }

    Loader {
        id: listLoader
        anchors { fill: parent; topMargin: titleBar.height; }
        sourceComponent: Component {
            Item {
                anchors.fill: parent
                BusyIndicator {
                    anchors.centerIn: parent
                    running: true
                    colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
                }
            }
        }
    }

    Component {
        id: selectableItemListComponent
        SelectableItemList {
            id: selectableItemList
            clip: true
            anchors { fill: listLoader; }
            searchBoxPlaceholderText: "Find or create notebook";
            selectedItemsHeaderText: "Selected notebook";
            allItemsHeaderText: "All notebooks";
            canSelectExactlyOneItem: true;
            onCreateObject: {
                var notebookId = qmlDataAccess.createNotebook(name, internal.noteId);
                selectableItemList.objectCreated(notebookId, name, true);
                root.window.showInfoBanner("Notebook '" + name + "' created");
            }
            onSearchModeEntered: titleBar.height = 0;
            onSearchModeExited: titleBar.height = titleBar.implicitHeight;
        }
    }

    ListModel {
        id: listModel
        property string objectId;
        property string name;
        property bool checked;
    }

    tools: ToolBarLayout {
        id: toolbarLayout
        ToolIconButton { iconId: "toolbar-back"; onClicked: { saveNotebook(); root.pageStack.pop(); } }
    }

    function setNoteId(noteId) {
        internal.noteId = noteId;
        qmlDataAccess.startListNotebooks(StorageConstants.NormalNotebook);
    }

    function setListModelFromObjectModel(objectModel) {
        var selectedNotebookId = qmlDataAccess.notebookForNote(internal.noteId);
        listModel.clear();
        for (var i = 0; i < objectModel.length; i++) {
            var obj = objectModel[i];
            var notebookId = obj.NotebookId;
            var checked = (selectedNotebookId == notebookId);
            listModel.append({ objectId: notebookId, name: obj.Name, checked: checked });
        }
        listLoader.sourceComponent = selectableItemListComponent;
        listLoader.item.setAllItemsListModel(listModel);
    }

    function saveNotebook() {
        if (internal.isLoaded) {
            var selectedItems = listLoader.item.selectedItems();
            if (selectedItems.count > 0) {
                var item = selectedItems.get(0);
                var notebookId = item.objectId;
                var notebookName = item.name;
                if (notebookId && notebookName) {
                    qmlDataAccess.setNotebookForNote(internal.noteId, notebookId);
                    notePage.updateNotebook(notebookName);
                }
            }
        }
    }

    function gotNotebooksList(notebookTypes, notebooksList) {
        if (notebookTypes == StorageConstants.NormalNotebook) {
            setListModelFromObjectModel(notebooksList);
            internal.isLoaded = true;
        }
    }

    Component.onCompleted: {
        qmlDataAccess.gotNotebooksList.connect(gotNotebooksList);
        qmlDataAccess.aboutToQuit.connect(saveNotebook);
    }

    Component.onDestruction: {
        // If we don't disconnect, the slots get called even after the page is popped off the pageStack
        qmlDataAccess.gotNotebooksList.disconnect(gotNotebooksList);
        qmlDataAccess.aboutToQuit.disconnect(saveNotebook);
    }
}
