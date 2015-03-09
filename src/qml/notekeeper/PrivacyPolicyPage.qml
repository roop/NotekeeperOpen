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
    property variant window;

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Privacy policy"
        iconPath: "images/titlebar/settings_white.svg"
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
            preferredWidth: flickable.width
            preferredHeight: flickable.height
            settings.standardFontFamily: _UI.fontFamilyRegular;
            html: internal.privacyPolicy;
            javaScriptWindowObjects: QtObject {
                WebView.windowObjectName: "qml"
                function openUrl(url) {
                    window.openUrlExternally(url);
                }
            }
            settings.defaultFontSize: (is_harmattan? _UI.fontSizeMedium : _UI.fontSizeSmall)
        }
    }

    ScrollDecorator {
        flickableItem: flickable
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.pageStack.pop(); }
    }

    QtObject {
        id: internal
        property string bgColor: (_UI.isDarkColorScheme? "#000" : "#fff")
        property string textColor: (_UI.isDarkColorScheme? "#fff" : "#000")
        property string linkColor: _UI.colorLinkString
        property string privacyPolicy:
            "<style type=\"text/css\">\n" +
            "body { background-color: " + bgColor + "; color: " + textColor + "; }\n" +
            "a    { color: " + linkColor + "; }\n" +
            "</style>\n" +
            "<body>" +

            "<h1>Privacy Policy</h1>" +
            "" +
            "<h2>Notekeeper versions 2.0 and later</h2>" +
            "" +
            "<h3>Authentication</h3>" +
            "" +
            "<p>Notekeeper uses Evernote's OAuth authentication to access your Evernote account. " +
            "OAuth is an authentication protocol that allows users to approve an application " +
            "to access and/or modify their data without sharing their password.</p>" +
            "" +
            "<p>Notekeeper embeds a web browser within itself that allows you to login in the " +
            "Evernote website and grant permission to Notekeeper to access your notes. Notekeeper " +
            "does not see your account password during this authorization process.</p>" +
            "" +
            "<p>For more information on OAuth 1.0, please see the " +
            "<a href=\"null/\" onclick=\"window.qml.openUrl('http://hueniverse.com/oauth/guide/intro/');\">" +
                "Introduction to OAuth 1.0" +
            "</a> " +
            "from Hueniverse. OAuth as used in Evernote is discussed in " +
            "<a href=\"null\" onclick=\"window.qml.openUrl('http://blog.evernote.com/tech/2012/04/24/security-enhancements-for-third-party-authentication/');\">" +
                "this post" +
            "</a> " +
            "in the Evernote techblog.</p>" +
            "" +
            "<h3>Revoking permission</h3>" +
            "" +
            "<p>You can at any time revoke the permission you have given Notekeeper " +
            "by signing-in in the Evernote website and going to Settings > Applications (Direct link: " +
            "<a href=\"null\" onclick=\"window.qml.openUrl('https://www.evernote.com/AuthorizedServices.action');\">" +
                "https://www.evernote.com/AuthorizedServices.action" +
            "</a>)." +
            "</p>" +
            "" +
            "<p>After you revoke the permission, Notekeeper will not be able to access your " +
            "Evernote account. When Notekeeper detects this, it will offer you an opportunity " +
            "to login to Evernote and give permissions to Notekeeper again.</p>" +
            "" +
            "<h3>Data transmission</h3>" +
            "" +
            "<p>Notekeeper never communicates any information to 3rd-party servers except for " +
            "communication with Evernote. All communication with Evernote is over an encrypted " +
            "channel (https). " +
            "" +
            "<h3>Storage</h3>" +
            "" +
            "<p>Your Evernote username and notes data are stored without encryption in the phone. " +
            "Tokens granted by Evernote to authenticate with your Evernote account are " +
            "stored encrypted in the phone.</p>" +
            "" +
            "<p>When you uninstall Notekeeper from your phone, all data stored by Notekeeper " +
            "is completely removed from your phone. (The note attachments that you explicitly " +
            "saved using the 'Save to phone' option in Notekeeper are not removed during " +
            "uninstallation.)</p>" +
            "" +

            "</body>";
    }
}
