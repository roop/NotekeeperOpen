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

    QtObject {
        id: internal
        property int notesListType: -1;
        property string collectionObjectId: "";  // "" / noteId / tagId
        property bool isSearchResultsPage: false;
        property bool isGoBackRequested: false;
        property string objectId; // notebookId or tagId
        property string objectName; // notebookName or tagName
        property string notebookOrTag: ((internal.notesListType == StorageConstants.NotesInNotebook)? "Notebook" :
                                        ((internal.notesListType == StorageConstants.NotesWithTag)? "Tag" : ""));

        property bool is24hourTimeFormat: (root.window && root.window.is24hourTimeFormat)? true : false;
    }

    Rectangle {
        anchors.fill: parent
        color: (qmlDataAccess.isDarkColorSchemeEnabled? "#000" : "#fff")
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Notes"
        buttonIconPath: (internal.isSearchResultsPage?
                             "" :
                             (is_harmattan?
                                  "images/titlebar/nokia_toolbar_add.svg" :
                                  ((is_symbian && internal.notebookOrTag != "")?
                                       "images/titlebar/toolbar-menu.svg" : "")));
        onButtonClicked: {
            if (!internal.isSearchResultsPage) {
                if (is_harmattan) {
                    var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
                    page.delayedLoad("", "");
                }
                if (is_symbian) {
                    menuSymbian.open();
                }
            }
        }
    }

    NotesList  {
        id: notesList
        anchors { fill: parent; topMargin: titleBar.height; }
        containingPage: root
        is24hourTimeFormat: internal.is24hourTimeFormat
        onSearchFinished: {
            if (internal.isGoBackRequested) {
                internal.isGoBackRequested = false;
                root.pageStack.pop();
            }
        }
    }

    tools: ToolBarLayout {
        ToolIconButton {
            iconId: "toolbar-back";
            onClicked: {
                if (internal.isSearchResultsPage && notesList.isSearchInProgress()) {
                    internal.isGoBackRequested = true;
                    notesList.cancelSearch();
                } else {
                    root.pageStack.pop();
                }
            }
        }
        ToolIconButton {
            iconId: "toolbar-settings"
            iconPath: (enabled? "" : "images/toolbar/blank.svg");
            enabled: (is_symbian && !internal.isSearchResultsPage)
            onClicked: {
                var page = root.pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), { window: root.window });
                page.load();
            }
        }
        ToolIconButton {
            iconId: "toolbar-search";
            iconPath: (enabled? "" : "images/toolbar/blank.svg");
            enabled: (is_symbian && !internal.isSearchResultsPage)
            onClicked: root.pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { window: root.window });
        }
        ToolIconButton {
            iconId: (is_symbian? "toolbar-add" : "toolbar-view-menu");
            iconPath: (enabled? "" : "images/toolbar/blank.svg");
            enabled: ((is_symbian && !internal.isSearchResultsPage) ||
                      (is_harmattan && (internal.notebookOrTag != ""))
                      )
            onClicked: {
                if (is_symbian) {
                    var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
                    page.delayedLoad("", "");
                }
                if (is_harmattan) {
                    menuHarmattan.open();
                }
            }
        }
    }

    Menu {
        id: menuHarmattan
        content: MenuLayout {
            id: menuContent
            MenuItem {
                text: ((internal.notesListType == StorageConstants.NotesInNotebook)?
                           "New note in this notebook" :
                           ((internal.notesListType == StorageConstants.NotesWithTag)?
                                "New note with this tag" : "")
                       );
                onClicked: {
                    if (internal.notesListType == StorageConstants.NotesInNotebook) {
                        var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
                        page.delayedLoadNewNoteInNotebook(internal.objectId, internal.objectName);
                    }
                    if (internal.notesListType == StorageConstants.NotesWithTag) {
                        var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
                        page.delayedLoadNewNoteWithTag(internal.objectId, internal.objectName);
                    }
                }
            }
            MenuItem {
                text: "Rename " + internal.notebookOrTag.toLowerCase();
                onClicked: {
                    if (internal.notebookOrTag != "") {
                        renameSheet.text = internal.objectName;
                        renameSheet.open();
                        renameSheet.openKeyboard();
                    }
                }
            }
        }
    }

    ContextMenu {
        id: menuSymbian
        content: MenuLayout {
            MenuItem {
                text: ((internal.notesListType == StorageConstants.NotesInNotebook)?
                           "New note in this notebook" :
                           ((internal.notesListType == StorageConstants.NotesWithTag)?
                                "New note with this tag" : "")
                       );
                onClicked: {
                    if (internal.notesListType == StorageConstants.NotesInNotebook) {
                        var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
                        page.delayedLoadNewNoteInNotebook(internal.objectId, internal.objectName);
                    }
                    if (internal.notesListType == StorageConstants.NotesWithTag) {
                        var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
                        page.delayedLoadNewNoteWithTag(internal.objectId, internal.objectName);
                    }
                }
            }
            MenuItem {
                text: "Rename " + internal.notebookOrTag.toLowerCase();
                onClicked: {
                    if (internal.notebookOrTag != "") {
                        renameSheet.text = internal.objectName;
                        renameSheet.open();
                        renameSheet.openKeyboard();
                    }
                }
            }
        }
    }

    Sheet {
        id: renameSheet
        property alias text: textField.text;
        pageStack: root.pageStack
        acceptButtonText: "Save"
        rejectButtonText: "Cancel"
        content: Item {
            anchors.fill: parent
            Column {
                anchors { fill: parent; margins: _UI.paddingLarge * 2; }
                spacing: _UI.paddingLarge
                TextLabel {
                    anchors.left: parent.left
                    text: ((internal.notebookOrTag != "")? (internal.notebookOrTag + " name:") : "")
                    color: "#000"
                }
                TextField {
                    id: textField
                    anchors { left: parent.left; right: parent.right; }
                    height: (is_symbian? 40 : implicitHeight)
                }
            }
        }
        function openKeyboard() {
            textField.cursorPosition = textField.text.length;
            textField.forceActiveFocus();
            textField.openSoftwareInputPanel();
        }
        onAccepted: {
            if (textField.text.length) {
                var newName = textField.text;
                var success = false;
                if (internal.notesListType == StorageConstants.NotesInNotebook) {
                    success = qmlDataAccess.renameNotebook(internal.objectId, newName);
                }
                if (internal.notesListType == StorageConstants.NotesWithTag) {
                    success = qmlDataAccess.renameTag(internal.objectId, newName);
                }
                if (success) {
                    titleBar.text = newName;
                    internal.objectName = newName;
                } else {
                    root.window.showInfoBanner("Could not rename " + internal.notebookOrTag.toLowerCase());
                }
            }
        }
    }

    function loadAllNotes() {
        notesList.loadAllNotes();
        titleBar.text = "All Notes";
        internal.notesListType = StorageConstants.AllNotes;
    }

    function loadNotesInNotebook(notebookId, notebookName) {
        notesList.loadNotesInNotebook(notebookId, notebookName);
        titleBar.text = notebookName;
        titleBar.iconPath = "images/titlebar/notebook_white.svg";
        internal.notesListType = StorageConstants.NotesInNotebook;
        internal.objectId = notebookId;
        internal.objectName = notebookName;
    }

    function loadNotesWithTag(tagId, tagName) {
        notesList.loadNotesWithTag(tagId, tagName);
        titleBar.text = tagName;
        titleBar.iconPath = "images/titlebar/tag_white.svg";
        internal.notesListType = StorageConstants.NotesWithTag;
        internal.objectId = tagId;
        internal.objectName = tagName;
    }

    function loadFavouriteNotes() {
        notesList.loadFavouriteNotes();
        titleBar.text = "Favourites";
        titleBar.iconPath = "images/titlebar/favourite_white.svg";
        internal.notesListType = StorageConstants.FavouriteNotes;
    }

    function loadTrashNotes() {
        notesList.loadTrashNotes();
        titleBar.text = "Trash";
        titleBar.iconPath = "images/titlebar/trash_white.svg";
        internal.notesListType = StorageConstants.TrashNotes;
    }

    function loadSearchLocalNotesResult(words) {
        internal.isSearchResultsPage = true;
        notesList.loadSearchLocalNotesResult(words);
        titleBar.text = words;
        titleBar.iconPath = "images/titlebar/search_white.svg";
        internal.notesListType = StorageConstants.SearchResultNotes;
    }
}
