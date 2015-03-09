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
    anchors { left: parent.left; right: parent.right; top: parent.top; }
    height: mainColumn.height + _UI.paddingLarge * 2;
    property int bottomSpacing: 25;
    property bool isButtonCentered: true;
    property bool isPopup: false;

    UiStyle {
        id: _UI
        isPopup: root.isPopup
    }

    Column {
        id: mainColumn

        spacing: _UI.paddingLarge * 3

        anchors { left: parent.left; right: parent.right; top: parent.top; topMargin: _UI.paddingLarge; }
        Item {
            anchors { left: parent.left; right: parent.right; }
            height: connectionNameColumn.height
            Column {
                id: connectionNameColumn
                spacing: _UI.paddingMedium
                anchors { left: parent.left; right: icon.left; top: parent.top; }
                GenericListItemText {
                    role: ((connectionManager.iapConfigurationName == "")? "Title" : "Subtitle")
                    text: connectionManager.connectionStateString +
                              ((connectionManager.iapConfigurationName == "")?
                                   "" : (" to " + (connectionManager.isIapConfigurationWifi? "wifi" : "mobile data") + " network")
                               )
                    isPopup: root.isPopup
                }
                GenericListItemText {
                    role: ((connectionManager.iapConfigurationName == "")? "Subtitle" : "Title")
                    text: connectionManager.iapConfigurationName
                    isPopup: root.isPopup
                }
            }
            Item {
                id: icon
                anchors { right: parent.right; top: parent.top; leftMargin: _UI.paddingSmall; }
                width: _UI.graphicSizeLarge
                height: _UI.graphicSizeLarge
                Image {
                    anchors.centerIn: parent
                    source: ((connectionManager.iapConfigurationName != "")?
                                 (connectionManager.isIapConfigurationWifi?
                                      "images/list/nokia_wlan_on.svg" :
                                      "images/list/nokia_connectivity.svg")
                               : "")
                }
            }
        }

        Column {
            anchors { left: parent.left; right: parent.right; }
            property bool shown: (is_symbian || connectionManager.connectionStateString != "Connected")
                                   // In Meego, hide the Scan Now button when connected, because
                                   // if we're connected to Mobile Data, scanning doesn't make sense
            height: (shown? implicitHeight : 0)
            opacity: (shown? 1 : 0)
            spacing: _UI.paddingMedium
            GenericListItemText {
                anchors { left: parent.left; right: parent.right; }
                role: "Title"
                text: (connectionManager.isScanningForWifi?
                           "Scanning for wifi networks" :
                           (connectionManager.isIapConfigurationWifi? "Wifi network found" : "No known wifi network found")
                       )
                isPopup: root.isPopup
            }
            AutoWidthButton {
                anchors{
                    right: (root.isButtonCentered? undefined : parent.right);
                    horizontalCenter: (root.isButtonCentered? parent.horizontalCenter : undefined);
                }
                text: "Scan now"
                enabled: (connectionManager.isScanningForWifi? false : true)
                onClicked: connectionManager.startScanningForNetworks();
            }
            Item { // spacer
                anchors { left: parent.left; right: parent.right; }
                height: root.bottomSpacing
            }
        }
    }
}
