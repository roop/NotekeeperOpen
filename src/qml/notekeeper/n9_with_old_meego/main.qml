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

import QtQuick 1.0
import QtWebKit 1.0
import com.nokia.meego 1.0
import "../../native/constants.js" as UI

PageStackWindow {
    id: root
    width: 100
    height: 62
    anchors.fill: parent

    initialPage: Page {

        orientationLock: PageOrientation.LockPortrait

        Rectangle {
            anchors.fill: parent
            color: "#fff"
        }

        Rectangle {
            id: titleBar
            anchors { left: parent.left; right: parent.right; top: parent.top; }
            height: UI.HEADER_DEFAULT_HEIGHT_PORTRAIT
            color: "#60a02a"
            Text {
                id: textLabel
                anchors { fill: parent; leftMargin: 10; }
                elide: Text.ElideRight
                wrapMode: Text.Wrap
                text: "Notekeeper can't start"
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                color: "#fff"
                font.pixelSize: titleBar.height * 0.5
                font.family: UI.FONT_FAMILY
            }
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
                settings.defaultFontSize: UI.FONT_DEFAULT
                settings.standardFontFamily: UI.FONT_FAMILY
                smooth: false
                settings.javascriptEnabled: true
                html: "" +
                      "<p><br/></p>" +
                      "<p>" +
                      "<strong>Notekeeper is unable to start properly because required software updates are not applied in your phone.</strong>" +
                      "</p>" +
                      "Notekeeper requires your phone's <em>Meego 1.2 Harmattan</em> software to be upgraded to version <em>PR1.2</em> or <em>PR1.3</em>. " +
                      "Please upgrade your phone's <em>Meego 1.2 Harmattan</em> software and then re-open Notekeeper. " +
                      "</p>" +
                      "<p>" +
                      "To know your phone's current software version, open <em>Settings</em> > <em>About product</em>, and see under <em>Version</em>." +
                      "</p>" +
                      "<p>" +
                      "Please see the " +
                      "<a href=\"null/\" onclick=\"window.qml.openUrl('http://www.nokia.com/global/support/software-update/n9-software-update/');\">" +
                          "Software Update for Nokia N9" +
                      "</a> webpage for more information on updating your phone's software." +
                      "</p>" +
                      "";
                javaScriptWindowObjects: QtObject {
                    WebView.windowObjectName: "qml"
                    function openUrl(url) {
                        Qt.openUrlExternally(url);
                    }
                }
            }
        }
        ScrollDecorator {
            flickableItem: flickable
        }

        tools: ToolBarLayout {
            ToolIcon {
                iconSource: "../images/toolbar/blank.svg";
                enabled: false;
            }

            ToolIcon {
                iconId: "toolbar-view-menu";
                onClicked: {
                    menu.open();
                }
            }
        }

        Menu {
            id: menu
            visualParent: pageStack
            MenuLayout {
                MenuItem {
                    text: "Translate this message";
                    enabled: (locale_lang_code != "en");
                    onClicked: {
                        var text = "Notekeeper is unable to start properly because required software updates " +
                                "are not applied in your phone." +
                                "\n" +
                                "Notekeeper requires your phone's Meego 1.2 Harmattan software " +
                                "to be  upgraded to version PR1.2 or PR1.3. Please upgrade your " +
                                "phone's Meego 1.2 Harmattan software and then re-open Notekeeper. " +
                                "\n" +
                                "To know your phone's current software version, open \"Settings\" > \"About product\", " +
                                "and see under \"Version\". " +
                                "\n" +
                                "Please see http://www.nokia.com/global/support/software-update/n9-software-update/ " +
                                "for more information on updating your phone's software.";
                        Qt.openUrlExternally("http://translate.google.com/m?sl=en&tl=" + locale_lang_code + "&q=" + encodeURIComponent(text));
                    }
                }
                MenuItem {
                    text: "About Notekeeper";
                    onClicked: {
                        root.pageStack.push(Qt.resolvedUrl("AboutNotekeeperPage.qml"));
                    }
                }
            }
        }
    }
}
