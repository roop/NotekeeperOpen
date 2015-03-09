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
    property string text: "";
    property bool checkmarksShown: true;
    property bool checked: false;
    property bool clickable: true;

    signal clicked;

    implicitHeight: selectedItem.height
    height: implicitHeight

    UiStyle {
        id: _UI
    }

    Component {
        id: checkedIconComponent
        Item {
            width: image.width
            height: image.height
            Image {
                id: image
                source: "images/list/checked.svg"
                sourceSize.width: _UI.graphicSizeSmall
                sourceSize.height: _UI.graphicSizeSmall
            }
        }
    }

    Item {
        id: selectedItem
        height: (textLabel.height + _UI.paddingLarge * 2)
        width: root.width
        Item {
            anchors { fill: parent; margins: _UI.paddingLarge; }
            clip: true
            Loader {
                id: iconLoader
                width: (root.checkmarksShown? _UI.graphicSizeSmall : 0)
                height: (root.checkmarksShown? _UI.graphicSizeSmall : 0)
                sourceComponent: ((root.checkmarksShown && root.checked) ? checkedIconComponent : undefined)
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; }
            }
            TextLabel {
                id: textLabel
                width: (root.width - iconLoader.width - _UI.paddingLarge * 3)
                horizontalAlignment: Text.AlignLeft
                anchors { verticalCenter: parent.verticalCenter; left: iconLoader.right; leftMargin: _UI.paddingLarge; }
                text: root.text
                wrapMode: Text.Wrap
            }
        }
        MouseArea {
            anchors.fill: parent
            enabled: root.clickable
            onClicked: root.clicked();
        }
    }
}
