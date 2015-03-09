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
    property bool isLoggingIn: false;
    property bool isAwaitingLogin: true;
    property string busyMessage: "Talking to Evernote";
    property bool showLogoImage: true;
    property bool isNokiaE6: _UI.isNokiaE6;

    signal startAuth();

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: "#4a8021"
    }

    Item {
        anchors { fill: parent; }

        Image {
            id: loginScreenImage
            source: (is_harmattan?
                         "images/loginscreen/loginscreen_n9.png" :
                         (isNokiaE6?
                              "images/loginscreen/loginscreen_landscape.png" :
                              "images/loginscreen/loginscreen.png"))
            anchors { top: parent.top; left: parent.left; right: parent.right; }
            Image {
                anchors { left: parent.left; top: parent.top; }
                source: "images/loginscreen/open_ribbon.png"
            }
            opacity: (root.showLogoImage? 1 : 0)
        }

        Column {
            id: preLoggingInColumn
            spacing: (is_harmattan? 60 : 24)
            anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                      right: parent.right; rightMargin: _UI.paddingLarge;
                      verticalCenter: (isNokiaE6? parent.verticalCenter : undefined);
                      top: (isNokiaE6? undefined : loginScreenImage.bottom); topMargin: -50; }
            opacity: (root.isAwaitingLogin? 1.0 : 0.0)
            Behavior on opacity { NumberAnimation { duration: 300; } }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Already have " + (is_sandbox? "a Sandbox" : "an Evernote") + " account?"
                color: "white"
                font.pixelSize: _UI.fontSizeLarge
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Sign in"
                width: parent.width * 0.4
                onClicked: root.startAuth(); // emit signal
            }
        }

        Column {
            id: loggingInColumn
            spacing: (is_harmattan? 60 : 24)
            anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                      right: parent.right; rightMargin: _UI.paddingLarge;
                      verticalCenter: (isNokiaE6? parent.verticalCenter : undefined);
                      top: (isNokiaE6? undefined : preLoggingInColumn.top); }
            opacity: (root.isLoggingIn? 1 : 0)
            Behavior on opacity { NumberAnimation { duration: 300; } }
            Item {
                anchors { left: parent.left; right: parent.right; }
                height: loginStatusMessage.height
                TextLabel {
                    id: loginStatusMessage
                    anchors.centerIn: parent
                    text: (root.isLoggingIn? root.busyMessage : " ")
                    opacity: (root.isLoggingIn? 1.0 : 0.0)
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    color: "white"
                }
            }
            Item {
                anchors.horizontalCenter: parent.horizontalCenter
                height: 50
                width: parent.width
                BusyIndicator {
                    id: busyLoggingIn
                    anchors.centerIn: parent
                    property bool shown: root.isLoggingIn
                    running: shown
                    opacity: (shown ? 1.0 : 0.0)
                    colorScheme: Qt.white
                    size: "medium"
                }
            }
        }

        Item {
            id: signupLinkContainer
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: 5; }
            height: Math.max(30, _UI.fontSizeSmall * 2 + 5)
            opacity: (root.isAwaitingLogin? 1.0 : 0.0)
            Behavior on opacity { PropertyAnimation { duration: 500; } }
            TextLabel {
                id: signupText
                anchors.centerIn: parent
                font.pixelSize: _UI.fontSizeSmall
                text: "or<br/><b>Sign up for an Evernote account</b>"
                textFormat: Text.RichText
                horizontalAlignment: Text.AlignHCenter
                color: (signupMouseArea.pressed? "lightblue" : "white")
            }
            MouseArea {
                id: signupMouseArea
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; }
                width: signupText.width
                height: parent.height
                enabled: root.isAwaitingLogin
                onPressed: _UI.playThemeEffect("BasicButton");
                onReleased: _UI.playThemeEffect("BasicButton");
                onClicked: {
                    window.openUrlExternally("https://www.evernote.com/Registration.action");
                }
            }
        }
    }
}
