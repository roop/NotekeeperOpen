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
import QtWebKit 1.0
import "../native"

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    orientationLock: _UI.pageOrientation_LockPortrait
    property variant pageStack;
    property variant window;
    property variant loginPage;

    signal webPageLoaded(variant page)
    signal callbackUrlCalled(string tempToken, string tempTokenSecret, string oauthVerifier)

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: "white"
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: {
            if (webView.isLoading) {
                return "Loading ...";
            }
            var urlString = webView.url + "";
            if (internal.isLoginWebPage) {
                return "Login to Evernote";
            }
            return "Authorize Notekeeper";
        }
        height: ((_UI.isVirtualKeyboardOpen && !webView.isLoading)? 0 : implicitHeight)
        Behavior on height { NumberAnimation { duration: 200; } }
    }

    Flickable {
        id: flickable
        anchors { fill: parent; topMargin: titleBar.height; }
        clip: true
        contentHeight: webView.height
        contentWidth: webView.width
        WebView {
            id: webView
            anchors { left: parent.left; top: parent.top; }
            preferredWidth: (is_harmattan? 360 : flickable.width)
            preferredHeight: flickable.height
            focus: true
            contentsScale: (is_harmattan? (_UI.screenPortraitWidth / preferredWidth) : 1)
            property bool isLoading: false
            onLoadStarted: {
                isLoading = true;
            }
            onLoadFinished: {
                internal.isLoginWebPage = webTextFieldExists('username');
                isLoading = false;
                var urlString = webView.url + "";
                // console.debug(webView.html);
                if (internal.isLoginWebPage) {
                    root.prepareEvernoteLoginPage();
                }
                root.webPageLoaded(root); // emit signal
            }
            onUrlChanged: {
                var urlString = webView.url + "";
                if ((internal.callbackUrl != "") && (urlString.indexOf(internal.callbackUrl) >= 0)) {
                    onOAuthCallback(urlString);
                }
            }
            javaScriptWindowObjects: QtObject {
                WebView.windowObjectName: "qml"
                function setUsername(username) {
                    root.setUsername(username);
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: flickable
    }

    tools: ToolBarLayout {
        ToolIconButton {
            iconId: "toolbar-back";
            onClicked: {
                if (webView.back.enabled) {
                    webView.back.trigger();
                } else {
                    root.loginPage.onOAuthLoginCancelled();
                    root.pageStack.pop();
                }
            }
        }
        ToolIconButton {
            iconId: (enabled? "toolbar-view-menu" : "");
            iconPath: (enabled? "" : "images/toolbar/blank.svg");
            enabled: (internal.isLoginWebPage && !webView.isLoading)
            onClicked: menu.open();
        }
    }

    Menu {
        id: menu
        content: MenuLayout {
            MenuItem {
                text: "Paste as username"
                onClicked: {
                    if (!webView.isLoading) {
                        root.setWebTextField('username', clipboard.text);
                        setUsername(clipboard.text);
                    }
                }
            }
            MenuItem {
                text: "Paste as password"
                onClicked: {
                    if (!webView.isLoading) {
                        root.setWebTextField('password', clipboard.text);
                    }
                }
            }
        }
    }

    QtObject {
        id: internal
        property string username;
        property string callbackUrl;
        property string tempToken;
        property string tempTokenSecret;
        property bool isLoginWebPage: false;
    }

    Timer {
        id: delayedWebViewScroller // scrolls the webview up after the keyboard flys in so that textfields are visible
        interval: 300
        onTriggered: {
            if (_UI.isNokiaE6) {
                flickable.contentY = 520;
            } else if (is_harmattan) {
                flickable.contentY = 10;
            } else {
                flickable.contentY = 145;
            }
        }
    }

    function setUsername(username) {
        internal.username = username;
    }

    function openOAuthUrl(url, callbackUrl, tempToken, tempTokenSecret) {
        internal.callbackUrl = callbackUrl;
        internal.tempToken = tempToken;
        internal.tempTokenSecret = tempTokenSecret;
        webView.url = url;
    }

    function prepareEvernoteLoginPage() {
        if (is_harmattan) {
            webView.forceActiveFocus();
        }
        var onUsernameLostFocusCode = "try{" +
                                   "var u=document.getElementById('username');" +
                                   "qml.setUsername(u.value);" +
                                   "}catch(e){}";
        webView.evaluateJavaScript("try{" +
                                   "var u=document.getElementById('username');" +
                                   "u.addEventListener(\"blur\", function() {" + onUsernameLostFocusCode + "}, false);" +
                                   (is_harmattan? "u.focus();" : "") +
                                   "}catch(e){}");
        if (_UI.isNokiaE6) {
            flickable.contentY = 520;
        }
    }

    function keyboardVisibleChanged() {
        if (_UI.isVirtualKeyboardOpen) {
            delayedWebViewScroller.start();
        }
    }

    function setWebTextField(id, value) {
        webView.evaluateJavaScript("try{" +
                                   "var f=document.getElementById('" + id + "');" +
                                   "f.value='" + value + "';" +
                                   "}catch(e){}");
    }

    function webTextFieldExists(id) {
        var exists = webView.evaluateJavaScript("document.getElementById('" + id + "')");
        if (exists) {
            return true;
        }
        return false;
    }

    function onOAuthCallback(urlString) {
        var callbackUrlPrefix = internal.callbackUrl + "?";
        if (internal.callbackUrl == "" || urlString.indexOf(callbackUrlPrefix) != 0) {
            return;
        }
        var paramsList = urlString.substr(callbackUrlPrefix.length).split("&");
        var oauthToken = "";
        var oauthVerifier = "";
        for (var i in paramsList) {
            var keyValue = paramsList[i].split("=");
            var key = keyValue[0];
            var value = keyValue[1];
            if (key == "oauth_token") {
                oauthToken = value;
            } else if (key == "oauth_verifier") {
                oauthVerifier = value;
            }
        }
        if (oauthToken != "" && oauthToken == internal.tempToken) {
            root.loginPage.onOAuthCallbackUrlCalled(internal.username, internal.tempToken, internal.tempTokenSecret, oauthVerifier);
            root.pageStack.pop();
        } else {
            root.loginPage.onOAuthLoginCancelled();
            root.pageStack.pop();
        }
    }

    Component.onCompleted: {
        _UI.isVirtualKeyboardOpenChanged.connect(keyboardVisibleChanged);
    }

    Component.onDestruction: {
        _UI.isVirtualKeyboardOpenChanged.disconnect(keyboardVisibleChanged);
    }
}
