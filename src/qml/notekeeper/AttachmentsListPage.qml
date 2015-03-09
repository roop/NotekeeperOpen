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
import "humanize.js" as Humanize

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    property variant window;
    property variant notePage;
    property bool canRemoveAttachments: true;

    signal removeAttachment(int index)

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property variant listModel;
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: "Attachments"
        iconPath: "images/titlebar/attachment_white.svg"
    }

    Loader {
        id: listLoader
        anchors { fill: parent; topMargin: titleBar.height; }
        sourceComponent: Item {
            anchors.fill: parent
            BusyIndicator {
                anchors.centerIn: parent
                running: true
                colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
            }
        }
    }

    Component {
        id: noAttachmentsComponent
        TextLabel {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: "No attachments"
            wrapMode: Text.Wrap
            maximumLineCount: 1
            font.pixelSize: _UI.fontSizeLarge
        }
    }

    Component {
        id: attachmentsListViewComponent
        Item {
            anchors.fill: parent
            ListView {
                id: listView
                clip: true
                anchors.fill: parent
                model: internal.listModel
                delegate: listDelegate
                cacheBuffer: Math.max(height, width)
            }
            ScrollDecorator {
                flickableItem: listView
            }
        }
    }

    Component {
        id: listDelegate
        Item {
            height: listItem.height
            width: root.width
            GenericListItem {
                id: listItem
                width: parent.width
                subItemIndicator: true
                property string mimeType: (modelData.MimeType? modelData.MimeType : "")
                property bool isImage: (mimeType.indexOf("image/") == 0)
                property bool isPdf: (mimeType == "application/pdf")
                property bool isAudio: (mimeType == "audio/wav" || mimeType == "audio/mpeg")
                Row {
                    anchors { fill: parent; margins: _UI.paddingLarge; }
                    spacing: _UI.paddingLarge
                    Image {
                        id: icon
                        width: _UI.graphicSizeSmall
                        height: _UI.graphicSizeSmall
                        anchors.verticalCenter: parent.verticalCenter;
                        source: {
                            if (listItem.isImage) {
                                return "images/filetypes/image.svg";
                            } else if (listItem.isAudio) {
                                return "images/filetypes/audio.svg";
                            } else if (listItem.isPdf) {
                                return "images/filetypes/pdf.svg";
                            }
                            return "images/filetypes/attachment.svg";
                        }
                        smooth: true
                    }
                    Column {
                        width: parent.width - _UI.graphicSizeSmall - parent.spacing
                        anchors.verticalCenter: parent.verticalCenter
                        TextLabel {
                            width: parent.width
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            text: (modelData.FileName? modelData.FileName : "Unnamed attachment")
                            font.pixelSize: _UI.fontSizeMedium
                            elide: Text.ElideMiddle
                            wrapMode: Text.Wrap
                        }
                        TextLabel {
                            id: typeText
                            width: parent.width
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            text: {
                                var type = listItem.mimeType.substring(listItem.mimeType.lastIndexOf("/") + 1).toUpperCase();
                                if (listItem.isPdf) {
                                    type = "PDF";
                                } else if (listItem.isAudio && listItem.mimeType == "audio/wav") {
                                    type = "WAV";
                                } else if (listItem.isAudio && listItem.mimeType == "audio/mpeg") {
                                    type = "MP3";
                                } else if (type.indexOf("SVG") == 0) {
                                    type = "SVG";
                                } else if (listItem.mimeType == "text/plain") {
                                    type = "Text";
                                } else if (type == "OCTET-STREAM") {
                                    type = "";
                                }
                                var bytes = (modelData.Size? modelData.Size : 0);
                                var str = "";
                                if (type != "") {
                                    str = type + " " + (listItem.isImage? "image" : (listItem.isAudio? "audio" : "file")) + ", ";
                                }
                                return str + Humanize.filesizeformat(bytes, decimal_point_char);
                            }
                            font.pixelSize: _UI.fontSizeSmall
                            color: "gray"
                            elide: Text.ElideMiddle
                            wrapMode: Text.Wrap
                        }
                    }
                }
                onClicked: {
                    var page = root.pageStack.push(Qt.resolvedUrl("AttachmentPage.qml"), { window: root.window, notePage: root.notePage, canRemoveAttachments: root.canRemoveAttachments });
                    page.removeAttachment.connect(root.removeAttachment);
                    page.delayedLoad(index, modelData);
                }
            }
        }
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.goBack(); }
    }

    Timer {
        id: delayedLoadTimer
        running: false
        repeat: false
        interval: 300 // a little after the pagestack transition completes, and the statusbar is hidden
        onTriggered: root.load();
    }

    function delayedLoad(attachmentsData) {
        internal.listModel = attachmentsData;
        delayedLoadTimer.start();
    }

    function load() {
        var attachmentsData = internal.listModel;
        if (attachmentsData.length > 0) {
            listLoader.sourceComponent = attachmentsListViewComponent;
        } else {
            listLoader.sourceComponent = noAttachmentsComponent;
        }
        if (root.notePage) {
            root.notePage.attachmentRemoved.connect(attachmentRemoveCompleted);
        }
    }

    function attachmentRemoveCompleted(attachmentIndex) {
        var model = internal.listModel;
        model.splice(attachmentIndex, 1);
        internal.listModel = model;
    }

    function goBack() {
        root.pageStack.pop();
        qmlDataAccess.removeAllPendingTempFiles();
    }

    Component.onDestruction: {
        if (root.notePage) {
            root.notePage.attachmentRemoved.disconnect(attachmentRemoveCompleted);
        }
    }
}
