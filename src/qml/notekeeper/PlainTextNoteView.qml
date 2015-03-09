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
import TextHelper 1.0

Item {
    id: root
    property int imageSpacing: _UI.paddingLarge
    property variant updatedTime: 0;
    property bool isFavourite: false;
    property bool showFavouriteControl: true;
    property bool isReadOnly: false;
    property alias text: plainTextViewTextEdit.text;
    property alias textHasBeenChangedByUser: plainTextViewTextEdit.textHasBeenChangedByUser;
    property bool isImageExists: false;
    property variant imagesModel;
    property alias virtualKeyboardVisible: plainTextViewTextEdit.virtualKeyboardVisible;
    property bool is24hourTimeFormat: false;

    signal favouritenessChanged(bool favouriteness);
    signal timestampClicked();
    signal imageClicked(int index);

    onImagesModelChanged: {
        plainTextViewImageContainer.updatePosition();
    }

    UiStyle {
        id: _UI
    }

    Flickable {
        id: plainTextFlickable
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width // vertical flicking only
        contentHeight: plainTextColumn.height
        RuledPaper {
            id: ruledPaperBg
            anchors.fill: plainTextColumn
            fontFamily: plainTextViewTextEdit.fontFamily
            fontPixelSize: plainTextViewTextEdit.fontPixelSize
            lineColor: (_UI.isDarkColorScheme? "#5a5a5a" : "lightblue")
            penWidth: 1
            headerHeight: noteHeader.height
            Item {
                id: noteMargin
                anchors { left: parent.left; leftMargin: _UI.paddingLarge * 3; top: parent.top; bottom: parent.bottom; }
                Rectangle {
                    anchors { left: parent.left; top: parent.top; bottom: parent.bottom; }
                    width: 1;
                    color: (_UI.isDarkColorScheme? "transparent" : "#ff5555")
                }
                Rectangle {
                    anchors { left: parent.left; leftMargin: 3; top: parent.top; bottom: parent.bottom; }
                    width: 1;
                    color: (_UI.isDarkColorScheme? "transparent" : "#ff5555")
                }
            }
        }
        MouseArea {
            id: detectOutsideClicksMouseArea
            anchors.fill: plainTextColumn
        }
        Column {
            id: plainTextColumn
            anchors { left: parent.left; top: parent.top; }
            width: parent.width
            NoteHeader {
                id: noteHeader
                anchors { left: parent.left; }
                width: root.width
                leftSpacing: (_UI.isDarkColorScheme? _UI.paddingLarge * 2.5 : _UI.paddingLarge * 4)
                timestampString: Qt.formatDateTime(root.updatedTime, "dddd, MMM d, yyyy\n" + (root.is24hourTimeFormat? "hh:mm" : "h:mm ap"));
                isFavourite: root.isFavourite
                showFavouriteControl: root.showFavouriteControl
                onFavouritenessChanged: root.favouritenessChanged(favouriteness);
                onTimestampClicked: root.timestampClicked();
            }
            TextEditor {
                id: plainTextViewTextEdit
                readOnly: root.isReadOnly
                anchors { left: parent.left; leftMargin: (_UI.isDarkColorScheme? _UI.paddingLarge * 2.5 : _UI.paddingLarge * 4); }
                width: plainTextFlickable.width - _UI.paddingLarge * 5
                text: ""
                color: (_UI.isDarkColorScheme? "#fff" : "#000")
                flickableContentY: plainTextFlickable.contentY
                flickableHeight: plainTextFlickable.height
                flickableContentHeaderHeight: noteHeader.height
                flickableIsMoving: plainTextFlickable.moving
                z: 1000 // so that flickableFiller doesn't grab clicks meant to go to the textedit's children
                onScrollToYRequested: plainTextFlickable.contentY = y; // where y is the emitted value
                onDisableUserFlicking: plainTextFlickable.interactive = false;
                onEnableUserFlicking: plainTextFlickable.interactive = true;
                Component.onCompleted: {
                    detectOutsideClicksMouseArea.clicked.connect(plainTextViewTextEdit.clickedOutside);
                }
            }
            Item {
                id: plainTextViewImageContainer
                width: plainTextFlickable.contentWidth
                height: (root.isImageExists? (plainTextViewImageLoader.implicitHeight + (root.imageSpacing * 2)) : 0)
                Loader {
                    id: plainTextViewImageLoader
                    property int additionalLeftSpacing: 0
                    anchors { left: parent.left; leftMargin: (root.imageSpacing + additionalLeftSpacing);
                        top: parent.top; topMargin: root.imageSpacing; }
                    sourceComponent: (root.isImageExists? imageFlowComponent : undefined)
                }
                Component {
                    id: imageFlowComponent
                    Flow {
                        id: plainTextViewImageFlow
                        width: plainTextFlickable.width - (root.imageSpacing * 2)
                        spacing: root.imageSpacing
                        Repeater {
                            model: root.imagesModel
                            Rectangle {
                                width: Math.max(plainTextViewImage.width + _UI.paddingSmall * 2, _UI.graphicSizeMedium);
                                height: Math.max(plainTextViewImage.height + _UI.paddingSmall * 2, _UI.graphicSizeMedium);
                                color: (_UI.isDarkColorScheme? "#000" : "#fff")
                                border { color: (_UI.isDarkColorScheme? "#848484" : "#000"); width:  1; }
                                Image {
                                    id: plainTextViewImage
                                    property int maxSourceSizeSide: 400;
                                    property int w: modelData.Width;
                                    property int h: modelData.Height;
                                    property double sourceSizeScale: Math.min(1.0, maxSourceSizeSide / Math.max(w, h));
                                    property int sourceSizeWidth: w * sourceSizeScale
                                    property int sourceSizeHeight: h * sourceSizeScale
                                    anchors.centerIn: parent
                                    source: "image://noteimage/" + modelData.Url
                                    width: Math.min(sourceSizeWidth, plainTextViewImageFlow.width - _UI.paddingSmall * 2)
                                    height: sourceSizeHeight * (width / sourceSizeWidth)
                                    fillMode: Image.PreserveAspectFit
                                    sourceSize { width: sourceSizeWidth; height: sourceSizeHeight; }
                                    smooth: true
                                    cache: false
                                    asynchronous: true
                                }
                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: (plainTextViewImage.status == Image.Loading)
                                    opacity: ((plainTextViewImage.status == Image.Loading &&
                                               plainTextViewImage.width > width &&
                                               plainTextViewImage.height > height) ? 1.0 : 0.0)
                                    colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
                                    size: "medium"
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: root.imageClicked(index)
                                }
                            }
                        }
                    } /* Flow */
                } /* Component */
                Component.onCompleted: {
                    updatePosition();
                }
                function updatePosition() {
                    var imagesModel = root.imagesModel;
                    if (imagesModel.length == 1) {
                        var maxSourceSizeSide = 400;
                        var w = imagesModel[0].Width;
                        var h = imagesModel[0].Height;
                        var sourceSizeScale = Math.min(1.0, maxSourceSizeSide / Math.max(w, h));
                        var containerRectWidth = (w * sourceSizeScale) + (_UI.paddingSmall * 2);
                        plainTextViewImageLoader.additionalLeftSpacing = (plainTextViewImageContainer.width - containerRectWidth) / 2;
                    } else {
                        plainTextViewImageLoader.additionalLeftSpacing = 0;
                    }
                }
            }
            Item {
                id: flickableFiller
                width: plainTextViewTextEdit.width
                anchors { left: parent.left; leftMargin: _UI.paddingLarge * 4; }
                height: Math.max(0, (plainTextFlickable.height - noteHeader.height - plainTextViewTextEdit.height - plainTextViewImageContainer.height))
                MouseArea {
                    anchors.fill: parent
                    enabled: (plainTextViewImageContainer.height == 0)
                    onClicked: plainTextViewTextEdit.clickedAtEnd();
                    onPressAndHold: plainTextViewTextEdit.longPressedAtEnd();
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: plainTextFlickable
    }

    function openKeyboardWithAllTextSelected() {
        plainTextViewTextEdit.openKeyboardWithAllTextSelected();
    }

    function beginAppendText() {
        plainTextViewTextEdit.beginAppendText();
    }

    function beginEditFirstLine() {
        plainTextViewTextEdit.beginEditFirstLine();
    }

    function scrollToBottom() {
        plainTextFlickable.contentY = plainTextFlickable.contentHeight - plainTextFlickable.height;
    }
}
