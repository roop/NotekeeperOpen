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
    width: parent.width;
    height: Math.max(leftField.height, rightField.height);
    property string keyText: "";
    property variant timestamp;
    property bool is24hourTimeFormat: false;
    property real leftFieldWidthProportion: 0.4;

    UiStyle {
        id: _UI
    }

    Item {
        id: leftField
        anchors { left: parent.left; top: parent.top; }
        width: parent.width * root.leftFieldWidthProportion;
        height: leftText.implicitHeight + _UI.paddingLarge * 2
        TextLabel {
            id: leftText
            anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                      top: parent.top; topMargin: _UI.paddingLarge;
                      right: parent.right; rightMargin: _UI.paddingLarge; }
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignHCenter
            text: root.keyText
            wrapMode: Text.Wrap
            font { pixelSize: _UI.fontSizeSmall; weight: Font.Light; }
        }
    }
    Item {
        id: rightField
        anchors { right: parent.right; top: parent.top; }
        width: parent.width * (1 - root.leftFieldWidthProportion);
        height: rightText.implicitHeight + _UI.paddingLarge * 2
        TextLabel {
            id: rightText
            anchors { left: parent.left; leftMargin: 0;
                      top: parent.top; topMargin: _UI.paddingLarge;
                      right: parent.right; rightMargin: _UI.paddingLarge; }
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignHCenter
            text: Qt.formatDateTime(root.timestamp, "MMM d, yyyy, " + (root.is24hourTimeFormat? "hh:mm" : "h:mm ap"))
            wrapMode: Text.Wrap
            font { pixelSize: _UI.fontSizeMedium; weight: Font.Normal; }
        }
    }
}
