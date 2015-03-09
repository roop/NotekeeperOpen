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

// This page is used like a dialog to get user-input text to be added/modified to a richtext note

Page {
    id: root
    anchors.fill: parent

    property string text: ""
    property string okButtonText: "Ok"
    property string cancelButtonText: "Cancel"
    property bool singleLineInput: false;

    signal okClicked(string text)
    signal cancelClicked(string text)

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TopButtonBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        colorScheme: Qt.white
        Row {
            anchors.centerIn: parent
            spacing: _UI.paddingLarge * 2
            Button {
                text: root.okButtonText
                enabled: (textEditLoader.item != null && textEditLoader.item.text != "" && textEditLoader.item.textHasBeenChangedByUser)
                onClicked: {
                    root.okClicked(textEditLoader.item.text); // emit signal
                    root.pageStack.pop();
                }
                width: titleBar.width / 3
            }
            Button {
                text: root.cancelButtonText
                enabled: (textEditLoader.item != null)
                onClicked: {
                    root.cancelClicked(textEditLoader.item.text); // emit signal
                    root.pageStack.pop();
                }
                width: titleBar.width / 3
            }
        }
    }

    Flickable {
        id: plainTextFlickable
        anchors { fill: parent; topMargin: titleBar.height; }
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width // vertical flicking only
        contentHeight: textEditLoader.height
        Column {
            id: plainTextColumn
            anchors { left: parent.left; top: parent.top; }
            width: parent.width
            Item {
                id: spacer
                height: _UI.paddingLarge
                width: parent.width
            }
            Item {
                anchors { left: parent.left; leftMargin: _UI.paddingLarge; }
                width: plainTextFlickable.width - _UI.paddingLarge * 2;
                height: textEditLoader.height
                Loader {
                    id: textEditLoader
                }
            }
            Item {
                id: flickableFiller
                width: plainTextFlickable.width
                anchors { left: parent.left; leftMargin: _UI.paddingLarge * 4; }
                height: Math.max(0, (plainTextFlickable.height - spacer.height - textEditLoader.height))
                MouseArea {
                    anchors.fill: parent
                    enabled: (textEditLoader.item != null)
                    onClicked: textEditLoader.item.clickedAtEnd();
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: plainTextFlickable
    }

    // If we don't use a component for UITextEdit, when the page gets created, it gets created with empty text, and then text
    // gets set separately, so UITextEdit::textChanged gets called during initialization iteself.
    // So, using a component for UITextEdit so that the text is set correctly during creation of the component.
    Component {
        id: textEditComponent
        TextEditor {
            id: textEdit
            readOnly: false
            width: plainTextFlickable.width - _UI.paddingLarge * 2
            text: internal.text
            color: (_UI.isDarkColorScheme? "#fff" : "#000")
            flickableContentY: plainTextFlickable.contentY
            flickableHeight: plainTextFlickable.height
            flickableContentHeaderHeight: _UI.paddingLarge
            flickableIsMoving: plainTextFlickable.moving
            z: 1000 // so that flickableFiller doesn't grab clicks meant to go to the textedit's children
            virtualKeyboardVisible: _UI.isVirtualKeyboardOpen
            onScrollToYRequested: plainTextFlickable.contentY = y; // where y is the emitted value
            onDisableUserFlicking: plainTextFlickable.interactive = false;
            onEnableUserFlicking: plainTextFlickable.interactive = true;
            onEnterPressed: {
                if (root.singleLineInput) {
                    root.okClicked(textEdit.text); // emit signal
                    root.pageStack.pop();
                }
            }
        }
    }

    tools: ToolBarLayout {
        id: toolbarLayout
    }

    QtObject {
        id: internal
        property string text: ""
    }

    function load(text) {
        internal.text = text;
        textEditLoader.sourceComponent = textEditComponent;
    }

    function openKeyboardWithAllTextSelected() {
        if (textEditLoader.item != null) {
            textEditLoader.item.openKeyboardWithAllTextSelected();
        }
    }

    function beginEditFirstLine() {
        if (textEditLoader.item != null) {
            textEditLoader.item.beginEditFirstLine();
        }
    }
}
