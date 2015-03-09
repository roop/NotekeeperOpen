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

    TabBar {
        id: tabBar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        TabButton {
            tab: notesTabContent
            text: "Notes"
            iconSource: "images/tabs/notepad.svg"
        }
        TabButton {
            tab: notebooksTabContent
            text: "Notebooks"
            iconSource: "images/tabs/notebook.svg"
        }
        TabButton {
            tab: tagsTabContent
            text: "Tags"
            iconSource: "images/tabs/tag.svg"
        }
    }

    TabGroup {
        id: tabGroup
        anchors { left: parent.left; right: parent.right; top: tabBar.bottom; bottom: parent.bottom }
        Item {
            id: notesTabContent
            property alias loader: notesTabLoader
            property bool loaded: false
            Loader {
                anchors.fill: parent
                id: notesTabLoader
                onLoaded: {
                    item.loadAllNotes();
                    item.containingPage = root;
                    parent.loaded = true;
                }
            }
        }
        Item {
            id: notebooksTabContent
            property alias loader: notebooksTabLoader
            property bool loaded: false
            Loader {
                anchors.fill: parent
                id: notebooksTabLoader
                onLoaded: {
                    item.loadAllNotebooks();
                    item.containingPage = root;
                    parent.loaded = true;
                }
            }
        }
        Item {
            id: tagsTabContent
            property alias loader: tagsTabLoader
            property bool loaded: false
            Loader {
                anchors.fill: parent
                id: tagsTabLoader
                onLoaded: {
                    item.loadAllTags();
                    item.containingPage = root;
                    parent.loaded = true;
                }
            }
        }
        onCurrentTabChanged: {
            if (!currentTab.loaded) {
                if (currentTab == notesTabContent) {
                    currentTab.loader.sourceComponent = notesListComponent;
                } else if (currentTab == notebooksTabContent) {
                    currentTab.loader.sourceComponent = notebooksListComponent;
                } else if (currentTab == tagsTabContent) {
                    currentTab.loader.sourceComponent = tagsListComponent;

                }
            }
        }
    }

    Component {
        id: notesListComponent
        NotesList {
            anchors.fill: parent
            is24hourTimeFormat: internal.is24hourTimeFormat
        }
    }

    Component {
        id: notebooksListComponent
        NotebooksList {
            anchors.fill: parent
        }
    }

    Component {
        id: tagsListComponent
        TagsList {
            anchors.fill: parent
        }
    }

    tools: ToolBarLayout {
        ToolIconButton {
            id: quitButton
            iconPath: (enabled? "images/toolbar/nokia_close_stop.svg" : "images/toolbar/nokia_close_stop_disabled.svg");
            onClicked: Qt.quit();
        }
        ToolIconButton {
            iconId: "toolbar-settings";
            onClicked: {
                var page = root.pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), { window: root.window });
                page.load();
            }
        }
        ToolIconButton {
            iconId: "toolbar-search";
            onClicked: root.pageStack.push(Qt.resolvedUrl("SearchPage.qml"), { window: root.window });
        }
        ToolIconButton {
            iconId: "toolbar-add";
            onClicked: {
                var page = root.pageStack.push(Qt.resolvedUrl("NotePage.qml"), { window: root.window });
                page.delayedLoad("", "");
            }
        }
    }

    Timer {
        id: quitButtonEnableTimer
        running: false
        repeat: false
        interval: 1500
        onTriggered: { quitButton.enabled = true; }
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
        property bool is24hourTimeFormat: (root.window && root.window.is24hourTimeFormat)? true : false;
    }

    function sync() {
        internal.isSyncing = true;
        evernoteSync.startSync();
    }

    function evernoteSyncFinished(success, message) {
        var infoMessage = "Synchronization " + (success? "complete" : "failed");
        if (message.indexOf("upload limit") >= 0) {
            infoMessage = "Synchronization incomplete";
        }
        if (message) {
            infoMessage += "\n" + message;
        }
        root.window.showInfoBannerDelayed(infoMessage);
        internal.isSyncing = false;
    }

    function load() {
        evernoteSync.loginError.connect(root.window.showLoginErrorDialog);
        root.pageStack.depthChanged.connect(pageStackDepthChanged);
        startupSyncTriggerTimer.start();
    }

    function pageStackDepthChanged() {
        if (root.pageStack.depth == 1) {
            quitButton.enabled = false;
            quitButtonEnableTimer.start();
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

    Component.onCompleted: {
        evernoteSync.syncFinished.connect(evernoteSyncFinished);
        connectionManager.showConnectionMessage.connect(showMessage);
    }
    Component.onDestruction: {
        evernoteSync.loginError.disconnect(root.window.showLoginErrorDialog);
        evernoteSync.syncFinished.disconnect(evernoteSyncFinished);
        connectionManager.showConnectionMessage.disconnect(showMessage);
        if (root.pageStack) {
            root.pageStack.depthChanged.disconnect(pageStackDepthChanged);
        }
    }
}
