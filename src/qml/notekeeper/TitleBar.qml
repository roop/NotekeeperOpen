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

Rectangle {
    id: root
    width: parent.width
    height: implicitHeight
    implicitHeight: (_UI.isPortrait ? _UI.tabBarHeightPortraitPrivate : _UI.tabBarHeightLandscapePrivate)
    color: (is_harmattan ? "#60a02a" : (_UI.isDarkColorScheme ? "#000" : "#6ab12f"))
    property alias text: textLabel.text
    property alias glossOpacity:  bgGlossImage.opacity
    property alias iconPath: iconLoader.iconPath
    property string buttonIconPath: ""
    property bool dismissKeyboardButton: false

    signal buttonClicked()

    UiStyle {
        id: _UI
    }

    Image {
        id: bgGlossImage
        anchors.fill: parent
        source: ((is_symbian && !_UI.isDarkColorScheme)? "images/titlebar/title-bar-gloss.png" : "")
        opacity: ((is_symbian && !_UI.isDarkColorScheme)? 0.8 : 0)
        smooth: true
    }

    Row {
        anchors { fill: parent; rightMargin: 0; leftMargin: 10; }
        Loader {
            id: iconLoader
            property string iconPath
            height: parent.height
            width: (iconPath != ""? height : 0)
            sourceComponent: (iconPath != ""? iconComponent : undefined)
            Component {
                id: iconComponent
                Item {
                    anchors.fill: parent
                    Image {
                        id: icon
                        width : (is_symbian? _UI.graphicSizeSmall : _UI.graphicSizeMedium)
                        height : (is_symbian? _UI.graphicSizeSmall : _UI.graphicSizeMedium)
                        anchors.centerIn: parent
                        source: iconLoader.iconPath
                        smooth: true
                    }
                }
            }
        }
        TextLabel {
            id: textLabel
            height: parent.height
            width: parent.width - iconLoader.width - buttonLoader.width - spacer.width
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            maximumLineCount: 1
            text: ""
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            color: "white"
            font.pixelSize: root.implicitHeight * 0.5
        }
        Item {
            id: spacer
            height: parent.height
            width: 10
        }
        Loader {
            id: buttonLoader
            property string iconPath: ((root.dismissKeyboardButton && _UI.isVirtualKeyboardOpen)?
                                           "images/titlebar/close_keyboard.svg" :
                                           root.buttonIconPath
                                       )
            height: parent.height
            width: (iconPath != ""? (is_symbian? height : height + 10) : 0)
            sourceComponent: (iconPath != ""? buttonComponent : undefined)
            Component {
                id: buttonComponent
                Item {
                    anchors.fill: parent
                    TitleBarButton {
                        anchors { fill: parent; margins: (is_symbian? _UI.paddingMedium : _UI.paddingLarge); }
                        iconPath: buttonLoader.iconPath
                        onClicked: {
                            var isDismissKeyboardButton = (root.dismissKeyboardButton && _UI.isVirtualKeyboardOpen);
                            if (isDismissKeyboardButton) {
                                focusStealer.forceActiveFocus();
                            } else {
                                root.buttonClicked(); /* emit signal */
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: topBorder
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        height: 1
        color: ((_UI.isDarkColorScheme && is_symbian)? "#1080dd" : "transparent")
    }

    Rectangle {
        id: bottomBorder
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; }
        height: 1
        color: ((_UI.isDarkColorScheme && is_symbian)? "#1080dd" : "transparent")
    }

    Item {
        id: focusStealer // to help in dismissing the keyboard
        width: 0
        height: 0
        x: 0
        y: 0
        focus: true
    }
}
