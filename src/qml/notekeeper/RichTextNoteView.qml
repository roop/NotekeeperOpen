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

Item {
    id: root
    property variant updatedTime: 0;
    property bool isFavourite: false;
    property bool showFavouriteControl: true;
    property string title: "";
    property string html: "";
    property variant checkboxStates;
    property bool is24hourTimeFormat: false;

    signal favouritenessChanged(bool favouriteness);
    signal timestampClicked();
    signal imageClicked(int index);
    signal attachmentClicked(int index);
    signal richTextNoteTextClicked();

    UiStyle {
        id: _UI
    }

    Flickable {
        id: richTextFlickable
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: webView.width
        contentHeight: richTextColumn.height
        Column {
            id: richTextColumn
            NoteHeader {
                id: noteHeader
                anchors { left: parent.left; }
                width: root.width
                leftSpacing: _UI.paddingLarge
                timestampString: Qt.formatDateTime(root.updatedTime, "dddd, MMM d, yyyy\n" + (root.is24hourTimeFormat? "hh:mm" : "h:mm ap"));
                isFavourite: root.isFavourite
                showFavouriteControl: root.showFavouriteControl
                onFavouritenessChanged: root.favouritenessChanged(favouriteness);
                onTimestampClicked: root.timestampClicked();
            }
            Rectangle {
                anchors.left: parent.left
                width: webView.width
                height: 1
                color: "gray"
                border.width: 0
            }
            TextLabel {
                text: root.title
                width: (root.width - _UI.paddingLarge * 2)
                anchors { left: parent.left; leftMargin: _UI.paddingLarge; bottomMargin: _UI.paddingLarge; }
                font { family: _UI.fontFamilyRegular; pixelSize: _UI.fontSizeMedium }
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignVCenter
            }
            Rectangle {
                anchors.left: parent.left
                width: webView.width
                height: 1
                color: "gray"
                border.width: 0
            }
            WebView {
                id: webView
                anchors.left: parent.left
                preferredWidth: richTextFlickable.width
                preferredHeight: richTextFlickable.height
                property string bgColor: (_UI.isDarkColorScheme? "#000" : "#fff")
                property string textColor: (_UI.isDarkColorScheme? "#fff" : "#000")
                property string linkColor: _UI.colorLinkString
                html: "" +
                      "<style type=\"text/css\">\n" +
                      "body { background-color: " + bgColor + "; color: " + textColor + "; }\n" +
                      "a    { color: " + linkColor + "; }\n" +
                      ".attachment-box { margin: 10px 10px 10px 0; background-color: " + (_UI.isDarkColorScheme? "#3d3d3d" : "#ededed") + "; border: 1px solid " + (_UI.isDarkColorScheme? "#ededed" : "#3d3d3d") + "; }\n" +
                      "</style>\n" +
                      "<body onclick=\"window.qml.bodyClicked();\" >" + root.html + "</body>"
                settings.defaultFontSize: _UI.fontSizeMedium
                settings.minimumFontSize: _UI.fontSizeSmall
                settings.standardFontFamily: _UI.fontFamilyRegular
                smooth: false
                settings.javascriptEnabled: true
                javaScriptWindowObjects: QtObject {
                    WebView.windowObjectName: "qml"
                    function openUrlExternally(url) {
                        webView.linkClicked = true;
                        webView.clickedUrl = url;
                        htmlItemClickHandlingTimer.start();
                    }
                    function checkboxClicked(index) {
                        webView.checkboxClicked = true;
                        webView.clickedIndex = index;
                        htmlItemClickHandlingTimer.start();
                    }
                    function imageClicked(index) {
                        webView.imageClicked = true;
                        webView.clickedIndex = index;
                        htmlItemClickHandlingTimer.start();
                    }
                    function attachmentClicked(index) {
                        webView.attachmentClicked = true;
                        webView.clickedIndex = index;
                        htmlItemClickHandlingTimer.start();
                    }
                    function bodyClicked() {
                        webView.bodyClicked = true;
                        htmlItemClickHandlingTimer.start();
                    }
                }
                property bool linkClicked: false;
                property bool checkboxClicked: false;
                property bool imageClicked: false;
                property bool attachmentClicked: false;
                property bool bodyClicked: false;
                property int clickedIndex: 0;
                property string clickedUrl: "";
                Timer {
                    // we have to detect what all actions apply for a click
                    id: htmlItemClickHandlingTimer
                    running: false
                    repeat: false
                    interval: 200
                    onTriggered: {
                        if (webView.checkboxClicked) {
                            var _checkboxStates = checkboxStates;
                            _checkboxStates[webView.clickedIndex] = (!checkboxStates[webView.clickedIndex]);
                            root.checkboxStates = _checkboxStates;
                            resetAll();
                            return;
                        }
                        if (webView.linkClicked && webView.imageClicked) {
                            openImageOrLinkContextMenu.clickedIndex = webView.clickedIndex;
                            openImageOrLinkContextMenu.clickedUrl = webView.clickedUrl;
                            openImageOrLinkContextMenu.open();
                            resetAll();
                            return;
                        }
                        if (webView.linkClicked) {
                            window.openUrlExternally(webView.clickedUrl);
                            resetAll();
                            return;
                        }
                        if (webView.imageClicked) {
                            root.imageClicked(webView.clickedIndex);
                            resetAll();
                            return;
                        }
                        if (webView.attachmentClicked) {
                            root.attachmentClicked(webView.clickedIndex);
                            resetAll();
                            return;
                        }
                        if (webView.bodyClicked) {
                            root.richTextNoteTextClicked();
                            resetAll();
                            return;
                        }
                        resetAll();
                    }
                    function resetAll() {
                        webView.linkClicked = false;
                        webView.checkboxClicked = false;
                        webView.imageClicked = false;
                        webView.attachmentClicked = false;
                        webView.bodyClicked = false;
                        webView.clickedIndex = 0;
                        webView.clickedUrl = "";
                    }
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: richTextFlickable
    }

    ContextMenu {
        id: openImageOrLinkContextMenu
        property int clickedIndex: 0;
        property string clickedUrl: "";
        MenuLayout {
            MenuItem {
                text: "Open Image";
                onClicked: root.imageClicked(openImageOrLinkContextMenu.clickedIndex);
            }
            MenuItem {
                text: "Open link";
                onClicked: window.openUrlExternally(openImageOrLinkContextMenu.clickedUrl);
            }
            MenuItem {
                text: "Cancel";
                onClicked: openImageOrLinkContextMenu.close();
            }
        }
    }

    function scrollToBottom() {
        richTextFlickable.contentY = richTextFlickable.contentHeight - richTextFlickable.height;
    }
}
