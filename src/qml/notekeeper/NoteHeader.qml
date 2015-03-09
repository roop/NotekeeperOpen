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
    width: 300
    height: Math.max(timestampText.implicitHeight + _UI.paddingLarge * 3, favouriteIcon.height + _UI.paddingLarge * 2)
    property bool isFavourite: false;
    property bool showFavouriteControl: true;
    property string timestampString;
    property int leftSpacing: _UI.paddingLarge;
    signal favouritenessChanged(bool favouriteness);
    signal timestampClicked();

    UiStyle {
        id: _UI
    }

    TextLabel {
        id: timestampText
        anchors { left: parent.left;
            leftMargin: root.leftSpacing;
            verticalCenter: parent.verticalCenter
        }
        text: timestampString;
        font { pixelSize: _UI.fontSizeSmall; bold: true; }
        color: (timestampMouseArea.pressed? _UI.colorLink : _UI.colorNoteTimestamp)
        MouseArea {
            id: timestampMouseArea
            anchors.fill: parent
            enabled: (timestampText.text != "")
            onPressed: _UI.playThemeEffect("BasicButton");
            onReleased: _UI.playThemeEffect("BasicButton");
            onClicked: {
                root.timestampClicked();
            }
        }
    }
    Image {
        id: favouriteIcon
        anchors { right: parent.right;
            rightMargin: _UI.paddingLarge * 2;
            verticalCenter: parent.verticalCenter;
            verticalCenterOffset: -2
        }
        width: (is_harmattan? _UI.graphicSizeLarge : _UI.graphicSizeMedium)
        height: width
        source: {
            if (!root.showFavouriteControl) {
                return "";
            }
            if (root.isFavourite) {
                return "images/note/favourite_on" + (favouriteIconMouseArea.pressed? "_pressed" : "") + (_UI.isDarkColorScheme? "_dark" : "") + ".png";
            } else {
                return "images/note/favourite_off" + (favouriteIconMouseArea.pressed? "_pressed" : "") + (_UI.isDarkColorScheme? "_dark" : "") + ".png";
            }
        }
        smooth: true
        MouseArea {
            id: favouriteIconMouseArea
            anchors { fill: parent; margins: (is_harmattan? -_UI.graphicSizeMedium : 0); } // because the icon looks too small to click in harmattan
            enabled: root.showFavouriteControl
            onPressed: _UI.playThemeEffect("BasicButton");
            onReleased: _UI.playThemeEffect("BasicButton");
            onClicked: {
                root.isFavourite = (!root.isFavourite);
                root.favouritenessChanged(root.isFavourite);
            }
        }
    }
}
