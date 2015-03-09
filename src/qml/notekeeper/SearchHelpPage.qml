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
        text: "Help on searching"
        iconPath: "images/titlebar/search_white.svg"
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
            html: internal.searchHelp;
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
        property string searchHelp:
            "<style type=\"text/css\">\n" +
            "body { background-color: " + bgColor + "; color: " + textColor + "; }\n" +
            "a    { color: " + linkColor + "; }\n" +
            "</style>\n" +
            "<body>" +

            "<h2>Search options</h2>" +


            "<h3>Note contents</h3>" +

            "<p><b>maurice</b></p>" +
            "<p>Matches notes containing the word \"maurice\" (in upper, lower or mixed case)</p>" +

            "<p><b>amazing maurice</b></p>" +
            "<p>Matches notes that contain both the words \"amazing\" and \"maurice\"</p>" +

            "<p><b>any: amazing maurice</b></p>" +
            "<p>Matches notes that contain either the word \"amazing\" or the word \"maurice\"</p>" +

            "<p><b>\"amazing maurice\"</p></b>" +
            "<p>Matches notes containing this sequence of words - i.e. \"amazing\" followed by \"maurice\"</p>" +

            "<p><b>amazing -maurice</p></b>" +
            "<p>Matches notes that contain the word \"amazing\", but don't contain the word \"maurice\"</p>" +

            "<p><b>amaz\* maur\*</p></b>" +
            "<p>Matches notes that contain both a word starting with \"amaz\" and a word starting with \"maur\"</p>" +

            "<p><b>intitle:amazing</p></b>" +
            "<p>Matches notes with \"amazing\" in the title</p>" +


            "<h3>Tags</h3>" +

            "<p><b>tag:tennis</p></b>" +
            "<p>Matches notes with the \"tennis\" tag</p>" +

            "<p><b>tag:ten*</p></b>" +
            "<p>Matches notes with a tag that starts with \"ten\"</p>" +

            "<p><b>tag:\"boolean algebra\"</p></b>" +
            "<p>Matches notes with the tag named \"boolean algebra\"</p>" +

            "<p><b>tag:river tag:asia</p></b>" +
            "<p>Matches notes that have both the tags \"river\" and \"asia\"</p>" +

            "<p><b>any: tag:river tag:asia</p></b>" +
            "<p>Matches notes that are either tagged \"river\" or tagged \"asia\"</p>" +

            "<p><b>tag:ufo -tag:fake</p></b>" +
            "<p>Matches notes that are tagged \"ufo\" and are not tagged as \"fake\"</p>" +


            "<h3>Notebooks</h3>" +

            "<p><b>notebook:work</p></b>" +
            "<p>Matches notes in the 'work' notebook</p>" +

            "<p><b>notebook:\"My notebook\"</p></b>" +
            "<p>Matches notes in the notebook named \"My notebook\"</p>" +

            "<p><b>notebook:work finance account</p></b>" +
            "<p>Matches notes in the 'work' notebook that contain both the words \"finance\" and \"account\"</p>" +

            "<p><b>any: notebook:work finance account</p></b>" +
            "<p>Matches notes in the 'work' notebook that contain either the word \"finance\" or the word \"account\" (any: does not apply to \"notebook:\" terms)</p>" +

            "</body>";
    }
}
