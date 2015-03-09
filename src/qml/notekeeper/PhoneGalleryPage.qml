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
import QtMobility.gallery 1.1

Page {
    id: root
    anchors.fill: parent
    property variant window;

    signal imageClicked(string imageLocalUrlPath)

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property bool clickingEnabled: (!addingImageDialog.isShown)
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Select image to add"
    }

    Loader {
        id: galleryLoader
        anchors { fill: parent; topMargin: titleBar.height; }
        sourceComponent: Component {
            Item {
                anchors.fill: parent
                BusyIndicator {
                    anchors.centerIn: parent
                    running: true
                    colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
                }
            }
        }
    }

    AddingAttachmentDialog {
        id: addingImageDialog
        window: root.window
        titleText: "Adding image"
        onAddingSucceededAndDialogClosed: {
            if (root.pageStack) {
                root.pageStack.pop();
            }
        }
    }

    Component {
        id: galleryComponent
        GridView {
            id: gridView
            anchors.fill: parent
            cellWidth: (_UI.isPortrait? Math.floor(width / 2) : Math.floor(width / 4))
            cellHeight: cellWidth
            clip: true
            cacheBuffer: 0
            onMovingChanged: {
                if (!moving && cacheBuffer == 0) { // when movement stops
                    cacheBuffer = Math.min(width, height);
                }
            }

            model: DocumentGalleryModel {
                rootType: DocumentGallery.Image
                properties: [ "url", "width", "height" ]
                sortProperties: [ "-dateTaken" ]
            }

            delegate: Item {
                width: gridView.cellWidth
                height: gridView.cellHeight
                clip: true
                Image {
                    id: imageDelegateImage
                    property bool isAdding: false;
                    source: {
                        var url = ("" + model.url); // convert to string
                        if (is_simulator) {
                            return "image://localimagethumbnail/" + url;
                        }
                        if (url.indexOf("file:///") == 0) {
                            if (is_harmattan) {
                                return "image://localimagethumbnail/" + url.substr(7); // file:///home/...
                            }
                            return "image://localimagethumbnail/" + url.substr(8); // file:///C:/...
                        }
                        return ("file:///" + url);
                    }
                    anchors.centerIn: parent
                    width: (is_harmattan? 210 : 160)
                    height: (is_harmattan? 210 : 160)
                    fillMode: Image.PreserveAspectFit
                    cache: true
                    asynchronous: true
                }
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: ((imageMouseArea.pressed || imageDelegateImage.isAdding)? "blue" : "transparent")
                    border.width: 4
                }
                MouseArea {
                    id: imageMouseArea
                    anchors.fill: parent
                    enabled: internal.clickingEnabled
                    onClicked: {
                        root.startAppendingImage(model.url);
                    }
                }
            }
        }
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; enabled: internal.clickingEnabled; onClicked: pageStack.pop(); }
    }

    Timer {
        id: delayedLoadTimer
        running: false
        repeat: false
        interval: 400 // a little after the pagestack transition completes
        onTriggered: root.load();
    }

    Timer {
        id: delayedAppendTimer
        property string url;
        running: false
        repeat: false
        interval: 200
        onTriggered: { root.imageClicked(delayedAppendTimer.url); }
    }

    function load() {
        galleryLoader.sourceComponent = galleryComponent;
    }

    function delayedLoad() {
        delayedLoadTimer.start();
    }

    function startAppendingImage(url) {
        addingImageDialog.showDialogTillAddingCompletes();
        delayedAppendTimer.url = url
        delayedAppendTimer.start();
    }
}
