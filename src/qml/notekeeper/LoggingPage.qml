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

    QtObject {
        id: internal
        property bool isLoggingEnabled: false;
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Log"
        iconPath: "images/titlebar/settings_white.svg"
    }

    Flickable {
        id: flickable
        anchors { fill: parent; topMargin: titleBar.height; }
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: parent.width
        contentHeight: logText.height
        TextLabel {
            id: logText
            width: parent.width
            text: ""
            wrapMode: Text.Wrap
        }
    }

    ScrollDecorator {
        flickableItem: flickable
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.pageStack.pop(); }
        ToolIconButton { iconId: "toolbar-view-menu"; onClicked: menu.open(); }
    }

    Menu {
        id: menu
        content: MenuLayout {
            MenuItem {
                text: (internal.isLoggingEnabled? "Stop" : "Start") + " logging";
                onClicked: {
                    internal.isLoggingEnabled = (!internal.isLoggingEnabled);
                    qmlDataAccess.saveSetting("Logging/enabled", internal.isLoggingEnabled);
                }
            }
            MenuItem {
                text: "Copy all";
                onClicked: { clipboard.setText(logText.text); }
            }
            MenuItem {
                text: "Clear log";
                onClicked: { clearConfirmDialog.open(); }
            }
        }
    }

    QueryDialog {
        id: clearConfirmDialog
        titleText: "Clearing log"
        message: "Are you sure you want to remove all logged information?"
        acceptButtonText: "Clear"
        rejectButtonText: "No"
        onAccepted: qmlDataAccess.clearLog();
        height: 200
    }

    function load() {
        internal.isLoggingEnabled = qmlDataAccess.retrieveSetting("Logging/enabled");
    }

    function textAddedToLog(text) {
        logText.text += text;
    }

    function logCleared() {
        logText.text = "";
    }

    Component.onCompleted: {
        logText.text = qmlDataAccess.loggedText();
        qmlDataAccess.textAddedToLog.connect(textAddedToLog);
        qmlDataAccess.logCleared.connect(logCleared);
    }

    Component.onDestruction: {
        qmlDataAccess.textAddedToLog.disconnect(textAddedToLog);
        qmlDataAccess.logCleared.disconnect(logCleared);
    }
}
