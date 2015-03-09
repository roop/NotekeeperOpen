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
    property alias text: textItem.text
    property alias horizontalAlignment: textItem.horizontalAlignment
    property alias elide: textItem.elide

    implicitWidth: ListView.view ? ListView.view.width : _UI.screenWidth
    implicitHeight: textItem.implicitHeight + _UI.paddingSmall * 2

    UiStyle {
        id: _UI
    }

    BorderImage {
        source: _UI.listHeadingImage
        border { left: 28; top: 0; right: 28; bottom: 0 }
        smooth: true
        anchors.fill: parent
    }

    TextLabel {
        id: textItem
        anchors { fill: parent; leftMargin: _UI.paddingLarge; rightMargin: _UI.paddingLarge; topMargin: _UI.paddingSmall; bottomMargin: _UI.paddingSmall; }
        width: parent.width - _UI.paddingLarge * 2
        font {
            family: _UI.fontFamilyRegular
            pixelSize: _UI.fontSizeSmall
            weight: Font.Normal
        }
        color: _UI.listHeaderTextColor
        elide: Text.ElideRight
        horizontalAlignment: Qt.AlignRight
        text: ""
    }
}
