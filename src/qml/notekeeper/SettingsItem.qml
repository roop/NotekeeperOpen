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

GenericListItem {
    id: root
    property string topTextRole: "Title";
    property string topText: "";
    property string bottomTextRole: "Subtitle";
    property string bottomText: "";
    property string controlItemType: ""; // "button" or "switch"
    property string buttonText: "";

    property bool buttonEnabled: true;
    property bool switchEnabled: true;
    property bool switchChecked: false;
    property bool itemClickable: false;

    signal itemClicked()
    signal buttonClicked()

    height: Math.max(settingInfoContainer.height + _UI.paddingLarge * 2, (is_symbian? 0 : _UI.listItemImplicitHeight))
    subItemIndicator: (root.controlItemType.toLowerCase() == "subitemindicator")
    enabled: itemClickable
    onClicked: itemClicked()

    UiStyle {
        id: _UI
    }

    Item {
        id: settingInfoContainer
        anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                  right: parent.right; rightMargin: _UI.paddingLarge;
                  verticalCenter: parent.verticalCenter; }
        height: Math.max(settingInfo.height, controlItemLoader.height)
        Column {
            id: settingInfo
            anchors { left: parent.left; right: controlItemLoader.left; rightMargin: _UI.paddingLarge; verticalCenter: parent.verticalCenter; }
            spacing: _UI.paddingMedium
            GenericListItemText {
                width: parent.width
                height: (text? implicitHeight : 0)
                horizontalAlignment: Qt.AlignLeft;
                role: root.topTextRole;
                text: root.topText;
            }
            GenericListItemText {
                width: parent.width
                height: (text? implicitHeight : 0)
                horizontalAlignment: Qt.AlignLeft;
                role: root.bottomTextRole;
                text: root.bottomText;
            }
        }
        Loader {
            id: controlItemLoader
            anchors { right: parent.right; verticalCenter: parent.verticalCenter; }
            sourceComponent: {
                if (root.controlItemType.toLowerCase() == "button") { return buttonComponent; }
                else if (root.controlItemType.toLowerCase() == "switch") { return switchComponent; }
                else if (root.controlItemType.toLowerCase() == "subitemindicator") { return subItemIndicatorSpaceComponent; }
                else { return undefined; }
            }
        }
    }

    Component {
        id: buttonComponent
        AutoWidthButton {
            enabled: root.buttonEnabled
            text: root.buttonText
            onClicked: root.buttonClicked();
        }
    }
    Component {
        id: switchComponent
        Switch {
            enabled: root.switchEnabled
            checked: root.switchChecked
            onCheckedChanged: { root.switchChecked = checked; }
        }
    }
    Component {
        id: subItemIndicatorSpaceComponent
        Item {
            height: _UI.graphicSizeSmall
            width: widthUsedBySubItemIndicator
        }
    }
}
