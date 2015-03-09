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

import QtQuick 1.0
import com.nokia.meego 1.0
import "../../native/constants.js" as UI

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent

    orientationLock: PageOrientation.LockPortrait

    Rectangle {
        anchors.fill: parent
        color: "#fff"
    }

    Rectangle {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        height: UI.HEADER_DEFAULT_HEIGHT_PORTRAIT
        color: "#60a02a"
        Text {
            id: textLabel
            anchors { fill: parent; leftMargin: 10; }
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            text: "About Notekeeper"
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            color: "#fff"
            font.pixelSize: titleBar.height * 0.5
            font.family: UI.FONT_FAMILY
        }
    }

    Flow {
        anchors { fill: parent; topMargin: titleBar.height; }
        Item {
            width: parent.width
            height: (parent.height / 2)
            Column {
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; }
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 128
                    height: 128
                    source: "../images/logo/notekeeper.svg"
                    smooth: true
                }
                TextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: appName + " " + appVersion
                    font { pixelSize: UI.FONT_DEFAULT; bold: true; }
                    wrapMode: Text.Wrap
                }
                TextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Open source edition"
                    width: parent.width
                    font { pixelSize: UI.FONT_SMALL - 2; bold: false; }
                    horizontalAlignment: Text.AlignHCenter
                    textFormat: Text.PlainText
                    color: "gray"
                    wrapMode: Text.Wrap
                }
                TextLabel {
                    property string link: "http://www.notekeeperapp.com/opensource"
                    width: parent.width
                    height: Math.max(implicitHeight, 45)
                    text: "notekeeperapp.com/opensource"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    textFormat: Text.PlainText
                    color: "blue"
                    font.pixelSize: UI.FONT_SMALL;
                    wrapMode: Text.Wrap
                    MouseArea { anchors.fill: parent; onClicked: { Qt.openUrlExternally(parent.link); } }
                }
            }
        }
        Item {
            width: parent.width
            height: (parent.height / 2)
            // In the actual About page, this part
            // contains Credits
        }
    }

    tools: ToolBarLayout {
        ToolIcon { iconId: "toolbar-back"; onClicked: root.pageStack.pop(); }
    }
}
