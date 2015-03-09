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
        text: "Select Tags"
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
            searchBoxPlaceholderText: "Find or create tag";
            selectedItemsHeaderText: "Selected tags";
            allItemsHeaderText: "All tags";
            onCreateObject: {
                var tagId = qmlDataAccess.createTag(name, internal.noteId);
                selectableItemList.objectCreated(tagId, name, true);
                root.window.showInfoBanner("Tag '" + name + "' created");
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
        ToolIconButton { iconId: "toolbar-back"; onClicked: { saveTags(); root.pageStack.pop(); } }
    }

    function setNoteId(noteId) {
        internal.noteId = noteId;
        qmlDataAccess.startListTagsWithTagsOnNoteChecked(noteId);
    }

    function setListModelFromObjectModel(objectModel) {
        listModel.clear();
        for (var i = 0; i < objectModel.length; i++) {
            listModel.append({ objectId: objectModel[i].TagId, name: objectModel[i].Name, checked: objectModel[i].Checked });
        }
        listLoader.sourceComponent = selectableItemListComponent;
        listLoader.item.setAllItemsListModel(listModel);
    }

    function saveTags() {
        if (internal.isLoaded) {
            var tagIdsStr = "";
            var tagNamesStr = "";
            var selectedItems = listLoader.item.selectedItems();
            for (var i = 0; i < selectedItems.count; i++) {
                var item = selectedItems.get(i);
                tagIdsStr = (tagIdsStr? (tagIdsStr + "," + item.objectId) : item.objectId);
                tagNamesStr = (tagNamesStr? (tagNamesStr + ", " + item.name) : item.name);
            }
            qmlDataAccess.setTagsOnNote(internal.noteId, tagIdsStr);
            notePage.updateTags(tagNamesStr, selectedItems.count);
        }
    }

    function gotTagsList(tagsList) {
        setListModelFromObjectModel(tagsList);
        internal.isLoaded = true;
    }

    Component.onCompleted: {
        qmlDataAccess.gotTagsListWithTagsOnNoteChecked.connect(gotTagsList);
        qmlDataAccess.aboutToQuit.connect(saveTags);
    }

    Component.onDestruction: {
        // If we don't disconnect, the slots get called even after the page is popped off the pageStack
        qmlDataAccess.gotTagsListWithTagsOnNoteChecked.disconnect(gotTagsList);
        qmlDataAccess.aboutToQuit.disconnect(saveTags);
    }
}
