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

GenericDialog {
    id: root
    property variant window;
    property bool isShown; // read-only

    signal addingSucceededAndDialogClosed();

    titleText: ""
    buttonTexts: ["Cancel"]
    cancelButtonEnabled: {
        if (internal.isCancelRequested) {
            return false;
        }
        if (internal.totalFileCount <= 0) {
            return false;
        }
        if (internal.currentFileIndex == internal.totalFileCount) {
            // the last file being attached can be cancelled only during the image-resize phase
            return (internal.isImage && (internal.statusMessage == "Resizing"));
        }
        return true;
    }
    onButtonClicked: {
        if (!internal.isCancelRequested) {
            internal.isCancelRequested = true;
            qmlDataAccess.cancelAddingAttachment();
        }
    }
    onRejected: {
        if (!internal.isCancelRequested) {
            internal.isCancelRequested = true;
            qmlDataAccess.cancelAddingAttachment();
        }
    }
    height: 300

    QtObject {
        id: internal
        property string currentFilePath: "";
        property string currentFileName: "";
        property string currentFileMimeType: "";
        property int currentFileIndex: 0;
        property int totalFileCount: 0;
        property string statusMessage: "";

        property bool isImage: (currentFileMimeType.indexOf("image/") == 0);
        property bool isPdf: (currentFileMimeType == "application/pdf");
        property bool isAudio: (currentFileMimeType == "audio/wav" || currentFileMimeType == "audio/mpeg");

        property bool isCancelRequested: false;
        property bool isAddingAttachmentSucceeded: false

        function updateAddingAttachmentStatus() {
            if (qmlDataAccess.addingAttachmentStatus) {
                currentFilePath = (qmlDataAccess.addingAttachmentStatus.CurrentFilePath? qmlDataAccess.addingAttachmentStatus.CurrentFilePath : "");
                currentFileName = (qmlDataAccess.addingAttachmentStatus.CurrentFileName? qmlDataAccess.addingAttachmentStatus.CurrentFileName : "");
                currentFileMimeType = (qmlDataAccess.addingAttachmentStatus.MimeType? qmlDataAccess.addingAttachmentStatus.MimeType : "");
                currentFileIndex = (qmlDataAccess.addingAttachmentStatus.CurrentFileIndex? qmlDataAccess.addingAttachmentStatus.CurrentFileIndex : 0);
                totalFileCount = (qmlDataAccess.addingAttachmentStatus.TotalFileCount? qmlDataAccess.addingAttachmentStatus.TotalFileCount : 0);
                statusMessage = (qmlDataAccess.addingAttachmentStatus.Message? qmlDataAccess.addingAttachmentStatus.Message : "");
            }
        }
    }

    Connections {
        target: qmlDataAccess
        onAddingAttachmentStatusChanged: {
            internal.updateAddingAttachmentStatus();
        }
    }

    content: Item {
        id: addingImageDialogContent
        width: parent.width
        height: (is_harmattan? 250 : 200)
        Item {
            anchors { fill: parent; margins: _UI.paddingLarge; }
            Item {
                id: addingImageIcon
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; }
                width: (is_symbian? 160 : Math.min(210, Math.floor((parent.width - _UI.paddingLarge) / 2.0)));
                height: (is_symbian? 160 : Math.min(210, Math.floor((parent.height - _UI.paddingLarge) / 2.0)));
                Image {
                    anchors.centerIn: parent
                    width: (internal.isImage? parent.width : _UI.graphicSizeSmall)
                    height: (internal.isImage? parent.height : _UI.graphicSizeSmall)
                    source: (internal.isImage? ("image://localimagethumbnail/" + internal.currentFilePath)
                                             : (internal.isPdf? "images/filetypes/pdf.svg"
                                                              : (internal.isAudio? "images/filetypes/audio.svg"
                                                                                 : (internal.totalFileCount? "images/filetypes/attachment.svg" : "")
                                                                 )
                                                )
                             )
                    fillMode: Image.PreserveAspectFit
                    clip: true
                    cache: true
                    asynchronous: true
                }
            }
            Column {
                id: addingImageColumn
                anchors { left: addingImageIcon.right; leftMargin: _UI.paddingLarge;
                    right: parent.right; verticalCenter: parent.verticalCenter; }
                spacing: _UI.paddingLarge
                TextLabel {
                    width: parent.width
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    text: (internal.statusMessage? internal.statusMessage : " ");
                    color: "#fff"
                }
                TextLabel {
                    width: parent.width
                    elide: Text.ElideMiddle
                    wrapMode: Text.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    text: (internal.currentFileName? internal.currentFileName : " ");
                    color: "#fff"
                }
                TextLabel {
                    width: parent.width
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    text: ((internal.totalFileCount > 1)? ("(" + internal.currentFileIndex + " of " + internal.totalFileCount + ")") : " ")
                    color: "#ededed"
                }
                ProgressBar {
                    width: Math.min(parent.width, 300)
                    anchors.horizontalCenter: parent.horizontalCenter
                    indeterminate: true
                }
            }
        }
    }

    onStatusChanged: {
        if (root.status == _UI.dialogStatus_Open) {
            if (internal.totalFileCount == 0) {
                // Appending was completed before the dialog could open, so close it now
                root.close();
            }
        }
        if (root.status == _UI.dialogStatus_Closed) {
            if (internal.isAddingAttachmentSucceeded && (!internal.isCancelRequested)) {
                root.addingSucceededAndDialogClosed(); // emit signal
            }
        }
        root.isShown = (root.status != _UI.dialogStatus_Closed);
    }

    function showDialogTillAddingCompletes() {
        qmlDataAccess.doneAddingAttachments.connect(doneAppendingImage);
        open();
    }

    function doneAppendingImage(success, message, filename) {
        qmlDataAccess.doneAddingAttachments.disconnect(doneAppendingImage);
        internal.isAddingAttachmentSucceeded = success;
        if (!success) {
            close();
            if (message) {
                if (message == "Cancelled") {
                    root.window.showInfoBanner("Adding image was cancelled");
                } else {
                    root.window.showErrorMessageDialog("Cannot add " + (filename? filename : "file"), message);
                }
            }
        } else {
            close();
        }
        internal.isCancelRequested = false;
    }
}
