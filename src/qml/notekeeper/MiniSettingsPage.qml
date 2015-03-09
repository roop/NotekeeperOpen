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

    Rectangle {
        anchors.fill: parent
        color: "white"
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "About"
        iconPath: "images/titlebar/settings_white.svg"
    }

    ListView {
        id: listView
        anchors { fill: parent; topMargin: titleBar.height; }
        clip: true
        model: VisualItemModel {
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "About"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "About Notekeeper"
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: {
                    root.pageStack.push(Qt.resolvedUrl("AboutNotekeeperPage.qml"), { window: root.window });
                }
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Privacy policy"
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: {
                    root.pageStack.push(Qt.resolvedUrl("PrivacyPolicyPage.qml"), { window: root.window });
                }
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Legal"
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: {
                    root.pageStack.push(Qt.resolvedUrl("LegalPage.qml"));
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Connection Status"
            }
            GenericListItem {
                enabled: false
                width: listView.width
                height: connectionStatusDisplay.height
                ConnectionStatusDisplay {
                    id: connectionStatusDisplay
                    anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                              right: parent.right; rightMargin: _UI.paddingLarge;
                              top: parent.top; }
                    bottomSpacing: _UI.paddingMedium
                    isButtonCentered: false
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Logging"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Log messages for debug"
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: {
                    var page = root.pageStack.push(Qt.resolvedUrl("LoggingPage.qml"), { window: root.window });
                    page.load();
                }
            }
        }
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.pageStack.pop(); }
    }


}
