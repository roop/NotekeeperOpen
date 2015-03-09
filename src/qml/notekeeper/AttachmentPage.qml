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
import QtMultimediaKit 1.1
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
        property bool isLoaded: false;

        property variant attachmentData;
        property int attachmentIndex;
        property string url;
        property string name;
        property int fileSize: 0;

        property bool isImage: false;
        property int imageWidth: 0;
        property int imageHeight: 0;
        property bool isImageInfoVisible: true;

        property bool isAudio: false;
        property int durationMsecs: 0;
        property bool isPlaying: false;
        property real audioVolume;

        property bool isPdf: false;
        property bool isUnknownFileType: false;

        // auto-downloading
        property bool isLocalFile: false;
        property bool isFileDownloadInitiated: false;
        property bool isFileDownloadComplete: false;
        property bool isFileDownloadErrored: false;
        property string downloadedFilePath;

        property bool isGoBackRequested: false;
        property bool isRemoveRequested: false;

        function humanPlayerTime(msec) {
            if (msec < 0 || msec == undefined) {
                return "";
            } else {
                var sec = "" + Math.ceil(msec / 1000) % 60
                if (sec.length == 1) {
                    sec = "0" + sec;
                }
                var hour = Math.floor(msec / 3600000);
                if (hour < 1) {
                    return Math.floor(msec / 60000) + ":" + sec
                } else {
                    var min = "" + Math.floor(msec / 60000) % 60
                    if (min.length == 1) {
                        min = "0" + min;
                    }
                    return hour + ":" + min + ":" + sec;
                }
            }
        }

        signal aboutToGoBack()
    }

    Rectangle {
        anchors.fill: parent
        color: (internal.isImage? "#000" : _UI.pageBgColor)
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: internal.name
        height: (internal.isImage? 0 : implicitHeight)
        opacity: (internal.isImage? 0 : 1)
        iconPath: "images/titlebar/attachment_white.svg"
    }

    Loader {
        id: loader
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
        id: imageComponent
        ImagePinchZoomer {
            anchors.fill: parent
            imageSource: internal.url
            actualImageWidth: internal.imageWidth
            actualImageHeight: internal.imageHeight
            pinchPanEnabled: true
            onClicked: {
                internal.isImageInfoVisible = (!internal.isImageInfoVisible);
                if (internal.isImageInfoVisible) {
                    infoHideTimer.restart();
                }
            }
            BusyIndicator {
                anchors.centerIn: parent
                running: (parent.imageStatus == Image.Loading)
                opacity: (running? 1 : 0)
                colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
            }
        }
    }

    Component {
        id: fileComponent
        Flickable {
            id: fileFlickable
            anchors.fill: parent
            contentWidth: width
            contentHeight: fileContainer.height
            boundsBehavior: Flickable.StopAtBounds
            Item {
                id: fileContainer
                width: fileFlickable.width
                height: Math.max(fileFlickable.height, fileColumn.height)
                Column {
                    id: fileColumn
                    anchors.centerIn: parent
                    width: parent.width - _UI.paddingLarge * 4
                    spacing: _UI.paddingLarge
                    TextLabel {
                        width: parent.width
                        text: internal.name
                        wrapMode: Text.Wrap
                        horizontalAlignment: Qt.AlignHCenter
                        font.pixelSize: _UI.fontSizeMedium
                    }
                    TextLabel {
                        width: parent.width
                        text: ((internal.isAudio && internal.durationMsecs > 0)? (internal.humanPlayerTime(internal.durationMsecs) + ", ") : "") + Humanize.filesizeformat(internal.fileSize, decimal_point_char)
                        wrapMode: Text.Wrap
                        horizontalAlignment: Qt.AlignHCenter
                        font.pixelSize: _UI.fontSizeSmall
                        color: (_UI.isDarkColorScheme? "lightgray" : "gray")
                    }
                    Item { // spacer
                        width: parent.width
                        height: _UI.paddingLarge
                    }
                    Loader {
                        id: controlsLoader
                        width: parent.width
                        height: 100
                        sourceComponent: (internal.isUnknownFileType? unknownFileTypeComponent :
                                              ((!internal.isFileDownloadComplete)? downloadingFileComponent :
                                                   (internal.isAudio? audioPlayerComponent :
                                                        (internal.isPdf? openPdfComponent : undefined)
                                                    )
                                               )
                                         )
                    }
                }
            }
        }
    }

    Component {
        id: unknownFileTypeComponent
        Item {
            width: parent.width
            height: unknownFileTypeText.height
            TextLabel {
                id: unknownFileTypeText
                anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                          right: parent.right; rightMargin: _UI.paddingLarge;
                          top: parent.top; }
                text: "Notekeeper does not know what to do with this attachment. To save this file, use\n Menu > Save to phone"
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                horizontalAlignment: Qt.AlignHCenter
                font.pixelSize: _UI.fontSizeSmall
            }
        }
    }

    Component {
        id: downloadingFileComponent
        Column {
            TextLabel {
                width: parent.width
                text: {
                    if (internal.isLocalFile) {
                        return (internal.isFileDownloadComplete? "" : "Preparing " + (internal.isAudio? "audio" : "file"));
                    } else if (internal.isFileDownloadComplete) {
                        return (internal.isAudio? "" : "Download complete");
                    } else if (internal.isFileDownloadErrored) {
                        return "Download failed";
                    }
                    return "Downloading " + (internal.isAudio? "audio" : "file");
                }
                wrapMode: Text.Wrap
                horizontalAlignment: Qt.AlignHCenter
                font.pixelSize: _UI.fontSizeSmall
                color: (internal.isFileDownloadErrored? "red" : (_UI.isDarkColorScheme? "#fff" : "#000"))
            }
            ProgressBar {
                id: downloadProgressBar
                anchors.horizontalCenter: parent.horizontalCenter
                width: 300
                minimumValue: 0
                maximumValue: internal.fileSize
                value: (internal.isFileDownloadInitiated? qmlDataAccess.attachmentDownloadedBytes : 0)
                opacity: ((value > 0 && value < maximumValue && !internal.isFileDownloadComplete && !internal.isLocalFile)? 1.0 : 0.0)
                Behavior on opacity { NumberAnimation { duration: 200; } }
            }
        }
    }

    Component {
        id: openPdfComponent
        Item {
            anchors.fill: parent
            AutoWidthButton {
                anchors.centerIn: parent
                text: (is_harmattan? "Open" : "Open with PDF reader")
                enabled: (internal.downloadedFilePath != "")
                onClicked: {
                    if (internal.downloadedFilePath == "") {
                        return;
                    }
                    if (internal.isPdf) {
                        window.openUrlExternally("file:///" + internal.downloadedFilePath);
                    }
                }
            }
        }
    }

    Component {
        id: audioPlayerComponent
        Item {
            id: audioPlayer
            opacity: (internal.isFileDownloadComplete && internal.isAudio? 1.0 : 0.0)
            anchors.horizontalCenter: parent.horizontalCenter
            Audio {
                id: audioItem
                source: (internal.downloadedFilePath? ("file:///" + internal.downloadedFilePath) : "")
                volume: volumeSlider.value
                autoLoad: false
                onPlayingChanged: { internal.isPlaying = audioItem.playing; }
                onPausedChanged: { internal.isPlaying = !audioItem.paused; }
                onStopped: { audioItem.position = 0; audioSeeker.audioPosition = 0; }
                onError: {
                    if (audioItem.errorString != "") {
                        console.debug("Audio error: " + audioItem.errorString);
                        root.window.showInfoBanner("Error loading audio");
                    }
                }
                function stopBeforeGoingBack() { // so that the temp file can be deleted
                    if (internal.isPlaying) {
                        audioItem.stop();
                    }
                }
                Component.onCompleted: {
                    internal.aboutToGoBack.connect(stopBeforeGoingBack);
                }
            }
            Column {
                width: Math.min(root.width, root.height) * 0.9
                anchors.centerIn: parent
                Rectangle {
                    width: parent.width
                    height: (_UI.isPortrait? _UI.toolBarHeightPortraitPrivate : _UI.toolBarHeightLandscapePrivate)
                    color: "#000"
                    border.color: (_UI.isDarkColorScheme? "gray" : "lightgray")
                    radius: 5
                    ToolIconButton {
                        id: playPauseButton
                        anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                            verticalCenter: parent.verticalCenter; }
                        iconId: (internal.isPlaying? "toolbar-mediacontrol-pause" : "toolbar-mediacontrol-play") + ((is_harmattan && !_UI.isDarkColorScheme)? "-white" : "");
                        enabled: (internal.isFileDownloadComplete && internal.isAudio && audioItem.error == Audio.NoError);
                        onClicked: {
                            if (internal.isPlaying) {
                                audioItem.pause();
                            } else {
                                audioItem.play();
                            }
                        }
                    }
                    Item {
                        id: audioSeeker
                        anchors { left: playPauseButton.right; leftMargin: _UI.paddingLarge;
                            right: parent.right; rightMargin: _UI.paddingLarge;
                            verticalCenter: parent.verticalCenter; }
                        height: playPauseButton.height / 2
                        property int audioPosition: 0
                        Rectangle {
                            id: elapsedDuration
                            color: "blue"
                            anchors { left: parent.left; top: parent.top; bottom: parent.bottom; }
                            width: parent.width * (audioSeeker.audioPosition / audioItem.duration);
                        }
                        Rectangle {
                            id: audioSeekHandle
                            anchors{ horizontalCenter: elapsedDuration.right; verticalCenter: parent.verticalCenter; }
                            height: parent.height + 4
                            width: 4
                            radius: 2
                            color: "lightblue"
                            opacity: (audioSeeker.audioPosition == 0? 0 : 1)
                        }
                        TextLabel {
                            anchors { left: parent.left; leftMargin: _UI.paddingMedium; verticalCenter: parent.verticalCenter; }
                            text: ((audioSeeker.audioPosition > 0)? internal.humanPlayerTime(audioSeeker.audioPosition) : "");
                            font.pixelSize: _UI.fontSizeSmall
                            color: "lightgray";
                        }
                        TextLabel {
                            anchors { right: parent.right; rightMargin: _UI.paddingMedium; verticalCenter: parent.verticalCenter; }
                            text: ((audioItem.duration > 0) ? internal.humanPlayerTime(audioItem.duration) : "");
                            font.pixelSize: _UI.fontSizeSmall
                            color: "lightgray";
                        }
                        MouseArea {
                            id: seekMouseArea
                            anchors.fill: parent
                            enabled: (audioItem.duration > 0)
                            onPressed: { if (internal.isPlaying) { audioItem.pause(); } }
                            onReleased: { audioItem.position = audioSeeker.audioPosition; }
                        }
                        Binding {
                            target: audioSeeker
                            property: "audioPosition"
                            when: seekMouseArea.pressed
                            value: audioItem.duration * Math.min(Math.max(0.01, seekMouseArea.mouseX / seekMouseArea.width), 1)
                        }
                        Binding {
                            target: audioSeeker
                            property: "audioPosition"
                            when: (internal.isPlaying && !seekMouseArea.pressed)
                            value: audioItem.position
                        }
                    }
                }
                Row {
                    width: parent.width
                    height: Math.max(_UI.graphicSizeSmall, volumeSlider.height)
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: _UI.graphicSizeSmall
                        height: _UI.graphicSizeSmall
                        source: "images/filetypes/low_volume.svg"
                        smooth: true
                    }
                    Slider {
                        anchors.verticalCenter: parent.verticalCenter
                        id: volumeSlider
                        width: parent.width - _UI.graphicSizeSmall * 2
                        minimumValue: 0.0
                        maximumValue: 1.0
                        value: 0
                        onValueChanged: {
                            internal.audioVolume = value;
                        }
                        Component.onCompleted: {
                            value = root.savedAudioVolume();
                        }
                    }
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: _UI.graphicSizeSmall
                        height: _UI.graphicSizeSmall
                        source: "images/filetypes/audio.svg"
                        smooth: true
                    }
                }
            }
        }
    }

    Item {
        id: imageInfo
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        height: (internal.isImage? (imageInfoColumn.height + _UI.paddingLarge * 2) : 0)
        opacity: (internal.isImage? (internal.isImageInfoVisible? 1.0 : 0.0) : 0)
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
                text: internal.name
                font.pixelSize: _UI.fontSizeMedium
                horizontalAlignment: Qt.AlignLeft
                wrapMode: Text.Wrap
                elide: Text.ElideMiddle
                color: "#fff"
            }
            TextLabel {
                text: {
                    if (internal.isLoaded) {
                        var width = internal.imageWidth;
                        var height = internal.imageHeight;
                        var bytes = internal.fileSize;
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

    tools: ToolBarLayout {
        ToolIconButton {
            iconId: "toolbar-back";
            onClicked: root.tryToGoBack(); // cancel any downloads in progress
        }
        ToolIconButton {
            iconId: "toolbar-view-menu";
            onClicked: menu.open();
        }
    }

    ToolBar {
        id: imageToolbar // used only when showing images (for fullscreen-ness)
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; }
        opacity: (internal.isImage && internal.isImageInfoVisible? 0.65 : 0.0)
        Behavior on opacity { NumberAnimation { duration: 200; } }
        tools: ToolBarLayout {
            ToolIconButton {
                iconId: "toolbar-back";
                onClicked: root.goBack();
            }
            ToolIconButton {
                iconId: "toolbar-view-menu";
                onClicked: menu.open();
            }
        }
    }

    Menu {
        id: menu
        content: MenuLayout {
            MenuItem {
                text: "Save to phone";
                enabled: (internal.isLoaded && (!internal.isFileDownloadInitiated || internal.isFileDownloadComplete));
                onClicked: attachmentSaveHelper.startSavingToPhone();
            }
            MenuItem { text: "Remove attachment"; enabled: (internal.isLoaded && root.canRemoveAttachments); onClicked: removeAttachmentConfirmDialog.open(); }
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
        url: (internal.isLoaded? internal.attachmentData.Url : "");
        fileSize: (internal.isLoaded? internal.attachmentData.Size : 0);
        mimeType: (internal.isLoaded? internal.attachmentData.MimeType : "");
        fileName: (internal.isLoaded? internal.attachmentData.FileName : "");
        isAutoDownloading: (internal.isFileDownloadInitiated && !internal.isFileDownloadComplete && !internal.isFileDownloadErrored);
    }

    QueryDialog {
        id: removeAttachmentConfirmDialog
        titleText: "Remove attachment from note?"
        message: internal.name
        acceptButtonText: "Remove"
        rejectButtonText: "Keep"
        onAccepted: root.removeAttachmentConfirmed();
        height: 150
        onStatusChanged: {
            if (internal.isImage && internal.isImageInfoVisible && status == _UI.dialogStatus_Closed) {
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
            if (menu.status == _UI.dialogStatus_Open || menu.status == _UI.dialogStatus_Opening ||
                removeAttachmentConfirmDialog.status == _UI.dialogStatus_Open || removeAttachmentConfirmDialog.status == _UI.dialogStatus_Opening) {
                return;
            }
            if (internal.isImage && internal.isImageInfoVisible) {
                internal.isImageInfoVisible = false;
            }
        }
    }

    Timer {
        id: delayedLoadTimer
        running: false
        repeat: false
        interval: 300 // a little after the pagestack transition completes
        onTriggered: root.load();
    }

    function load() {
        var attachmentData = internal.attachmentData;

        // load common data
        internal.url = attachmentData.Url;
        internal.name = (attachmentData.FileName? attachmentData.FileName : "Unnamed attachment");
        internal.fileSize = attachmentData.Size;
        internal.isLocalFile = (internal.url.indexOf("file:///") == 0);
        var mimeType = attachmentData.MimeType;

        // load the appropriate component
        if (mimeType.indexOf("image/") == 0) {
            internal.isImage = true;
            internal.imageWidth = attachmentData.Width;
            internal.imageHeight = attachmentData.Height;
            root.window.showStatusBar = false;
            root.window.showToolBar = false;
            loader.sourceComponent = imageComponent;
            if (internal.isImageInfoVisible) {
                infoHideTimer.restart();
            }
        } else if (mimeType == "audio/wav" || mimeType == "audio/mpeg") {
            internal.isAudio = true;
            internal.durationMsecs = attachmentData.Duration;
            loader.sourceComponent = fileComponent;
        } else if (mimeType == "application/pdf") {
            internal.isPdf = true;
            loader.sourceComponent = fileComponent;
        } else {
            internal.isUnknownFileType = true;
            loader.sourceComponent = fileComponent;
        }
        internal.isLoaded = true;

        // start download
        if (internal.isPdf && mimeType == "application/pdf") {
            qmlDataAccess.startAttachmentDownload(internal.url, "" /* assume temp filename */, "pdf" /* extension */);
            internal.isFileDownloadInitiated = true;
        } else if (internal.isAudio && mimeType == "audio/wav") {
            qmlDataAccess.startAttachmentDownload(internal.url, "" /* assume temp filename */, "wav" /* extension */);
            internal.isFileDownloadInitiated = true;
        } else if (internal.isAudio && mimeType == "audio/mpeg") {
            qmlDataAccess.startAttachmentDownload(internal.url, "" /* assume temp filename */, "mp3" /* extension */);
            internal.isFileDownloadInitiated = true;
        } else {
            // unknown-file-type files are downloaded only when user tries to save them to the phone
        }

        if (root.notePage) {
            root.notePage.attachmentRemoved.connect(attachmentRemoveCompleted);
        }
    }

    function delayedLoad(currentIndex, attachmentData) {
        internal.attachmentIndex = currentIndex;
        internal.attachmentData = attachmentData;
        var mimeType = (attachmentData.MimeType? attachmentData.MimeType : "");
        if (mimeType.indexOf("image/") == 0) {
            root.window.showStatusBar = false;
            root.window.showToolBar = false;
            internal.isImage = true;
        }
        delayedLoadTimer.start();
    }

    function attachmentDownloaded(filePath) {
        if (filePath == "") {
            if (internal.isGoBackRequested) {
                goBack();
            } else if (internal.isRemoveRequested) {
                root.removeAttachment(internal.attachmentIndex);
            } else {
                internal.isFileDownloadErrored = true;
            }
        } else {
            internal.downloadedFilePath = filePath;
            internal.isFileDownloadComplete = true;
        }
    }

    function removeAttachmentConfirmed() {
        if (!internal.isLoaded) {
            return;
        }
        var isDownloading = (internal.isFileDownloadInitiated && !internal.isFileDownloadComplete && !internal.isFileDownloadErrored);
        if (isDownloading) {
            qmlDataAccess.cancel();
            internal.isRemoveRequested = true;
        } else {
            root.removeAttachment(internal.attachmentIndex);
        }
    }

    function attachmentRemoveCompleted(index) {
        if (internal.attachmentIndex == index) {
            goBack();
        }
    }

    function tryToGoBack() { // try to cancel downloads first, if any are in progress
        var isDownloading = (internal.isFileDownloadInitiated && !internal.isFileDownloadComplete && !internal.isFileDownloadErrored);
        if (isDownloading) {
            qmlDataAccess.cancel();
            internal.isGoBackRequested = true;
        } else {
            goBack();
        }
    }

    function goBack() {
        internal.aboutToGoBack();
        saveSettings();
        root.window.showStatusBar = true;
        root.window.showToolBar = true;
        root.pageStack.pop();
        if (internal.downloadedFilePath != "") {
            qmlDataAccess.removeTempFile(internal.downloadedFilePath);
        }
        internal.isGoBackRequested = false;
    }

    function savedAudioVolume() {
        var audioVolumeStr = qmlDataAccess.retrieveStringSetting("Attachments/audioVolume");
        if (audioVolumeStr == "") {
            audioVolumeStr = "0.7"; // default audio volume
        }
        return parseFloat(audioVolumeStr);
    }

    function saveSettings() {
        if (internal.isAudio) {
            qmlDataAccess.saveStringSetting("Attachments/audioVolume", "" + Math.round(internal.audioVolume * 100) / 100);
        }
    }

    Component.onCompleted: {
        qmlDataAccess.attachmentDownloaded.connect(attachmentDownloaded);
    }

    Component.onDestruction: {
        qmlDataAccess.attachmentDownloaded.disconnect(attachmentDownloaded);
        if (root.notePage) {
            root.notePage.attachmentRemoved.disconnect(attachmentRemoveCompleted);
        }
    }
}
