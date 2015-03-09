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
import "humanize.js" as Humanize

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    property variant window;

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property string evernoteUsername: "";
        property bool isValidAccountDataAvailable: false;
        property string userType;
        property string accountIncomingEmail: "";
        property int uploadedBytes: 0;
        property int uploadLimitBytes: 0;
        property variant uploadLimitResetsAt;
        property bool isLogoutRequested: false;
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Account information"
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
                text: "Evernote account"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Logged in as"
                bottomTextRole: "Title"
                bottomText: internal.evernoteUsername
                controlItemType: "Button"
                buttonText: "Logout"
                onButtonClicked: logoutConfirmDialog.open();
            }
            Column {
                width: parent.width
                height: (internal.isValidAccountDataAvailable? implicitHeight : 0)
                opacity: (internal.isValidAccountDataAvailable? 1 : 0)
                Item {
                    width: parent.width
                    height: ((internal.userType != "Unknown")? userTypeSettingsItem.height : 0)
                    opacity: ((internal.userType != "Unknown")? 1 : 0)
                    SettingsItem {
                        id: userTypeSettingsItem
                        width: listView.width
                        topTextRole: "Subtitle"
                        topText: "Account type"
                        bottomTextRole: "Title"
                        bottomText: internal.userType
                        itemClickable: false
                    }
                }
                Item {
                    width: parent.width
                    height: ((internal.accountIncomingEmail != "")? accountIncomingEmailSettingsItem.height : 0)
                    opacity: ((internal.accountIncomingEmail != "")? 1 : 0)
                    SettingsItem {
                        id: accountIncomingEmailSettingsItem
                        width: listView.width
                        topTextRole: "Title"
                        topText: "Email notes to"
                        bottomTextRole: "Subtitle"
                        bottomText: internal.accountIncomingEmail
                        itemClickable: (internal.accountIncomingEmail != "")
                        onItemClicked: { incomingEmailContextMenu.open(); }
                    }
                }
                GenericListHeading {
                    width: listView.width
                    horizontalAlignment: Qt.AlignLeft
                    text: "Monthly usage status"
                }
                GenericListItem {
                    width: listView.width
                    enabled: false
                    height: usageStatusColumn.height + _UI.paddingLarge * 2
                    Column {
                        id: usageStatusColumn
                        anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                            right: parent.right; rightMargin: _UI.paddingLarge;
                            top: parent.top; topMargin: _UI.paddingLarge; }
                        spacing: _UI.paddingLarge;
                        Item {
                            anchors { left: parent.left; right: parent.right; }
                            height: usageStatusProgressBar.height + Math.max(Math.max(textLeft.height, textMid.height), textRight.height)
                            ProgressBar {
                                id: usageStatusProgressBar
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: 270
                                minimumValue: 0
                                maximumValue: internal.uploadLimitBytes
                                value: internal.uploadedBytes
                            }
                            TextLabel {
                                id: textLeft;
                                anchors { top: usageStatusProgressBar.bottom; horizontalCenter: usageStatusProgressBar.left; }
                                text: "0";
                            }
                            TextLabel {
                                id: textMid;
                                anchors { top: usageStatusProgressBar.bottom; horizontalCenter: usageStatusProgressBar.horizontalCenter; }
                                text: Humanize.filesizeformat(internal.uploadLimitBytes / 2, decimal_point_char);
                            }
                            TextLabel {
                                id: textRight;
                                anchors { top: usageStatusProgressBar.bottom; horizontalCenter: usageStatusProgressBar.right; }
                                text: Humanize.filesizeformat(internal.uploadLimitBytes, decimal_point_char);
                            }
                        }
                        TextLabel {
                            anchors { left: parent.left; right: parent.right; }
                            horizontalAlignment: Qt.AlignLeft; verticalAlignment: Qt.AlignVCenter; font.pixelSize: _UI.fontSizeSmall;
                            text: (Humanize.filesizeformat(internal.uploadedBytes, decimal_point_char) + " out of " +
                                   Humanize.filesizeformat(internal.uploadLimitBytes, decimal_point_char) + " used (" +
                                   (internal.uploadLimitBytes? Math.floor((internal.uploadedBytes * 100) / internal.uploadLimitBytes) : 0) + "%)\n" +
                                   "Cycle will reset on " + Qt.formatDateTime(internal.uploadLimitResetsAt, "MMM d, yyyy"))
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: listView
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.pageStack.pop(); }
    }

    QueryDialog {
        id: logoutConfirmDialog
        titleText: "Logging out"
        message: "Are you sure you want to logout?"
        acceptButtonText: "Logout"
        rejectButtonText: "No"
        onAccepted: root.tryToLogout();
        height: 150
    }

    ContextMenu {
        id: incomingEmailContextMenu
        MenuLayout {
            MenuItem {
                text: "Copy email address";
                enabled: (internal.accountIncomingEmail != "")
                onClicked: { clipboard.setText(internal.accountIncomingEmail); }
            }
            MenuItem {
                text: "Send email to this address";
                enabled: (internal.accountIncomingEmail != "")
                onClicked: { window.openUrlExternally("mailto:" + internal.accountIncomingEmail); }
            }
        }
    }

    function load() {
        var loggedIn = qmlDataAccess.isLoggedIn();
        if (is_simulator) {
            loggedIn = true;
        }
        var user = qmlDataAccess.activeUser();
        if (!loggedIn || user == "") {
            internal.evernoteUsername = "Not logged in";
            return;
        }
        internal.evernoteUsername = user;
        var accountData = qmlDataAccess.userAccountData();
        if (accountData.AccountDataValid) {
            internal.isValidAccountDataAvailable = true;
            internal.accountIncomingEmail = (accountData.IncomingEmail? accountData.IncomingEmail : "");
            internal.userType = (accountData.UserType? accountData.UserType : "Unknown");
            if (is_simulator) {
                internal.accountIncomingEmail = "dummy.email@m.evernote.com";
            }
            internal.uploadLimitBytes = (accountData.UploadLimitForThisCycle? accountData.UploadLimitForThisCycle : 0);
            internal.uploadLimitResetsAt = accountData.UploadLimitNextResetsAt;
            internal.uploadedBytes = (accountData.UploadedBytesInThisCycle? accountData.UploadedBytesInThisCycle : 0);
        }
    }

    function tryToLogout() {
        if (evernoteSync.isSyncing) {
            internal.isLogoutRequested = true;
            evernoteSync.cancel();
        } else {
            logout();
        }
    }

    function logout() {
        qmlDataAccess.setReadyForCreateNoteRequests(false);
        root.pageStack.clear();
        var page = root.pageStack.push(Qt.resolvedUrl("LoginPage.qml"), { window: window });
        qmlDataAccess.logout();
    }

    function evernoteSyncFinished(success, message) {
        if (internal.isLogoutRequested) {
            logout();
        }
    }

    Component.onCompleted: {
        evernoteSync.syncFinished.connect(evernoteSyncFinished);
    }
    Component.onDestruction: {
        evernoteSync.syncFinished.disconnect(evernoteSyncFinished);
    }
}
