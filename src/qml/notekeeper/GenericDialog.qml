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

Dialog {
    id: root

    property alias titleText: titleAreaText.text
    property variant buttonTexts: []
    property variant cancelButtonEnabled: true

    signal buttonClicked(int index)

    UiStyle {
        id: _UI
        isPopup: true
    }

    title: Item {
        anchors.left: parent.left
        anchors.right: parent.right
        height: _UI.fontSizeLarge + 2 * _UI.paddingLarge
        TextLabel {
            id: titleAreaText
            anchors { fill: parent; margins: _UI.paddingLarge; }
            font { family: _UI.fontFamilyRegular; pixelSize: _UI.fontSizeLarge }
            color: _UI.colorDialogTitle
            elide: Text.ElideRight
            horizontalAlignment: (is_symbian? Text.AlignLeft : Text.AlignHCenter)
            verticalAlignment: Text.AlignVCenter
        }
    }

    buttons: Item {
        id: buttonContainer
        width: parent.width
        height: buttonTexts.length ? (_UI.buttonHeightPrivate + 2 * _UI.paddingMedium) : 0

        Loader {
            anchors.fill: parent
            sourceComponent: (is_harmattan? buttonColumnComponent : buttonRowComponent)
        }
    }

    Component {
        id: buttonRowComponent
        Row {
            id: buttonRow
            anchors.centerIn: parent
            spacing: _UI.paddingLarge
            Repeater {
                model: buttonTexts.length
                Button {
                    width: (buttonContainer.width - buttonRow.spacing * (buttonTexts.length + 3)) / buttonTexts.length
                    text: (buttonTexts[index]? buttonTexts[index] : "")
                    enabled: ((text != "Cancel") || root.cancelButtonEnabled)
                    onClicked: {
                        if (root.status == _UI.dialogStatus_Open) {
                            root.buttonClicked(index)
                            root.close()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: buttonColumnComponent
        Column {
            id: buttonColumn
            anchors.fill: parent
            spacing: _UI.paddingLarge
            Repeater {
                model: buttonTexts.length
                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: (buttonTexts[index]? buttonTexts[index] : "")
                    enabled: ((text != "Cancel") || root.cancelButtonEnabled)
                    onClicked: {
                        if (root.status == _UI.dialogStatus_Open) {
                            root.buttonClicked(index)
                            root.close()
                        }
                    }
                    __dialogButton: true // meego-components-specific private property
                }
            }
        }
    }
}
