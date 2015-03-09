/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

// Adapted from qt-components/src/symbian/extras/SearchBox.qml

import QtQuick 1.1
import "../native"

Item {
    id: root

    property bool backButton: false
    property bool rightButton: false
    property string rightButtonText: ""
    property bool rightButtonEnabled: false
    property alias placeHolderText: placeHolderText.text
    property alias searchText: searchTextInput.text
    property alias maximumLength: searchTextInput.maximumLength

    signal clearClicked()
    signal backClicked()
    signal rightButtonClicked()

    implicitWidth: Math.max(80, _UI.screenWidth)
    implicitHeight: _UI.tabBarHeightPortraitPrivate

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property int animationtime: 250
    }

    BorderImage {
        anchors.fill: parent
        source: _UI.tabBarBgImage
        border {
            left: _UI.borderSizeMedium
            top: _UI.borderSizeMedium
            right: _UI.borderSizeMedium
            bottom: _UI.borderSizeMedium
        }
    }

    ToolIconButton {
        id: backToolButton; objectName: "backToolButton"
        iconId: "toolbar-back"
        height: _UI.tabBarHeightPortraitPrivate
        width: _UI.tabBarHeightPortraitPrivate
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: backButton
        colorScheme: Qt.white
        onClicked: root.backClicked()
    }

    FocusScope {
        id: textPanel
        anchors.left: backButton ? backToolButton.right : parent.left
        anchors.leftMargin: backButton ? 0 : _UI.paddingLarge
        anchors.right: rightButton ? rightTabButton.left : parent.right
        anchors.rightMargin: _UI.paddingLarge
        anchors.verticalCenter: parent.verticalCenter
        height: _UI.textFieldHeightPrivate
        property int hSpacing: (is_harmattan? _UI.paddingLarge : _UI.paddingSmall)

        BorderImage {
            id: frame
            anchors.fill: parent
            source: searchTextInput.activeFocus ? _UI.textFieldBgHighlighted : _UI.textFieldBgEditable
            border {
                left: _UI.borderSizeMedium
                top: _UI.borderSizeMedium
                right: _UI.borderSizeMedium
                bottom: _UI.borderSizeMedium
            }
            smooth: true
        }

        Image {
            id: searchIndicator
            sourceSize.width: _UI.graphicSizeSmall
            sourceSize.height: _UI.graphicSizeSmall
            fillMode: Image.PreserveAspectFit
            smooth: true
            anchors.left: textPanel.left
            anchors.leftMargin: textPanel.hSpacing
            anchors.verticalCenter: textPanel.verticalCenter
            source: _UI.searchIndicatorIcon
        }

        TextInput {
            id: searchTextInput; objectName: "searchTextInput"
            anchors.left: searchIndicator.right
            anchors.leftMargin: textPanel.hSpacing
            anchors.right: clearButton.left
            anchors.rightMargin: textPanel.hSpacing
            anchors.verticalCenter: textPanel.verticalCenter
            clip: true
            color: "black"
            selectByMouse: true
            font { family: _UI.fontFamilyRegular; pixelSize: _UI.fontSizeMedium }
            activeFocusOnPress: false
            inputMethodHints: Qt.ImhNoPredictiveText
            onTextChanged: {
                if (text) {
                    clearButton.state = "ClearVisible"
                } else {
                    clearButton.state = "ClearHidden"
                }
            }
            onActiveFocusChanged: {
                if (!searchTextInput.activeFocus) {
                    searchTextInput.closeSoftwareInputPanel()
                }
            }
        }
        MouseArea {
            id: searchMouseArea
            anchors {
                left: textPanel.left;
                right: clearButton.state=="ClearHidden" ? textPanel.right : clearButton.left
                verticalCenter : textPanel.verticalCenter
            }
            height: textPanel.height
            onPressed: {
                if (!searchTextInput.activeFocus) {
                    searchTextInput.forceActiveFocus()
                }
            }
            onClicked: {
                searchTextInput.openSoftwareInputPanel()
                _UI.playThemeEffect("PopupOpen");
            }
        }

        TextLabel {
            id: placeHolderText; objectName: "placeHolderText"
            anchors.left: searchIndicator.right
            anchors.leftMargin: _UI.paddingMedium
            anchors.right: clearButton.left
            anchors.rightMargin: _UI.paddingMedium
            anchors.verticalCenter: textPanel.verticalCenter
            color: _UI.placeholderTextColor
            font: searchTextInput.font
            visible: (!searchTextInput.activeFocus) && (!searchTextInput.text) && text
        }

        Image {
            id: clearButton; objectName: "clearButton"
            height: _UI.graphicSizeSmall
            width: _UI.graphicSizeSmall
            anchors.right: textPanel.right
            anchors.rightMargin: textPanel.hSpacing
            anchors.verticalCenter: textPanel.verticalCenter
            state: "ClearHidden"
            source: clearMouseArea.pressed ? _UI.clearIconPressed : _UI.clearIconNormal

            MouseArea {
                id: clearMouseArea
                anchors.fill: parent
                onClicked: {
                    searchTextInput.focus = false
                    searchTextInput.cursorVisible = false
                    searchTextInput.closeSoftwareInputPanel()
                    searchTextInput.text = ""
                    root.clearClicked()
                    clearButton.state = "ClearHidden"
                }
            }

            states: [
                State {
                    name: "ClearVisible"
                    PropertyChanges {target: clearButton; opacity: 1}
                },
                State {
                    name: "ClearHidden"
                    PropertyChanges {target: clearButton; opacity: 0}
                }
            ]

            transitions: [
                Transition {
                    from: "ClearVisible"; to: "ClearHidden"
                    NumberAnimation { properties: "opacity"; duration: internal.animationtime; easing.type: Easing.Linear }
                },
                Transition {
                    from: "ClearHidden"; to: "ClearVisible"
                    NumberAnimation { properties: "opacity"; duration: internal.animationtime; easing.type: Easing.Linear }
                }
            ]
        }
    }

    AutoWidthButton {
        id: rightTabButton;
        anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: _UI.paddingLarge; }
        text: root.rightButtonText;
        enabled: (root.rightButton && root.rightButtonEnabled);
        visible: root.rightButton;
        colorScheme: Qt.white
        onClicked: root.rightButtonClicked();
    }
}
