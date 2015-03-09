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
    property bool enabled: true;
    property bool subItemIndicator: false;
    property bool bottomBorder: true;
    property int widthUsedBySubItemIndicator: (subItemIndicator? (_UI.graphicSizeSmall + _UI.paddingLarge) : 0);

    property bool isMenu: false; // used only in Harmattan
    property string menuPosition: ""; // used only in Harmattan

    implicitHeight: _UI.listItemImplicitHeight

    signal clicked()

    UiStyle {
        id: _UI
    }

    BorderImage {
        id: bgImage
        anchors.fill: parent
        source: ((is_harmattan && root.isMenu)?
                     ("image://theme/meegotouch-list" + (theme.inverted? "-inverted" : "") + "-background-" + root.menuPosition)
                   : ""
                 )
        border {
            left: _UI.borderSizeMedium
            top: _UI.borderSizeMedium
            right: _UI.borderSizeMedium
            bottom: _UI.borderSizeMedium
        }
        opacity: ((is_harmattan && root.isMenu)? 1 : 0)
    }

    BorderImage {
        id: highlight
        anchors.fill: parent
        source: ((is_harmattan && root.isMenu)?
                     ("image://theme/meegotouch-list" + (theme.inverted? "-inverted" : "") + "-background-pressed-" + root.menuPosition)
                   : _UI.listItemHightlightImage
                 )
        border {
            left: _UI.borderSizeMedium
            top: _UI.borderSizeMedium
            right: _UI.borderSizeMedium
            bottom: _UI.borderSizeMedium
        }
        opacity: 0
    }

    NumberAnimation {
        id: dehighlightAnimation
        target: highlight; property: "opacity"; to: 0; duration: 150;
    }

    Rectangle {
        id: borderLine
        height: 1
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; }
        opacity: (root.bottomBorder? 1 : 0)
        color: _UI.colorSeparatorLine
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: root.enabled
        onPressed: { highlight.opacity = 1.0; _UI.playThemeEffect("BasicItem"); }
        onReleased: { dehighlightAnimation.restart(); _UI.playThemeEffect("BasicItem"); }
        onCanceled: { dehighlightAnimation.restart(); }
        onClicked: { root.clicked(); }
    }

    Loader {
        id: subItemIndicatorLoader
        anchors { right: parent.right; rightMargin: _UI.paddingLarge; verticalCenter: parent.verticalCenter; }
        sourceComponent: root.subItemIndicator? subItemIndicatorComponent : undefined;
    }

    Component {
        id: subItemIndicatorComponent
        Image {
            source: _UI.listSubItemIndicatorImage
            sourceSize.width: _UI.graphicSizeSmall
            sourceSize.height: _UI.graphicSizeSmall
        }
    }
}
