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
import com.nokia.meego 1.1
import "SyncTriggerer.js" as SyncTrigger

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    property variant window;

    Rectangle {
        anchors.fill: parent
        color: (qmlDataAccess.isDarkColorSchemeEnabled? "#000" : "#fff")
    }

    TabGroup {
        id: tabGroup
        anchors.fill: parent
        currentTab: notesTabContent
        Item {
            id: notesTabContent
            property alias loader: notesTabLoader
            property bool loaded: false
            anchors.fill: parent
            Loader {
                id: notesTabLoader
                anchors.fill: parent
                onLoaded: {
                    item.contentItem.loadAllNotes();
                    item.contentItem.containingPage = root;
                    parent.loaded = true;
                }
            }
        }
        Item {
            id: notebooksTabContent
            property alias loader: notebooksTabLoader
            property bool loaded: false
            anchors.fill: parent
            Loader {
                id: notebooksTabLoader
                anchors.fill: parent
                onLoaded: {
                    item.contentItem.loadAllNotebooks();
                    item.contentItem.containingPage = root;
                    parent.loaded = true;
                }
            }
        }
        Item {
            id: tagsTabContent
            property alias loader: tagsTabLoader
            property bool loaded: false
            anchors.fill: parent
            Loader {
                id: tagsTabLoader
                anchors.fill: parent
                onLoaded: {
                    item.contentItem.loadAllTags();
                    item.contentItem.containingPage = root;
                    parent.loaded = true;
                }
            }
        }
        Item {
            id: searchTabContent
            property alias loader: tagsTabLoader
            property bool loaded: false
            anchors.fill: parent
            Loader {
                id: searchTabLoader
                anchors.fill: parent
                onLoaded: {
                    item.contentItem.window = root.window;
                    item.contentItem.pageStack = root.pageStack;
                    parent.loaded = true;
                }
            }
        }
        Item {
            id: settingsTabContent
            property alias loader: tagsTabLoader
            property bool loaded: false
            anchors.fill: parent
            Loader {
                id: settingsTabLoader
                anchors.fill: parent
                onLoaded: {
                    item.contentItem.load();
                    item.contentItem.window = root.window;
                    item.contentItem.pageStack = root.pageStack;
                    parent.loaded = true;
                }
            }
        }
        onCurrentTabChanged: {
            ensureCurrentTabIsLoaded();
        }
    }

    Component {
        id: notesListComponent
        Item {
            anchors.fill: parent
            property alias contentItem: ci;
            TitleBar {
                id: titleBar
                anchors { left: parent.left; right: parent.right; top: parent.top; }
                text: "All notes"
                iconPath: "images/titlebar/notepad_white.svg"
                buttonIconPath: "images/titlebar/nokia_toolbar_add.svg"
                onButtonClicked: { root.showAddNotePage(); }
            }
            NotesList {
                id: ci
                anchors { fill: parent; topMargin: titleBar.height; }
                is24hourTimeFormat: internal.is24hourTimeFormat
            }
        }
    }

    Component {
        id: notebooksListComponent
        Item {
            property alias contentItem: ci;
            anchors.fill: parent
            TitleBar {
                id: titleBar
                anchors { left: parent.left; right: parent.right; top: parent.top; }
                text: "Notebooks"
                iconPath: "images/titlebar/notebook_white.svg"
                buttonIconPath: "images/titlebar/nokia_toolbar_add.svg"
                onButtonClicked: { root.showAddNotePage(); }
            }
            NotebooksList {
                id: ci
                anchors { fill: parent; topMargin: titleBar.height; }
            }
        }
    }

    Component {
        id: tagsListComponent
        Item {
            property alias contentItem: ci;
            anchors.fill: parent
            TitleBar {
                id: titleBar
                anchors { left: parent.left; right: parent.right; top: parent.top; }
                text: "Tags"
                iconPath: "images/titlebar/tag_white.svg"
                buttonIconPath: "images/titlebar/nokia_toolbar_add.svg"
                onButtonClicked: { root.showAddNotePage(); }
            }
            TagsList {
                id: ci
                anchors { fill: parent; topMargin: titleBar.height; }
            }
        }
    }

    Component {
        id: searchComponent
        Item {
            property alias contentItem: ci;
            anchors.fill: parent
            TitleBar {
                id: titleBar
                anchors { left: parent.left; right: parent.right; top: parent.top; }
                text: "Search"
                iconPath: "images/titlebar/search_white.svg"
                buttonIconPath: "images/titlebar/nokia_toolbar_add.svg"
                onButtonClicked: { root.showAddNotePage(); }
                dismissKeyboardButton: (is_harmattan? true : false)
            }
            SearchView {
                id: ci
                anchors { fill: parent; topMargin: titleBar.height; }
            }
        }
    }

    Component {
        id: settingsComponent
        Item {
            property alias contentItem: ci;
            anchors.fill: parent
            TitleBar {
                id: titleBar
                anchors { left: parent.left; right: parent.right; top: parent.top; }
                text: "Settings"
                iconPath: "images/titlebar/settings_white.svg"
                buttonIconPath: "images/titlebar/nokia_toolbar_add.svg"
                onButtonClicked: { root.showAddNotePage(); }
            }
            SettingsView {
                id: ci
                anchors { fill: parent; topMargin: titleBar.height; }
                window: root.window
            }
        }
    }

    tools: ToolBarLayout {
        ButtonRow {
            platformStyle: TabButtonStyle { }
            TabButton {
                tab: notesTabContent
                iconSource: "images/toolbar/notepad_n9" + (qmlDataAccess.isDarkColorSchemeEnabled? "_white" : "") + ".png"
            }
            TabButton {
                tab: notebooksTabContent
                iconSource: "images/toolbar/notebook_n9" + (qmlDataAccess.isDarkColorSchemeEnabled? "_white" : "") + ".png"
            }
            TabButton {
                tab: tagsTabContent
                iconSource: "image://theme/icon-m-toolbar-tag" + (qmlDataAccess.isDarkColorSchemeEnabled? "-white" : "")
            }
            TabButton {
                tab: searchTabContent
                iconSource: "image://theme/icon-m-toolbar-search" + (qmlDataAccess.isDarkColorSchemeEnabled? "-white" : "")
            }
            TabButton {
                tab: settingsTabContent
                iconSource: "image://theme/icon-m-toolbar-settings" + (qmlDataAccess.isDarkColorSchemeEnabled? "-white" : "")
            }
        }
    }

    Timer {
        id: startupSyncTriggerTimer;
        running: false;
        repeat: false;
        interval: 10000; // trigger a sync 10 seconds after the start page is loaded
        onTriggered: SyncTrigger.triggerSync();
    }

    QtObject {
        id: internal
        property bool isSyncing: false;
        property bool isLoadingSuppressed: true;
        property bool is24hourTimeFormat: (root.window && root.window.is24hourTimeFormat)? true : false;
    }

    function sync() {
        internal.isSyncing = true;
        evernoteSync.startSync();
    }

    function evernoteSyncFinished(success, message) {
        var infoMessage = "Synchronization " + (success? "complete" : "failed");
        if (message) {
            infoMessage += "\n" + message;
        }
        root.window.showInfoBanner(infoMessage);
        internal.isSyncing = false;
    }

    function load() {
        evernoteSync.loginError.connect(root.window.showLoginErrorDialog);
        var processed = processCreateNoteRequestFromCommandLine();
        internal.isLoadingSuppressed = false;
        ensureCurrentTabIsLoaded();
        if (!processed) {
            // if started normally, we schedule an auto-sync
            startupSyncTriggerTimer.start();
        }
    }

    function showMessage(code, title, msg) {
        var warningEnabled = true;
        if (code == "UNUSUAL_ACCESS_POINT_ORDERING") {
            warningEnabled = (!qmlDataAccess.retrieveSetting("Warn/dontWarnOnUnusualIAPOrdering"));
        }
        if (warningEnabled) {
            root.window.showErrorMessageDialog(title, msg);
        }
    }

    function showAddNotePage() {
        var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
        page.delayedLoad("", "");
    }

    function processCreateNoteRequestFromCommandLine() {
        if (is_harmattan) {
            if (appArguments && (appArguments.indexOf("-create-note-request") > 0)) {
                return processCreateNoteRequest();
            } else {
                qmlDataAccess.harmattanRemoveCreateNoteRequest(); // clear any existing request file
            }
        }
        return false;
    }

    function processCreateNoteRequest() {
        if (!is_harmattan) {
            return false;
        }
        var map = qmlDataAccess.harmattanReadAndRemoveCreateNoteRequest();
        if (!map.isValidRequest) {
            return false;
        }
        var atStartPage = popTillStartPage();
        if (!atStartPage) {
            return false;
        }

        var title = (map.Title? map.Title : "");
        var url = (map.Url? map.Url : "");

        // Create note with title and url
        var noteId = qmlDataAccess.createNoteWithUrl(title, url);
        var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window, isProcessingCreateNoteRequest: true }, true /*immediate*/);

        // Open the note; If there are attachments to be added, start adding them
        if (map.Attachments) {
            page.delayedLoadAndAttach(noteId, title, map.Attachments)
        } else {
            page.delayedLoad(noteId, title);
            if (url) {
                root.window.showInfoBanner("Web URL added to note");
            }
        }
        return true;
    }

    function popTillStartPage() {
        // If we're already at the top, nothing to do
        if (root.pageStack.currentPage == root) {
            return true;
        }

        // Check if the StartPage is in the stack
        var startPageInStack = root.pageStack.find(function (page) {
                                                       if (page == root) { return true; }
                                                       return false;
                                                   });
        if (!startPageInStack) {
            return false;
        }

        // If there's a NotePage in the stack, save the note first
        var notePage = root.pageStack.find(function (page) {
                                               if (page.isNotePage) { return true; }
                                               return false;
                                           });
        if (notePage) {
            notePage.saveUnlessWeHaveToCreateAnEmptyNote();
        }

        // Pop off everything till the StartPage
        root.pageStack.pop(root, true /*immediate*/);

        // Show statusbar and toolbar in case they were hidden (eg. we were in the ImageBrowserPage)
        root.window.showStatusBar = true;
        root.window.showToolBar = true;

        return true;
    }

    function appActiveStateChanged() {
        if (!platformWindow) {
            return false;
        }
        if (platformWindow.active) { // app has just come to the foreground
            processCreateNoteRequest();
        }
    }

    function ensureCurrentTabIsLoaded() {
        var currentTab = tabGroup.currentTab;
        if ((!internal.isLoadingSuppressed) && (!currentTab.loaded)) {
            if (currentTab == notesTabContent) {
                notesTabLoader.sourceComponent = notesListComponent;
            } else if (currentTab == notebooksTabContent) {
                notebooksTabLoader.sourceComponent = notebooksListComponent;
            } else if (currentTab == tagsTabContent) {
                tagsTabLoader.sourceComponent = tagsListComponent;
            } else if (currentTab == searchTabContent) {
                searchTabLoader.sourceComponent = searchComponent;
            } else if (currentTab == settingsTabContent) {
                settingsTabLoader.sourceComponent = settingsComponent;
            }
        }
    }

    Component.onCompleted: {
        evernoteSync.syncFinished.connect(evernoteSyncFinished);
        connectionManager.showConnectionMessage.connect(showMessage);
        if (platformWindow) {
            platformWindow.activeChanged.connect(root.appActiveStateChanged);
        }
    }
    Component.onDestruction: {
        evernoteSync.loginError.disconnect(root.window.showLoginErrorDialog);
        evernoteSync.syncFinished.disconnect(evernoteSyncFinished);
        connectionManager.showConnectionMessage.disconnect(showMessage);
        if (platformWindow) {
            platformWindow.activeChanged.disconnect(root.appActiveStateChanged);
        }
    }
}
