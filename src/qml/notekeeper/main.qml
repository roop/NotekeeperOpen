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

PageStackWindow {
    id: window
    property alias loginErrorDialogStatus: loginErrorDialog.status
    property alias errorMessageDialogStatus: errorMessageDialog.status
    property string buyFromNokiaStoreLink: "http://store.ovi.mobi/content/266755";
    property bool is24hourTimeFormat: (is_symbian?
                                           is_24hour_time_format_symbian :
                                           // In Harmattan, the system locale is available as an MLocale object in the QML var 'locale'
                                           ((locale === undefined || locale.timeFormat24h === undefined || locale.defaultTimeFormat24h === undefined)?
                                                false :
                                                ((locale.timeFormat24h === internal._DEFAULT_TIME_FORMAT_24H)?
                                                     (locale.defaultTimeFormat24h === internal._TWENTYFOURHOUR_TIME_FORMAT_24H) :
                                                     (locale.timeFormat24h === internal._TWENTYFOURHOUR_TIME_FORMAT_24H)
                                                )
                                            )
                                       )
    applicationStatusMessage: (evernoteSync.isSyncing? evernoteSync.syncStatusMessage : "");

    UiStyle {
        id: _UI
        onHarmattanDarkColorSchemeEnabledChanged: {
            theme.inverted = harmattanDarkColorSchemeEnabled;
        }
    }

    BorderImage {
        id: roundedCorners
        property int toolbarHeight: (_UI.isPortrait ? _UI.tabBarHeightPortraitPrivate : _UI.tabBarHeightLandscapePrivate);
        anchors {
            fill: parent;
            topMargin: (window.showStatusBar? _UI.statusBarHeightPrivate : 0) + applicationStatusBarHeight;
            bottomMargin: (window.showToolBar? toolbarHeight : 0);
        }
        source: ((is_symbian && !_UI.isDarkColorScheme)? "images/window/rounded-container-border.sci" : "")
        opacity: ((is_symbian && !_UI.isDarkColorScheme)? 1.0 : 0.0)
        smooth: true
        z: 100
    }

    QueryDialog {
        id: loginErrorDialog
        titleText: "Login failed"
        message: ""
        acceptButtonText: "Ok"
        height: 180
    }

    QueryDialog {
        id: errorMessageDialog
        titleText: "Error"
        message: ""
        acceptButtonText: "Ok"
        height: 180
    }

    InfoBanner {
        id: infoBanner
    }

    function showLoginErrorDialog(message) {
        if (loginErrorDialog.status == _UI.dialogStatus_Closed) {
            loginErrorDialog.message = message;
            loginErrorDialog.open();
        }
    }

    function showErrorMessageDialog(titleText, message) {
        if (errorMessageDialog.status == _UI.dialogStatus_Closed) {
            errorMessageDialog.titleText = titleText;
            errorMessageDialog.message = message;
            errorMessageDialog.open();
        }
    }

    function closeAllDialogs() {
        if (loginErrorDialog.status != _UI.dialogStatus_Closed) {
            loginErrorDialog.close();
        }
        if (errorMessageDialog.status != _UI.dialogStatus_Closed) {
            errorMessageDialog.close();
        }
    }

    function showInfoBanner(message) {
        infoBanner.text = message;
        infoBanner.show();
    }

    function showInfoBannerDelayed(message) {
        delayedInfoBannerOpenTimer.message = message;
        delayedInfoBannerOpenTimer.start();
    }

    function openUrlExternally(url) {
        if (url.indexOf("file:///") == 0) {
            showInfoBanner("Opening file");
        } else if (url.indexOf("mailto") == 0) {
            showInfoBanner("Opening mail");
        } else if (url.indexOf("http://store.ovi.") == 0) {
            showInfoBanner("Opening Nokia Store");
        } else {
            showInfoBanner("Opening link");
        }
        Qt.openUrlExternally(url);
    }

    QueryDialog {
        id: authTokenExpiredDialog
        titleText: "Please re-login"
        acceptButtonText: "Re-login"
        rejectButtonText: "Cancel"
        message: "Notekeeper cannot access your Evernote account because the authorization for " +
                 "Notekeeper has expired. Please re-login to Evernote and authorize Notekeeper again.";
        height: 250
        onAccepted: {
            window.tryToLogout();
        }
    }

    QueryDialog {
        id: movedToOAuthDialog
        titleText: "Please re-login"
        acceptButtonText: "Ok"
        message: "Notekeeper 2.0 uses OAuth for gaining access to your Evernote account. " +
                 "Please re-login to Evernote now to give Notekeeper access to your notes.";
        height: 250
    }

    Timer {
        id: delayedInfoBannerOpenTimer
        running: false
        repeat: false
        interval: 1000
        property string message;
        onTriggered: {
            window.showInfoBanner(message);
        }
    }

    QtObject {
        id: internal
        property bool isLogoutRequested: false

        // constants from MLocale in libmeegotouch
        property int _DEFAULT_TIME_FORMAT_24H: 0;
        property int _TWELVEHOUR_TIME_FORMAT_24H: 1;
        property int _TWENTYFOURHOUR_TIME_FORMAT_24H: 2;
    }

    function onAuthTokenInvalidated() {
        authTokenExpiredDialog.open();
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
        window.pageStack.clear();
        var page = window.pageStack.push(Qt.resolvedUrl("LoginPage.qml"), { window: window });
        var activeUser = qmlDataAccess.activeUser();
        if (activeUser) {
            page.setUsername(activeUser);
        }
        qmlDataAccess.logout();
    }

    function evernoteSyncFinished(success, message) {
        if (internal.isLogoutRequested) {
            logout();
        }
    }

    Component.onCompleted: {
        evernoteSync.syncFinished.connect(evernoteSyncFinished);
        evernoteSync.authTokenInvalid.connect(onAuthTokenInvalidated);
        qmlDataAccess.authTokenInvalid.connect(onAuthTokenInvalidated);

        if (is_harmattan) {
            theme.colorScheme = 3; // Meego's green theme
            theme.inverted = _UI.harmattanDarkColorSchemeEnabled;
        }

        if (is_simulator) { // running in simulator
            var start = pageStack.push(Qt.resolvedUrl(is_harmattan? "StartPage_N9.qml" : "StartPage.qml"), { window: window });
            start.load();
            qmlDataAccess.setReadyForCreateNoteRequests(true);
            return;
        }
        var loggedIn = qmlDataAccess.isLoggedIn();
        if (loggedIn) {
            var startPage = pageStack.push(Qt.resolvedUrl(is_harmattan? "StartPage_N9.qml" : "StartPage.qml"), { window: window });
            startPage.load();
            qmlDataAccess.setReadyForCreateNoteRequests(true);
        } else {
            var loginPage = pageStack.push(Qt.resolvedUrl("LoginPage.qml"), { window: window });
            if (qmlDataAccess.isLoggedOutBecauseLoginMethodIsObsolete()) {
                var activeUser = qmlDataAccess.activeUser();
                if (activeUser) {
                    loginPage.setUsername(activeUser);
                    movedToOAuthDialog.open();
                }
            }
        }
    }

    Component.onDestruction: {
        evernoteSync.syncFinished.disconnect(evernoteSyncFinished);
        evernoteSync.authTokenInvalid.disconnect(onAuthTokenInvalidated);
        qmlDataAccess.authTokenInvalid.disconnect(onAuthTokenInvalidated);
    }
}

