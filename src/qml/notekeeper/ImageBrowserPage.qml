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
    property variant imagesModel;
    property int startingIndex: 0;
    property variant notePage;
    property bool canRemoveImages: true;

    signal removeImage(int index)

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: "#000"
    }

    Loader {
        id: imageBrowserLoader
        anchors.fill: parent
        sourceComponent: Component {
            Item {
                anchors.fill: parent
                property int currentIndex: 0; // dummy
                BusyIndicator {
                    anchors.centerIn: parent
                    running: true
                    colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
                }
            }
        }
    }

    Component {
        id: imageBrowserComponent
        ImageBrowser {
            window: root.window;
            modelKeyUrl: "Url";
            modelKeyWidth: "Width";
            modelKeyHeight: "Height";
            useModelData: true;
            model: root.imagesModel;
            currentIndex: root.startingIndex;
            onClicked: {
                internal.isImageInfoVisible = (!internal.isImageInfoVisible);
                if (internal.isImageInfoVisible) {
                    infoHideTimer.restart();
                }
            }
            onCurrentIndexChanged: {
                if (internal.isImageInfoVisible) {
                    infoHideTimer.restart();
                }
            }
        }
    }

    Item {
        id: imageInfo
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        height: (imageInfoColumn.height + _UI.paddingLarge * 2)
        opacity: (internal.isImageInfoVisible? 1.0 : 0.0)
        Behavior on opacity { NumberAnimation { duration: 200; } }
        Rectangle {
            anchors.fill: parent
            color: "#000"
            opacity: 0.6
        }
        Column {
            id: imageInfoColumn
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: _UI.paddingLarge; }
            width: parent.width
            TextLabel {
                text: (internal.isLoaded? ("Image " + (imageBrowserLoader.item.currentIndex+1) + " of " + internal.imageCount) : "")
                horizontalAlignment: Qt.AlignLeft
                wrapMode: Text.Wrap
                elide: Text.ElideMiddle
                color: "#fff"
            }
            TextLabel {
                text: (internal.isLoaded? (root.imagesModel[imageBrowserLoader.item.currentIndex].FileName) : "")
                font.pixelSize: _UI.fontSizeSmall
                horizontalAlignment: Qt.AlignLeft
                wrapMode: Text.Wrap
                elide: Text.ElideMiddle
                color: "#fff"
            }
            TextLabel {
                text: {
                    if (internal.isLoaded) {
                        var width = (root.imagesModel[imageBrowserLoader.item.currentIndex]).Width;
                        var height = (root.imagesModel[imageBrowserLoader.item.currentIndex]).Height;
                        var bytes = (root.imagesModel[imageBrowserLoader.item.currentIndex]).Size;
                        return (width + " x " + height + " pixels, " + Humanize.filesizeformat(bytes, decimal_point_char));
                    }
                    return "";
                }
                font.pixelSize: _UI.fontSizeSmall
                horizontalAlignment: Qt.AlignLeft
                wrapMode: Text.Wrap
                elide: Text.ElideMiddle
                color: "#fff"
            }
        }
    }

    QtObject {
        id: internal
        property int imageCount: 0;
        property bool isLoaded: false;
        property bool isImageInfoVisible: true;
    }

    ToolBar {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; }
        opacity: (internal.isImageInfoVisible? 0.65 : 0.0)
        Behavior on opacity { NumberAnimation { duration: 200; } }
        tools: ToolBarLayout {
            id: toolbarLayout
            ToolIconButton {
                iconId: "toolbar-back";
                onClicked: {
                    root.window.showStatusBar = true;
                    root.window.showToolBar = true;
                    root.pageStack.pop();
                }
            }
            ButtonRow {
                checkedButton: null
                exclusive: false
                ButtonRowIconButton {
                    iconId: (enabled? "toolbar-previous" : "");
                    iconPath: (enabled? "" : "images/toolbar/blank.svg");
                    enabled: (internal.isLoaded && imageBrowserLoader.item.currentIndex > 0);
                    onClicked: {
                        parent.checkedButton = null;
                        root.goPrevious();
                    }
                }
                ButtonRowIconButton {
                    iconId: (enabled? "toolbar-next" : "");
                    iconPath: (enabled? "" : "images/toolbar/blank.svg");
                    enabled: (internal.isLoaded && imageBrowserLoader.item.currentIndex < (internal.imageCount - 1));
                    onClicked: {
                        parent.checkedButton = null;
                        root.goNext();
                    }
                }
            }
            ToolIconButton {
                iconId: "toolbar-view-menu";
                onClicked: imageAttachmentMenu.open();
            }
        }
    }

    Menu {
        id: imageAttachmentMenu
        content: MenuLayout {
            MenuItem { text: "Save to phone"; enabled: internal.isLoaded; onClicked: attachmentSaveHelper.startSavingToPhone(); }
            MenuItem { text: "Remove image"; enabled: (internal.isLoaded && root.canRemoveImages); onClicked: removeImageConfirmDialog.open(); }
        }
        onStatusChanged: {
            if (status == _UI.dialogStatus_Closed) {
                infoHideTimer.restart();
            }
        }
    }

    AttachmentSaveHelper {
        id: attachmentSaveHelper
        window: root.window;
        notePage: root.notePage;
        url: (internal.isLoaded? (root.imagesModel[imageBrowserLoader.item.currentIndex].Url) : "")
        fileSize: (internal.isLoaded? (root.imagesModel[imageBrowserLoader.item.currentIndex].Size) : 0)
        mimeType: (internal.isLoaded? (root.imagesModel[imageBrowserLoader.item.currentIndex].MimeType) : "")
        fileName: (internal.isLoaded? (root.imagesModel[imageBrowserLoader.item.currentIndex].FileName) : "");
    }

    QueryDialog {
        id: removeImageConfirmDialog
        titleText: "Remove image from note?"
        message: (internal.isLoaded? (root.imagesModel[imageBrowserLoader.item.currentIndex].FileName) : "")
        acceptButtonText: "Remove"
        rejectButtonText: "Keep"
        onAccepted: root.removeImageConfirmed();
        height: 150
        onStatusChanged: {
            if (status == _UI.dialogStatus_Closed) {
                infoHideTimer.restart();
            }
        }
    }

    Timer {
        id: infoHideTimer
        running: false
        repeat: false
        interval: 5000
        onTriggered: {
            if (imageAttachmentMenu.status == _UI.dialogStatus_Open || imageAttachmentMenu.status == _UI.dialogStatus_Opening ||
                removeImageConfirmDialog.status == _UI.dialogStatus_Open || removeImageConfirmDialog.status == _UI.dialogStatus_Opening) {
                return;
            }
            if (internal.isImageInfoVisible) {
                internal.isImageInfoVisible = false;
            }
        }
    }

    Timer {
        id: delayedLoadTimer
        running: false
        repeat: false
        interval: 300 // a little after the pagestack transition completes, and the statusbar is hidden
        onTriggered: root.load();
    }

    function load() {
        root.window.showStatusBar = false;
        root.window.showToolBar = false;
        imageBrowserLoader.sourceComponent = imageBrowserComponent;
        internal.isLoaded = true;
        internal.imageCount = imageBrowserLoader.item.count;
        if (internal.isImageInfoVisible) {
            infoHideTimer.restart();
        }
        if (root.notePage) {
            root.notePage.imageRemoved.connect(imageRemoveCompleted);
        }
    }

    function delayedLoad() {
        root.window.showStatusBar = false;
        root.window.showToolBar = false;
        delayedLoadTimer.start();
    }

    function goToIndex(i) {
        if (internal.isLoaded) {
            imageBrowserLoader.item.goToIndex(i);
        }
    }

    function goNext() {
        if (internal.isLoaded) {
            imageBrowserLoader.item.goNext();
        }
    }

    function goPrevious() {
        if (internal.isLoaded) {
            imageBrowserLoader.item.goPrevious();
        }
    }

    function removeImageConfirmed() {
        if (!internal.isLoaded) {
            return;
        }
        root.removeImage(imageBrowserLoader.item.currentIndex);
    }

    function imageRemoveCompleted(imageIndex) {
        if (!internal.isLoaded) {
            return;
        }
        var model = root.imagesModel;
        model.splice(imageIndex, 1);
        var imageCount = internal.imageCount - 1;
        if (imageCount > 0) {
            var currentIndex = imageBrowserLoader.item.currentIndex;
            if (currentIndex >= imageCount) {
                currentIndex = imageCount - 1;
                imageBrowserLoader.item.currentIndex = currentIndex;
            }
            internal.imageCount = imageCount;
            root.imagesModel = model;
        } else {
            root.window.showStatusBar = true;
            root.window.showToolBar = true;
            root.pageStack.pop();
        }
    }

    Component.onDestruction: {
        if (root.notePage) {
            root.notePage.imageRemoved.disconnect(imageRemoveCompleted);
        }
        infoHideTimer.stop();
    }
}
