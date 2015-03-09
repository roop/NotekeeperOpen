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
    property variant window;
    property variant notePage;

    property string url;
    property int fileSize: 0; // bytes
    property string mimeType;
    property string fileName;

    property bool isAutoDownloading: false;

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property bool isSavingFile: false;
        property string fileSavedPath;
        property bool isCancelled: false;
    }

    GenericDialog {
        id: savingFileDialog
        property bool isLocalFile: false;
        property int fileSizeBytes: 0;
        titleText: "Saving attachment to phone"
        buttonTexts: (savingFileDialog.isLocalFile? [] : [ "Cancel" ])
        content: Item {
            width: parent.width
            height: 200
            Item {
                anchors { fill: parent; margins: _UI.paddingLarge; }
                Column {
                    id: savingFileColumn
                    anchors.centerIn: parent
                    spacing: _UI.paddingLarge
                    TextLabel {
                        width: parent.width
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
                        horizontalAlignment: Qt.AlignHCenter
                        text: (savingFileDialog.isLocalFile? "Copying" : "Downloading")
                        color: "#fff"
                    }
                    ProgressBar {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 300
                        minimumValue: 0
                        maximumValue: savingFileDialog.fileSizeBytes
                        value: (internal.isSavingFile? qmlDataAccess.attachmentDownloadedBytes : 0)
                    }
                }
            }
        }
        onButtonClicked: {
            if (index == 0) { // Cancel clicked
                if (internal.isSavingFile && !savingFileDialog.isLocalFile) {
                    internal.isCancelled = true;
                    qmlDataAccess.cancel();
                    savingFileDialog.close();
                }
            }
        }
    }

    function startSavingToPhone() {
        if (root.isAutoDownloading) {
            return;
        }

        var fileName = root.fileName;
        var fileSize = root.fileSize;
        var url = root.url;
        var mimeType = (root.mimeType? root.mimeType : "").toLowerCase();
        var isImage = (mimeType.indexOf("image/") == 0);

        if (!url || !fileSize) {
            return;
        }

        if (!fileName) {
            var noteTitleLeft = (root.notePage? root.notePage.noteTitle() : "note_attachment").substr(0, 15);
            // try to guess an appropriate extension
            var extension = "";
            var type = mimeType.substring(mimeType.lastIndexOf("/") + 1);
            if (mimeType == "application/pdf") {
                extension = "pdf";
            } else if (mimeType == "image/jpeg" || mimeType == "image/jpg") {
                extension = "jpg";
            } else if (mimeType == "image/gif") {
                extension = "gif";
            } else if (mimeType == "image/png") {
                extension = "png";
            } else if (mimeType == "audio/wav") {
                extension = "wav";
            } else if (mimeType == "audio/mpeg") {
                extension = "mp3";
            } else if (type.indexOf("svg") == 0) {
                extension = "svg";
            } else if (mimeType == "text/plain") {
                extension = "txt";
            } else if (type == "octet-stream") {
                extension = "";
            } else {
                var specialCharPos = type.search(/[^A-Za-z0-9_]/);
                if (specialCharPos >= 0) {
                    extension = type.substr(0, specialCharPos);
                } else {
                    extension = type;
                }
            }
            fileName = (noteTitleLeft + (extension? ("." + extension) : ""));
        }

        if (internal.fileSavedPath) { // file's been saved earlier in this page
            var alreadySaved = qmlDataAccess.fileExistsInFull(internal.fileSavedPath, fileSize);
            if (alreadySaved) {
                root.window.showInfoBanner("File was already saved to " + internal.fileSavedPath);
                return;
            }
        }

        savingFileDialog.isLocalFile = (url.indexOf("file:///") == 0);
        savingFileDialog.fileSizeBytes = fileSize;
        savingFileDialog.open();

        var drive = "";
        var dir = "";
        if (is_symbian) {
            drive = "C:/";
            dir = "Data/Notekeeper/Saved Attachments";
        } else if (is_harmattan) {
            if (isImage) {
                dir = qmlDataAccess.harmattanPicturesDir();
            } else {
                dir = qmlDataAccess.harmattanDocumentsDir();
            }
        }

        var created = qmlDataAccess.createDir(drive, dir);
        if (!created) {
            root.window.showInfoBanner("Could not create directory " + drive + dir + " to save image");
            return;
        }

        qmlDataAccess.attachmentDownloaded.connect(attachmentDownloaded);
        qmlDataAccess.startAttachmentDownload(url, drive + dir + "/" + fileName, "" /* no need to specify extension */);
        internal.isSavingFile = true;
    }

    function attachmentDownloaded(filePath) {
        if (internal.isSavingFile) {
            qmlDataAccess.attachmentDownloaded.disconnect(attachmentDownloaded);
            if (savingFileDialog.status != _UI.dialogStatus_Closed) {
                savingFileDialog.close();
            }
            internal.isSavingFile = false;
            internal.fileSavedPath = filePath;
            if (filePath == "") {
                if (!internal.isCancelled) {
                    root.window.showInfoBanner("Error saving file");
                }
            } else {
                root.window.showInfoBanner("File saved to " + filePath);
            }
            internal.isCancelled = false;
        }
    }
}
