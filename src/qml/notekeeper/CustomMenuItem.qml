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

GenericListItem {
    id: root
    property string imageSource;
    property string heading;
    property string subheading;
    property int imageWidthHeight: (height / 5);
    property int maximumTextWidth: width;
    subItemIndicator: false
    bottomBorder: false
    isMenu: true

    UiStyle {
        id: _UI
    }

    Column {
        anchors.centerIn: parent
        width: parent.width
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: root.imageSource;
            height: root.imageWidthHeight;
            width: root.imageWidthHeight;
            smooth: true
        }
        TextLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.maximumTextWidth
            text: root.heading;
            font { pixelSize: _UI.fontSizeSmall; weight: Font.Bold; }
            horizontalAlignment: Text.AlignHCenter
            color: _UI.colorMenuText
            elide: Text.ElideRight
            wrapMode: Text.WrapAnywhere
            maximumLineCount: 1
        }
        Item { height: _UI.paddingLarge; width: parent.width; }
        TextLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.maximumTextWidth
            text: root.subheading;
            font { pixelSize: _UI.fontSizeSmall; weight: Font.DemiBold; }
            horizontalAlignment: Text.AlignHCenter
            color: _UI.colorMenuText
            elide: Text.ElideRight
            wrapMode: Text.WrapAnywhere
            maximumLineCount: 1
        }
    }
}
