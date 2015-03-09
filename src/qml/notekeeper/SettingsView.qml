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
    anchors.fill: parent
    property variant window;
    property variant pageStack;

    ListView {
        id: listView
        anchors { fill: parent; }
        clip: true
        model: VisualItemModel {
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Mobile data usage"
            }
            GenericListItem {
                width: listView.width
                enabled: false
                height: mobileDataDescription.height + _UI.paddingLarge * 2
                TextLabel {
                    id: mobileDataDescription
                    anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                              right: parent.right; rightMargin: _UI.paddingLarge;
                              top: parent.top; topMargin: _UI.paddingLarge; }
                    horizontalAlignment: Qt.AlignLeft; verticalAlignment: Qt.AlignVCenter; font.pixelSize: _UI.fontSizeSmall
                    text: "When no wifi network is available, Notekeeper can make use of your mobile data connection for syncing with the Evernote server."
                    wrapMode: Text.Wrap
                }
            }
            SettingsItem {
                id: settingSyncUsingMobileData
                width: listView.width
                topTextRole: "Title"
                topText: "Sync using mobile data"
                bottomTextRole: "Subtitle"
                bottomText: (switchChecked? "Enabled" : "Disabled")
                controlItemType: "Switch"
                onSwitchCheckedChanged: {
                    qmlDataAccess.saveSetting("Sync/syncUsingMobileData", switchChecked);
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Synchronization"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Title"
                topText: {
                    if (evernoteSync.isSyncing) {
                        return "Sync in progress";
                    } else if (evernoteSync.isRateLimitingInEffect) {
                        return "Please sync after";
                    } else {
                        return "Last synced at";
                    }
                }
                bottomTextRole: "Subitle"
                bottomText: {
                    if (evernoteSync.isSyncing) {
                        return (evernoteSync.syncProgress + "% complete");
                    } else if (evernoteSync.isRateLimitingInEffect) {
                        return Qt.formatDateTime(evernoteSync.rateLimitEndTime, "MMM d, yyyy, " + (internal.is24hourTimeFormat? "hh:mm" : "h:mm ap"));
                    } else if (evernoteSync.lastSyncTime) {
                        return Qt.formatDateTime(evernoteSync.lastSyncTime, "MMM d, yyyy, " + (internal.is24hourTimeFormat? "hh:mm" : "h:mm ap"));
                    }
                    return "Never synced";
                }
                controlItemType: "Button"
                buttonText: "Sync now"
                buttonEnabled: (!evernoteSync.isSyncing && internal.isSyncable)
                onButtonClicked: evernoteSync.startSync();
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Title"
                topText: "Offline notebooks"
                bottomTextRole: "Subtitle"
                bottomText: {
                    var count = qmlDataAccess.offlineNotebooksCount;
                    return (count + (count == 1? " notebook" : " notebooks"));
                }
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: { root.showOfflineNotebooks(); }
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
                text: "Image attachments"
            }
            GenericListItem {
                width: listView.width
                enabled: false
                height: imageResizeDescription.height + _UI.paddingLarge * 2
                TextLabel {
                    id: imageResizeDescription
                    anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                              right: parent.right; rightMargin: _UI.paddingLarge;
                              top: parent.top; topMargin: _UI.paddingLarge; }
                    horizontalAlignment: Qt.AlignLeft; verticalAlignment: Qt.AlignVCenter; font.pixelSize: _UI.fontSizeSmall
                    text: "When adding image attachments to a note, Notekeeper scales down large images."
                    wrapMode: Text.Wrap
                }
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Maximum width or height"
                bottomTextRole: "Title"
                bottomText: internal.imageMaxDimension + " pixels"
                controlItemType: "Button"
                buttonText: "Change"
                onButtonClicked: { imageMaxDimensionPicker.open(); }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Evernote account"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Logged in as"
                bottomTextRole: "Title"
                bottomText: internal.evernoteUsername
                controlItemType: "SubItemIndicator"
                itemClickable: (internal.evernoteUsername != "Not logged in")
                onItemClicked: {
                    var page = root.pageStack.push(Qt.resolvedUrl("AccountInfoPage.qml"), { window: root.window });
                    page.load();
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Color scheme"
            }
            SettingsItem {
                id: settingDarkColorScheme
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Current color scheme is"
                bottomTextRole: "Title"
                bottomText: (qmlDataAccess.isDarkColorSchemeEnabled? "Dark" : "Default")
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: {
                    root.pageStack.push(Qt.resolvedUrl("ColorSchemeSettingsPage.qml"), { window: root.window });
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Advanced"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Title"
                topText: "Advanced settings"
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: {
                    var page = root.pageStack.push(Qt.resolvedUrl("AdvancedSettingsPage.qml"), { window: root.window });
                    page.load();
                }
            }
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
        }
    }

    ScrollDecorator {
        flickableItem: listView
    }

    SelectionDialog {
        id: imageMaxDimensionPicker
        titleText: "Select max width / height"
        model: imageMaxDimensionPickerModel
        onAccepted: {
            if (selectedIndex < 0 || selectedIndex >= internal.allowedImageMaxDimensions.length) {
                return;
            }
            internal.imageMaxDimension = internal.allowedImageMaxDimensions[selectedIndex];
            qmlDataAccess.saveStringSetting("ImageAttachments/maxDimensionInPixels", internal.imageMaxDimension);
        }
    }

    ListModel {
        id: imageMaxDimensionPickerModel
    }

    QtObject {
        id: internal
        property string evernoteUsername;
        property bool isLoggedIn: false;
        property bool isSyncable: (connectionManager.isIapConfigurationWifi || settingSyncUsingMobileData.switchChecked);
        property variant allowedImageMaxDimensions: [ "1200", "2400", "3600", "4800", "6000", "8000" ]
        property string defaultImageMaxDimension: "2400";
        property string imageMaxDimension: defaultImageMaxDimension;
        property bool is24hourTimeFormat: (root.window && root.window.is24hourTimeFormat)? true : false;
    }

    function load() {
        loadEvernoteLoggedInUsername();
        settingSyncUsingMobileData.switchChecked = qmlDataAccess.retrieveSetting("Sync/syncUsingMobileData");

        imageMaxDimensionPickerModel.clear();
        for (var i = 0; i < internal.allowedImageMaxDimensions.length; i++) {
            imageMaxDimensionPickerModel.append({"name": (internal.allowedImageMaxDimensions[i] + " pixels")});
        }

        var imageMaxDimension = qmlDataAccess.retrieveStringSetting("ImageAttachments/maxDimensionInPixels");
        if (internal.allowedImageMaxDimensions.indexOf(imageMaxDimension) < 0) {
            imageMaxDimension = internal.defaultImageMaxDimension;
        }
        internal.imageMaxDimension = imageMaxDimension;
    }

    function loadEvernoteLoggedInUsername() {
        var loggedIn = qmlDataAccess.isLoggedIn();
        if (is_simulator) {
            loggedIn = true;
        }
        var user = qmlDataAccess.activeUser();
        if (!loggedIn || user == "" && !is_simulator) {
            internal.evernoteUsername = "Not logged in";
        } else {
            internal.evernoteUsername = user;
        }
    }

    function showOfflineNotebooks() {
        var page = root.pageStack.push(Qt.resolvedUrl("OfflineNotebooksPickerPage.qml"), { window: window });
        page.load();
    }
}
