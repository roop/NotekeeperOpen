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

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "About Notekeeper"
        iconPath: "images/titlebar/settings_white.svg"
    }

    Flow {
        anchors { fill: parent; topMargin: titleBar.height; }
        Item {
            width: (_UI.isPortrait? parent.width : parent.width / 2)
            height: (_UI.isPortrait? parent.height / 2 : parent.height)
            Column {
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; }
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 128
                    height: 128
                    source: "images/logo/notekeeper.svg"
                    smooth: true
                }
                TextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: appName + " " + appVersion
                    font { pixelSize: _UI.fontSizeMedium; bold: true; }
                    wrapMode: Text.Wrap
                }
                TextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Open source edition"
                    width: parent.width
                    font { pixelSize: _UI.fontSizeSmall - 2; bold: false; }
                    horizontalAlignment: Text.AlignHCenter
                    textFormat: Text.PlainText
                    color: "gray"
                    wrapMode: Text.Wrap
                }
                TextLabel {
                    property string website: (is_harmattan ? "notekeeperapp.com/n9" : "notekeeperapp.com")
                    property string link: "http://" + website
                    width: parent.width
                    height: Math.max(implicitHeight, 45)
                    text: website
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    textFormat: Text.PlainText
                    color: _UI.colorLink
                    font.pixelSize: _UI.fontSizeSmall;
                    wrapMode: Text.Wrap
                    MouseArea { anchors.fill: parent; onClicked: { window.openUrlExternally(parent.link); } }
                }
            }
        }
        Item {
            width: (_UI.isPortrait? parent.width : parent.width / 2)
            height: (_UI.isPortrait? parent.height / 2 : parent.height)
            Column {
                anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                          right: parent.right; rightMargin: _UI.paddingLarge;
                          verticalCenter: parent.verticalCenter; }
                TextLabel {
                    width: parent.width
                    text: "Credits:"
                    textFormat: Text.PlainText
                    horizontalAlignment: Text.AlignHCenter
                    font { pixelSize: _UI.fontSizeSmall; bold: true; }
                    wrapMode: Text.Wrap
                }
                TextLabel {
                    width: parent.width
                    text: "" +
                          "Roopesh Chander\n" +

                          /* Add additional contributors here */

                          ""
                    horizontalAlignment: Text.AlignHCenter
                    textFormat: Text.PlainText
                    font.pixelSize: _UI.fontSizeSmall;
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.pageStack.pop(); }
    }
}
