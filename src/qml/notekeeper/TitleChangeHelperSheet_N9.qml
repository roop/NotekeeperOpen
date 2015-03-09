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
import "../native"

Sheet {
    id: root

    signal okClicked(string text)
    signal cancelClicked(string text)

    acceptButtonText: "Save"
    rejectButtonText: "Cancel"

    UiStyle {
        id: _UI
    }

    content: Item {
        anchors.fill: parent
        Column {
            anchors { fill: parent; margins: _UI.paddingLarge; }
            spacing: _UI.paddingLarge
            TextLabel {
                anchors.left: parent.left
                text: "Note title:"
            }
            TextField {
                id: textField
                anchors { left: parent.left; right: parent.right; }
            }
        }
    }

    onAccepted: root.okClicked(textField.text);
    onRejected: root.cancelClicked(textField.text);

    function setText(text) {
        textField.text = text;
    }

    function openKeyboard() {
        textField.cursorPosition = textField.text.length;
        textField.forceActiveFocus();
        textField.openSoftwareInputPanel();
    }
}
