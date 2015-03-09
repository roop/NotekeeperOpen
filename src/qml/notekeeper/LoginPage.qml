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
    height: (_UI.screenHeight - toolbarLayout.height - _UI.statusBarHeightPrivate)
    anchors { left: parent.left; top: parent.top; right: parent.right; }
    orientationLock: _UI.pageOrientation_LockPortrait
    property variant window;

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property int formMovedUpSpace: 0
        property bool isLoggingIn: false;
        property bool isDone: false;
        property bool isAwaitingLogin: (!isLoggingIn && currentFullSyncProgress == 0 && !isDone)
        property int currentFullSyncProgress: 0
        property bool isWaitingForFirstChunk: false
        property string busyMessage: "Talking to Evernote"
    }

    LoginScreen {
        id: loginScreen
        anchors.fill: parent;
        isLoggingIn: internal.isLoggingIn
        isAwaitingLogin: (!internal.isLoggingIn && !internal.isDone)
        busyMessage: internal.busyMessage
        onStartAuth: root.startAuth();
    }

    tools: ToolBarLayout {
        id: toolbarLayout
        ToolIconButton {
            iconPath: (enabled? "images/toolbar/nokia_close_stop.svg" : "images/toolbar/blank.svg");
            enabled: is_symbian;
            onClicked: Qt.quit();
        }
        ToolIconButton {
            iconId: "toolbar-settings";
            onClicked:  root.pageStack.push(Qt.resolvedUrl("MiniSettingsPage.qml"), { window: root.window });
        }
    }

    QueryDialog {
        id: firstLoginWarningDialog
        titleText: (is_symbian? "Using mobile data for first login" : "Wifi recommended")
        acceptButtonText: "Proceed"
        rejectButtonText: "Cancel"
        message: "The first sign-in can involve a lot of data transfer, so use of wifi is recommended." +
                 (is_symbian? "\n\nIn case your wifi is already up, please tap 'Cancel', connect to the wifi network on your phone and then sign-in again." : "")
        height: 360
        onAccepted: root.startLoginAsync();
    }

    function startAuth() {
        internal.isLoggingIn = true;
        var showFirstLoginDialog = false;
        if (is_symbian) {
            showFirstLoginDialog = ((!qmlDataAccess.hasUserLoggedInBefore()) && (!connectionManager.isIapConfigurationWifi))
        }
        if (is_harmattan) {
            var isConnectedToWifi = (connectionManager.isIapConfigurationWifi && (connectionManager.connectionStateString == "Connected"));
            showFirstLoginDialog = ((!qmlDataAccess.hasUserLoggedInBefore()) && (!isConnectedToWifi));
        }
        if (showFirstLoginDialog) {
            firstLoginWarningDialog.open();
            internal.isLoggingIn = false;
            return;
        }
        startLoginAsync();
    }

    function startLoginAsync() {
        internal.isLoggingIn = true;
        evernoteOAuth.startAuthentication();
    }

    function onOpenOAuthUrl(urlString, callbackUrlString, tempToken, tempTokenSecret) {
        var component = Qt.createComponent("OAuthWebHelperPage.qml");
        if (component.status == Component.Ready) {
            var page = component.createObject(root);
            page.openOAuthUrl(urlString, callbackUrlString, tempToken, tempTokenSecret);
            if (root.pageStack.depth > 1) {
                // pop off minisettings page or other pages until we get to this login page
                root.pageStack.pop(root, /*immediate*/ true);
            }
            root.pageStack.push(page, { pageStack: root.pageStack, window: root.window, loginPage: root });
        } else if (component.status == Component.Error) {
            console.debug("OAuthWebHelperPage: " + component.errorString());
        }
    }

    function onOAuthLoginCancelled() {
        internal.isLoggingIn = false;
        internal.isDone = false;
        qmlDataAccess.clearCookieJar();
        internal.busyMessage = "Talking to Evernote";
    }

    function onOAuthCallbackUrlCalled(username, tempToken, tempTokenSecret, oauthVerifier) {
        if (qmlDataAccess.hasUserLoggedInBeforeByName(username)) {
            internal.busyMessage = "Just a moment";
        } else {
            internal.busyMessage = "Accessing your notes";
        }
        evernoteOAuth.continueAuthentication(tempToken, tempTokenSecret, oauthVerifier);
    }

    function onLoggedIn() {
        if (qmlDataAccess.hasUserLoggedInBefore()) {
            showStartPage();
        } else {
            // let's wait until we have atleast a few notes to show
            internal.isWaitingForFirstChunk = true;
            evernoteSync.startSync();
        }
    }

    function onGotFirstChunk() {
        if (internal.isWaitingForFirstChunk) {
            showStartPage();
        }
    }

    function showStartPage() {
        internal.isDone = true;
        root.window.closeAllDialogs();
        if (root.pageStack.depth > 1) {
            // pop off minisettings page or other pages until we get to this login page
            root.pageStack.pop(root, /*immediate*/ true);
        }
        var startPage = pageStack.push(Qt.resolvedUrl(is_harmattan? "StartPage_N9.qml" : "StartPage.qml"), { window: window });
        startPage.load();
        qmlDataAccess.setReadyForCreateNoteRequests(true);
    }

    function evernoteSyncFinished(success, message) {
        if (!success) {
            internal.isLoggingIn = false;
            internal.isDone = false;
            internal.currentFullSyncProgress = 0;
            if (root.window.loginErrorDialogStatus == _UI.dialogStatus_Closed) { // only if not a login error
                root.window.showLoginErrorDialog(message);
            }
        }
    }

    function evernoteLoginError(message) {
        internal.isLoggingIn = false;
        internal.isDone = false;
        internal.currentFullSyncProgress = 0;
        root.window.showLoginErrorDialog(message);
        qmlDataAccess.clearCookieJar();
        internal.busyMessage = "Talking to Evernote";
    }

    function showMessage(code, title, msg) {
        var warningEnabled = true;
        if (code == "UNUSUAL_ACCESS_POINT_ORDERING") {
            warningEnabled = (!qmlDataAccess.retrieveSetting("Warn/dontWarnOnUnusualIAPOrdering"));
        }
        if (warningEnabled) {
            root.window.showErrorMessageDialog(title, msg);
        }
    }

    Component.onCompleted: {
        evernoteOAuth.loggedIn.connect(onLoggedIn);
        evernoteSync.firstChunkDone.connect(onGotFirstChunk);
        evernoteSync.loginError.connect(evernoteLoginError);
        evernoteSync.syncFinished.connect(evernoteSyncFinished);
        evernoteOAuth.openOAuthUrl.connect(onOpenOAuthUrl);
        evernoteOAuth.error.connect(evernoteLoginError);
        connectionManager.showConnectionMessage.connect(showMessage);
    }

    Component.onDestruction: {
        evernoteOAuth.loggedIn.disconnect(onLoggedIn);
        evernoteSync.firstChunkDone.disconnect(onGotFirstChunk);
        evernoteSync.loginError.disconnect(evernoteLoginError);
        evernoteSync.syncFinished.disconnect(evernoteSyncFinished);
        evernoteOAuth.openOAuthUrl.disconnect(onOpenOAuthUrl);
        evernoteOAuth.error.disconnect(evernoteLoginError);
        connectionManager.showConnectionMessage.disconnect(showMessage);
    }
}
