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
import "SyncTriggerer.js" as SyncTrigger

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    property variant window;
    property bool isNotePage: true; // read-only, constant
    property bool isProcessingCreateNoteRequest: false;

    signal imageRemoved(int imageIndex) // used in ImageBrowserPage
    signal attachmentRemoved(int attachmentIndex) // used in AttachmentPage and AttachmentsListPage

    UiStyle {
        id: _UI
        onHarmattanAppDeactivated: {
            if (is_harmattan) {
                root.saveUnlessWeHaveToCreateAnEmptyNote();
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: ""
        dismissKeyboardButton: (is_harmattan? true : false)
        // In Belle FP2, the prediction bar appears above the keyboard
        // so to make more space for typing in landscape in Symbian, we hide the titlebar
        height: ((is_symbian && !_UI.isPortrait && _UI.isVirtualKeyboardOpen)? 0 : implicitHeight)
        Behavior on height { NumberAnimation { duration: 200; } }
        opacity: (height == 0? 0 : 1)
    }

    Component {
        id: plainTextComponent
        PlainTextNoteView {
            anchors.fill: parent
            text: internal.plainTextContent
            updatedTime: internal.updatedTime
            isFavourite: internal.isFavourite
            isReadOnly: (internal.isEditingDisabled || internal.isBusy || internal.goBackRequested)
            isImageExists: internal.isImageExists
            is24hourTimeFormat: internal.is24hourTimeFormat
            imagesModel: internal.imagesModel
            showFavouriteControl: (!internal.isTrashed && internal.noteId != "")
            onFavouritenessChanged: { internal.isFavourite = favouriteness; }
            onTimestampClicked: root.showNoteDetailsPage();
            onImageClicked: root.showImagePage(index);
            virtualKeyboardVisible: _UI.isVirtualKeyboardOpen
        }
    }

    Component {
        id: richTextComponent
        RichTextNoteView {
            anchors.fill: parent
            html: internal.htmlContent
            updatedTime: internal.updatedTime
            isFavourite: internal.isFavourite
            is24hourTimeFormat: internal.is24hourTimeFormat
            title: internal.title
            checkboxStates: internal.checkboxStates
            showFavouriteControl: (!internal.isTrashed && internal.noteId != "")
            onFavouritenessChanged: { internal.isFavourite = favouriteness; }
            onTimestampClicked: root.showNoteDetailsPage();
            onImageClicked: root.showImagePage(index);
            onAttachmentClicked: root.showAttachmentPage(index);
            onRichTextNoteTextClicked: {
                root.window.showInfoBanner("Cannot edit rich-text notes directly\nTry Menu -> 'Append text' instead");
            }
        }
    }

    Component {
        id: fetchErrorTextComponent
        Item {
            anchors.fill: parent
            TextLabel {
                anchors.centerIn: parent
                width: parent.width * 0.6
                font.pixelSize: _UI.fontSizeLarge
                text: appName + " could not fetch this note from the Evernote server"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: "gray"
            }
        }
    }

    Loader {
        id: contentLoader
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
        Binding {
            when: (internal.isPlainText && !internal.isEditingDisabled)
            target: internal; property: "plainTextContent";
            value: contentLoader.item.text;
        }
        Binding {
            when: (internal.isPlainText && !internal.isEditingDisabled)
            target: internal; property: "plainTextHasBeenChangedByUser";
            value: contentLoader.item.textHasBeenChangedByUser;
        }
        Binding {
            when: (!internal.isPlainText && !internal.isEditingDisabled)
            target: internal; property: "checkboxStates";
            value: contentLoader.item.checkboxStates
        }
    }

    tools: ToolBarLayout {
        id: toolbarLayout
        enabled: (!internal.goBackRequested)
        ToolIconButton {
            iconId: (root.isProcessingCreateNoteRequest? "" : "toolbar-back");
            iconPath: (root.isProcessingCreateNoteRequest? "images/toolbar/notekeeper_logo_n9.png" : "")
            onClicked: root.backButtonPressed();
        }
        ToolIconButton {
            iconPath: (enabled ? (is_harmattan? "images/toolbar/add_image_n9" + (_UI.isDarkColorScheme? "_white" : "") + ".svg" : "images/toolbar/add_image.svg") : "images/toolbar/blank.svg");
            enabled: (!(internal.isBusy || internal.isTrashed || internal.isErrorFetchingNote))
            onClicked: root.showAddImagePage();
        }
        ToolIconButton {
            iconPath: (enabled? (is_harmattan? "images/toolbar/info_n9" + (_UI.isDarkColorScheme? "_white" : "") + ".svg" : "images/toolbar/nokia_information_userguide.svg") : "images/toolbar/blank.svg");
            enabled: (!(internal.isBusy || internal.isTrashed || internal.isErrorFetchingNote))
            onClicked: root.openNoteInfo();
        }
        ToolIconButton {
            iconId: (enabled? "toolbar-view-menu" : "");
            iconPath: (enabled? "" : "images/toolbar/blank.svg");
            enabled: (!(internal.isBusy || internal.isErrorFetchingNote))
            onClicked: root.openMenu();
        }
    }

    Component {
        id: noteViewDialogsManager
        NoteViewDialogs {
            anchors.fill: parent
            isTrashed: internal.isTrashed;
            title: internal.title
            notebookName: internal.notebookName;
            tagNames: internal.tagNames;
            tagCount: internal.tagCount;
            attachmentsCount: (internal.attachmentsData? internal.attachmentsData.length : 0)
            guid: internal.guid
            isEditingDisabled: internal.isEditingDisabled
            onNotebookClicked: root.openNotebookPicker();
            onTagsClicked: root.openTagsPicker();
            onNoteDetailsClicked: root.showNoteDetailsPage();
            onAttachmentsClicked: root.showAttachmentsListPage();
            onRestoreNoteClicked: root.restoreNote();
            onTrashMenuClicked: root.confirmTrash();
            onTrashNoteConfirmed: root.trashNote();
            onExpungeNoteConfirmed: root.expungeNote();
            onAppendTextMenuClicked: root.beginAppendText();
            onEditTitleMenuClicked: root.beginEditTitle();
        }
    }

    Loader {
        id: dialogsLoader
        anchors { fill: parent; topMargin: titleBar.height; }
        sourceComponent: undefined;
    }

    AddingAttachmentDialog {
        id: addingImageDialog
        window: root.window
        onAddingSucceededAndDialogClosed: {
            if (internal.attachmentsToAdd && (internal.attachmentsToAdd.length > 0)) {
                root.window.showInfoBanner(internal.attachmentsToAdd.length + " " +
                                           ((internal.attachmentsToAdd.length == 1)? "attachment" : "attachments") +
                                           " added to note");
            }
        }
    }

    QtObject {
        id: internal
        property string noteId: "";
        property bool isFavourite: false;
        property bool isTrashed: false;
        property variant baseContentHash;

        property string notebookName;
        property string tagNames;
        property int tagCount: 0;

        property bool isNoteFetchedConnected: false;
        property bool isBusy: true;
        property bool goBackRequested: false;
        property string loadedContentText;
        property string loadedTitleText;

        property string title: "";
        property string htmlContent: "";
        property string plainTextContent: "";
        property bool plainTextHasBeenChangedByUser: false;
        property bool richTextOrTitleHasBeenChangedByUser: false;
        property bool attachmentsChangedByUser: false;
        property bool isPlainText: true;
        property variant imagesModel; // used only in case of plaintext note
        property bool isImageExists: false;
        property variant attachmentsData;
        property variant updatedTime;
        property variant createdTime;
        property variant enmlContent;
        property variant checkboxStates;
        property variant savedCheckboxStates;
        property variant noteAttributes;

        property string guid;
        property bool isNotebookOrTagsChanged: false;

        property bool isEnmlParsingErrored: false;
        property bool isErrorFetchingNote: false;
        property bool isMarkedReadOnlyByEvernote: false;
        property bool isEditingDisabled: (isTrashed || isEnmlParsingErrored || isErrorFetchingNote || isMarkedReadOnlyByEvernote);

        property Item textAppendHelperSheetN9;
        property Item titleChangeHelperSheetN9;

        property variant attachmentsToAdd;

        property string unsavedNotebookId;
        property string unsavedTagId;

        property bool is24hourTimeFormat: (root.window && root.window.is24hourTimeFormat)? true : false;
    }

    Timer {
        id: delayedGetNoteDataTimer
        running: false
        repeat: false
        interval: 300 // a little after the pagestack transition completes
        onTriggered: root.load();
    }

    Timer {
        id: delayedAddAttachmentsTimer
        running: false
        repeat: false
        interval: 500 // a little after the note data is loaded
        onTriggered: {
            if (internal.attachmentsToAdd && (internal.attachmentsToAdd.length > 0)) {
                addingImageDialog.titleText = "Adding " + ((internal.attachmentsToAdd.length == 1)? "attachment" : "attachments");
                addingImageDialog.showDialogTillAddingCompletes();
                qmlDataAccess.startAppendAttachmentsToNote(internal.noteId, internal.attachmentsToAdd, internal.enmlContent, internal.baseContentHash);
            }
        }
    }

    Timer {
        id: autoSaveTimer
        running: false
        repeat: true
        interval: 5 * 60 * 1000 // every 5 mins
        onTriggered: root.saveUnlessWeHaveToCreateAnEmptyNote();
    }

    function delayedLoad(noteId, title) {
        internal.noteId = noteId;
        titleBar.text = title;
        internal.isBusy = true;
        delayedGetNoteDataTimer.start(); // when timer fires, load() is called
    }

    function delayedLoadAndAttach(noteId, title, attachments) {
        internal.noteId = noteId;
        internal.attachmentsToAdd = attachments;
        titleBar.text = title;
        internal.isBusy = true;
        delayedGetNoteDataTimer.start(); // when timer fires, load() is called
    }

    function delayedLoadNewNoteInNotebook(notebookId, notebookName) {
        internal.noteId = "";
        internal.unsavedNotebookId = notebookId;
        internal.notebookName = notebookName;
        internal.isBusy = true;
        delayedGetNoteDataTimer.start();
    }

    function delayedLoadNewNoteWithTag(tagId, tagName) {
        internal.noteId = "";
        internal.unsavedTagId = tagId;
        internal.tagNames = tagName;
        internal.tagCount = 1;
        internal.isBusy = true;
        delayedGetNoteDataTimer.start();
    }

    function load() {
        if (internal.noteId == "") { // new note
            titleBar.text = "New note";
            internal.plainTextContent = "Use the first line as the note title.";
            // If we start with empty text, keyboard switches to all caps the second time we do that
            internal.isPlainText = true;
            if (internal.notebookName == "") {
                internal.notebookName = qmlDataAccess.defaultNotebookName();
            }
            internal.imagesModel = new Array;
            internal.attachmentsData = new Array;
            contentLoader.sourceComponent = plainTextComponent;
            dialogsLoader.sourceComponent = noteViewDialogsManager;
            internal.isBusy = false;
            contentLoader.item.openKeyboardWithAllTextSelected();
        } else {                     // not a new note
            internal.isBusy = true;
            qmlDataAccess.startGetNoteData(internal.noteId);
        }
        autoSaveTimer.start();
    }

    function gotNoteTitle(noteTitle) {
        titleBar.text = noteTitle;
    }

    function gotNoteData(noteData) {
        var title = noteData.Title;
        var content = noteData.ParsedContent;
        titleBar.text = title;
        internal.title = title;
        internal.loadedTitleText = title;
        internal.isPlainText = noteData.IsPlainText;
        internal.isFavourite = noteData.Favourite;
        internal.isTrashed = noteData.Trashed;
        internal.baseContentHash = noteData.BaseContentHash;
        internal.attachmentsData = noteData.AllAttachments;
        internal.updatedTime = noteData.UpdatedTime;
        internal.createdTime = noteData.CreatedTime;
        internal.notebookName = noteData.NotebookName;
        internal.tagNames = noteData.TagNames;
        internal.tagCount = noteData.TagCount;
        internal.guid = noteData.guid;
        internal.isEnmlParsingErrored = noteData.IsEnmlParsingErrored;
        internal.imagesModel = noteData.ImageAttachments;
        internal.isImageExists = (internal.imagesModel.length > 0);
        internal.noteAttributes = {
            "author": noteData["Attributes/Author"],
            "source": noteData["Attributes/Source"],
            "sourceUrl": noteData["Attributes/SourceUrl"],
            "sourceApplication": noteData["Attributes/SourceApplication"],
            "contentClass": noteData["Attributes/ContentClass"]
        };
        internal.enmlContent = noteData.EnmlContent;
        if (internal.noteAttributes['contentClass']) {
            internal.isMarkedReadOnlyByEvernote = true;
        }

        if (noteData.IsPlainText) {
            internal.plainTextContent = title + "\n" + content;
            internal.loadedContentText = content;
            contentLoader.sourceComponent = plainTextComponent;
        } else {
            internal.htmlContent = content;
            internal.checkboxStates = noteData.CheckboxStates;
            internal.savedCheckboxStates = noteData.CheckboxStates;
            contentLoader.sourceComponent = richTextComponent;
        }

        dialogsLoader.sourceComponent = noteViewDialogsManager;
        internal.isBusy = false;
        if (internal.isMarkedReadOnlyByEvernote) {
            root.window.showInfoBanner("This is a special note that cannot be edited in Notekeeper\nEditing is disabled");
        } else if (internal.isEnmlParsingErrored) {
            root.window.showInfoBanner("Error parsing note\nNote loading might be incomplete\nEditing is disabled");
        } else if (internal.isPlainText && noteData.NonImageAttachmentsCount > 0) {
            var attachmentsStr = noteData.NonImageAttachmentsCount + (noteData.NonImageAttachmentsCount == 1? " attachment is" : " attachments are");
            if (internal.isTrashed) {
                root.window.showInfoBanner(attachmentsStr + " not shown.\n" + (noteData.NonImageAttachmentsCount == 1? "It" : "They")  + " can be seen only after restoring this note");
            } else {
                root.window.showInfoBanner(attachmentsStr + " not shown.\nTo see " + (noteData.NonImageAttachmentsCount == 1? "it" : "them") + ", go to Info > Attachments");
            }
        }
        internal.isErrorFetchingNote = false;
        if (internal.goBackRequested) { // maybe the reply came before the request got cancelled
            saveAndGoBack();
            return;
        }

        // Add any attachments if requested through createNoteRequest.ini
        if (internal.attachmentsToAdd && (internal.attachmentsToAdd.length > 0)) {
            delayedAddAttachmentsTimer.start();
        }
    }

    function errorFetchingNote(message) {
        if (internal.goBackRequested) {
            saveAndGoBack();
            return;
        }
        internal.isErrorFetchingNote = true;
        contentLoader.sourceComponent = fetchErrorTextComponent;
        if (!dialogsLoader.item) {
            dialogsLoader.sourceComponent = noteViewDialogsManager;
        }
        dialogsLoader.item.openInfoDialog("Could not fetch note", message);
        internal.isBusy = false;
    }

    function save() {
        var noteText = internal.plainTextContent;
        if (internal.isEditingDisabled) {
            return;
        }
        var noteId = "";
        var noChange = true;
        if (internal.isPlainText) {
            if (internal.noteId == "" || internal.plainTextHasBeenChangedByUser) {
                var map = qmlDataAccess.saveNote(internal.noteId, noteText, internal.attachmentsData, internal.loadedTitleText, internal.loadedContentText, internal.baseContentHash);
                noteId = map.NoteId;
                noChange = map.NoChange;
                if (map.IsEnmlChanged) {
                    internal.enmlContent = map.Enml;
                }
            }
        } else {
            var checkboxStatesModified = false;
            if (internal.savedCheckboxStates.length == internal.checkboxStates.length) {
                for (var i = 0; i < internal.savedCheckboxStates.length; i++) {
                    if (internal.savedCheckboxStates[i] != internal.checkboxStates[i]) {
                        checkboxStatesModified = true;
                    }
                }
                if (checkboxStatesModified) {
                    var changed = qmlDataAccess.saveCheckboxStates(internal.noteId, internal.checkboxStates, internal.enmlContent, internal.baseContentHash);
                    if (changed) {
                        noChange = false;
                    }
                    internal.savedCheckboxStates = internal.checkboxStates;
                }
            }
        }
        if (noteId != "") {
            internal.noteId = noteId;
        }
        if (internal.noteId != "") {
            qmlDataAccess.setFavouriteNote(internal.noteId, internal.isFavourite);
            if (internal.unsavedNotebookId != "") {
                qmlDataAccess.setNotebookForNote(internal.noteId, internal.unsavedNotebookId);
                internal.unsavedNotebookId = "";
            }
            if (internal.unsavedTagId != "" && internal.tagCount == 1) {
                qmlDataAccess.setTagsOnNote(internal.noteId, internal.unsavedTagId);
                internal.unsavedTagId = "";
            }
        }
        return (!noChange);
    }

    function saveUnlessWeHaveToCreateAnEmptyNote() {
        var canSave = false;
        if (internal.noteId != "") {
            canSave = true;
        } else {
            if (internal.plainTextHasBeenChangedByUser && internal.plainTextContent.match(/\S/)) {
                canSave = true;
            }
        }
        if (canSave) {
            return save();
        }
        return false;
    }

    function saveAndGoBack() {
        internal.goBackRequested = false;
        if (internal.isErrorFetchingNote) {
            pageStack.pop();
            return;
        }
        var changed = saveUnlessWeHaveToCreateAnEmptyNote();
        pageStack.pop();
        if (changed || internal.isNotebookOrTagsChanged || internal.richTextOrTitleHasBeenChangedByUser || internal.attachmentsChangedByUser) {
            SyncTrigger.triggerSync();
        }
    }

    function backButtonPressed() {
        internal.goBackRequested = true;
        if (internal.isBusy) {
            var wasBusy = qmlDataAccess.cancel();
            if (!wasBusy) {
                console.log("Was incorrectly assuming that we were busy.");
                saveAndGoBack();
            }
        } else {
            saveAndGoBack();
        }
    }

    function confirmTrash() {
        if (internal.isTrashed) {
            dialogsLoader.item.openExpungeConfirmDialog();
        } else {
            if (!internal.isPlainText || internal.plainTextContent || internal.noteId) {
                dialogsLoader.item.openTrashConfirmDialog();
            } else {
                trashNote(); // if no text on an unsaved plaintext note, delete without confirming
            }
        }
    }

    function trashNote() {
        saveUnlessWeHaveToCreateAnEmptyNote();
        if (internal.noteId) {
            qmlDataAccess.moveNoteToTrash(internal.noteId);
            SyncTrigger.triggerSync();
        }
        pageStack.pop();
    }

    function expungeNote() {
        if (internal.isTrashed) {
            if (internal.noteId) {
                qmlDataAccess.expungeNoteFromTrash(internal.noteId);
            }
            pageStack.pop();
        }
    }

    function restoreNote() {
        if (internal.isTrashed) {
            if (internal.noteId) {
                qmlDataAccess.restoreNoteFromTrash(internal.noteId);
                SyncTrigger.triggerSync();
            }
            pageStack.pop();
        }
    }

    function openMenu() {
        dialogsLoader.item.openMenu();
    }

    function openNoteInfo() {
        dialogsLoader.item.openNoteInfo();
    }

    function openNotebookPicker() {
        if (!internal.noteId) {
            save();
        }
        if (internal.noteId) {
            var page = pageStack.push(Qt.resolvedUrl("NotebookPickerPage.qml"), { window: root.window, notePage: root });
            page.setNoteId(internal.noteId);
        }
    }

    function openTagsPicker() {
        if (!internal.noteId) {
            save();
        }
        if (internal.noteId) {
            var page = pageStack.push(Qt.resolvedUrl("TagsPickerPage.qml"), { window: root.window, notePage: root });
            page.setNoteId(internal.noteId);
        }
    }

    function setFavourite(isFav) {
        internal.isFavourite = isFav;
        if (internal.noteId != "") {
            qmlDataAccess.setFavouriteNote(internal.noteId, internal.isFavourite);
        }
    }

    function updateNotebook(name) {
        if (internal.notebookName != name) {
            internal.notebookName = name;
            internal.isNotebookOrTagsChanged = true;
        }
    }

    function updateTags(names, count) {
        if (internal.tagNames != names || internal.tagCount != count) {
            internal.tagNames = names;
            internal.tagCount = count;
            internal.isNotebookOrTagsChanged = true;
        }
    }

    function beginAppendText() {
        if (internal.isPlainText) {
            contentLoader.item.beginAppendText();
        } else {
            if (is_harmattan) {
                if (!internal.textAppendHelperSheetN9) {
                    var component = Qt.createComponent("TextAppendHelperSheet_N9.qml");
                    if (component.status == Component.Ready) {
                        internal.textAppendHelperSheetN9 = component.createObject(root);
                        internal.textAppendHelperSheetN9.okClicked.connect(appendTextToRichText);
                    } else {
                        console.debug("Error loading TextAppendHelperSheet_N9.qml: " + component.errorString());
                    }
                }
                internal.textAppendHelperSheetN9.reset();
                internal.textAppendHelperSheetN9.open();
                internal.textAppendHelperSheetN9.openKeyboard();
            } else {
                var page = pageStack.push(Qt.resolvedUrl("TextChangeHelperPage.qml"), { okButtonText: "Append" });
                page.load("Text to append");
                // If we start with empty text, keyboard switches to all caps the second time we do that
                page.openKeyboardWithAllTextSelected();
                page.okClicked.connect(appendTextToRichText);
            }
        }
    }

    function appendTextToRichText(text) {
        if (!internal.isPlainText && text != "") {
            var appendResult = qmlDataAccess.appendTextToNote(internal.noteId, text, internal.enmlContent, internal.baseContentHash);
            if (appendResult.Appended) {
                internal.enmlContent = appendResult.UpdatedEnml;
                var changedHtml = internal.htmlContent + appendResult.HtmlToAdd;
                internal.htmlContent = changedHtml;
                internal.richTextOrTitleHasBeenChangedByUser = true;
            }
        }
    }

    function beginEditTitle() {
        if (internal.isPlainText) {
            contentLoader.item.beginEditFirstLine();
        } else {
            if (is_harmattan) {
                if (!internal.titleChangeHelperSheetN9) {
                    var component = Qt.createComponent("TitleChangeHelperSheet_N9.qml");
                    if (component.status == Component.Ready) {
                        internal.titleChangeHelperSheetN9 = component.createObject(root);
                        internal.titleChangeHelperSheetN9.okClicked.connect(setTitleForRichTextNote);
                    } else {
                        console.debug("Error loading TitleChangeHelperSheet_N9.qml: " + component.errorString());
                    }
                }
                internal.titleChangeHelperSheetN9.setText(internal.title);
                internal.titleChangeHelperSheetN9.open();
                internal.titleChangeHelperSheetN9.openKeyboard();
            } else {
                var page = pageStack.push(Qt.resolvedUrl("TextChangeHelperPage.qml"), { okButtonText: "Save", singleLineInput: true });
                page.load(internal.title);
                page.beginEditFirstLine();
                page.okClicked.connect(setTitleForRichTextNote);
            }
        }
    }

    function setTitleForRichTextNote(text) {
        if (!internal.isPlainText && text != "") {
            var changed = qmlDataAccess.setNoteTitle(internal.noteId, text);
            if (changed) {
                internal.title = text;
                titleBar.text = text;
                internal.richTextOrTitleHasBeenChangedByUser = true;
            }
        }
    }

    function showNoteDetailsPage() {
        var page = pageStack.push(Qt.resolvedUrl("NoteDetailsPage.qml"),
                                  { title: internal.title,
                                    updatedTime: internal.updatedTime,
                                    createdTime: internal.createdTime,
                                    notebookName: internal.notebookName,
                                    tagNames: internal.tagNames,
                                    window: root.window
                                  });
        page.load(internal.noteAttributes);
    }

    function showImagePage(i) {
        var page = pageStack.push(Qt.resolvedUrl("ImageBrowserPage.qml"),
                                  { imagesModel: internal.imagesModel, startingIndex: i,
                                    canRemoveImages: (internal.isPlainText && !internal.isEditingDisabled),
                                    window: root.window, notePage: root });
        page.removeImage.connect(removeImage);
        page.delayedLoad();
    }

    function removeImage(index) {
        if (!internal.isPlainText) {
            return;
        }
        var imagesModel = internal.imagesModel;
        var attachmentsData = internal.attachmentsData;
        var removedImageItem = (imagesModel.splice(index, 1))[0];
        var attachmentIndex = removedImageItem.AttachmentIndex;
        var map = qmlDataAccess.removeAttachmentInstanceFromNote(internal.noteId, internal.plainTextContent, internal.attachmentsData, attachmentIndex, internal.baseContentHash);
        if (map.Removed) {
            for (var i = index; i < imagesModel.length; i++) {
                imagesModel[i].AttachmentIndex = (imagesModel[i].AttachmentIndex - 1);
            }
            internal.enmlContent = map.Enml;
            internal.attachmentsData = map.AttachmentsData;
            internal.imagesModel = imagesModel;
            internal.attachmentsChangedByUser = true;
            root.imageRemoved(index);
        }
    }

    function removeAttachment(index) {
        if (!internal.isPlainText) {
            return;
        }
        var imagesModel = internal.imagesModel;
        var attachmentsData = internal.attachmentsData;

        var mimeType = attachmentsData[index].MimeType;
        if (mimeType.indexOf("image/") == 0) {
            // it's an image attachment
            var imageIndex = -1;
            for (var i = 0; i < imagesModel.length; i++) {
                if (imagesModel[i].AttachmentIndex == index) {
                    imageIndex = i;
                    break;
                }
            }
            if (imageIndex >= 0) {
                removeImage(imageIndex);
                root.attachmentRemoved(index); // emit signal
            }
            return;
        }

        // not an image attachment
        var map = qmlDataAccess.removeAttachmentInstanceFromNote(internal.noteId, internal.plainTextContent, internal.attachmentsData, index, internal.baseContentHash);
        if (map.Removed) {
            internal.enmlContent = map.Enml;
            internal.attachmentsData = map.AttachmentsData;
            internal.imagesModel = imagesModel;
            internal.attachmentsChangedByUser = true;
            root.attachmentRemoved(index); // emit signal
        }
    }

    function showAddImagePage() {
        if (internal.isBusy || internal.isEditingDisabled) {
            return;
        }
        var page = pageStack.push(Qt.resolvedUrl("PhoneGalleryPage.qml"), { window: root.window });
        page.imageClicked.connect(appendImage);
        page.delayedLoad();
    }

    function appendImage(imageLocalUrlPath) {
        if (imageLocalUrlPath != "") {
            if (!internal.noteId) {
                save();
            }
            // dialog will be opened by PhoneGalleryPage
            qmlDataAccess.startAppendImageToNote(internal.noteId, imageLocalUrlPath, internal.enmlContent, internal.baseContentHash);
        }
    }

    function updateViewWithAppendedAttachment(appendResult) {
        if (!appendResult.Appended) {
            return;
        }
        if ((internal.noteId == "") || (appendResult.NoteId != internal.noteId)) {
            return;
        }
        internal.enmlContent = appendResult.UpdatedEnml;
        if (!internal.plainTextContent) {
            var changedHtml = internal.htmlContent + appendResult.HtmlToAdd;
            internal.htmlContent = changedHtml;
        }

        var imagesModel = internal.imagesModel;
        var attachmentsData = internal.attachmentsData;

        if (appendResult.IsImage) {
            imagesModel.push( {
                                 'guid'    : appendResult.guid, // should be empty guid
                                 'Hash'    : appendResult.Hash,
                                 'Url'     : appendResult.Url,
                                 'Width'   : appendResult.Width,
                                 'Height'  : appendResult.Height,
                                 'FileName'        : appendResult.FileName,
                                 'Size'            : appendResult.Size,
                                 'MimeType'        : appendResult.MimeType,
                                 'AttachmentIndex' : attachmentsData.length
                             } );
            internal.imagesModel = imagesModel;
            internal.isImageExists = true;
        }

        attachmentsData.push( {
                                 'guid'         : appendResult.guid, // should be empty guid
                                 'Hash'         : appendResult.Hash,
                                 'Url'          : appendResult.Url,
                                 'MimeType'     : appendResult.MimeType,
                                 'TrailingText' : '',
                                 'FileName'     : appendResult.FileName,
                                 'Size'         : appendResult.Size,
                                 'Width'        : appendResult.Width,
                                 'Height'       : appendResult.Height,
                                 'Duration'     : 0
                             } );
        internal.attachmentsData = attachmentsData;

        internal.attachmentsChangedByUser = true;
        if (!internal.isBusy) {
            contentLoader.item.scrollToBottom();
        }
    }

    function showAttachmentsListPage() {
        var page = pageStack.push(Qt.resolvedUrl("AttachmentsListPage.qml"),
                                  {
                                      window: root.window,
                                      notePage: root,
                                      canRemoveAttachments: (internal.isPlainText && !internal.isEditingDisabled)
                                  });
        page.removeAttachment.connect(removeAttachment);
        page.delayedLoad(internal.attachmentsData);
    }

    function showAttachmentPage(index) {
        if (index < 0 || index >= internal.attachmentsData.length) {
            return;
        }
        var page = root.pageStack.push(Qt.resolvedUrl("AttachmentPage.qml"),
                                       {
                                           window: root.window,
                                           notePage: root,
                                           canRemoveAttachments: (internal.isPlainText && !internal.isEditingDisabled)
                                       });
        page.removeAttachment.connect(removeAttachment);
        page.delayedLoad(index, internal.attachmentsData[index]);
    }

    function noteTitle() {
        return internal.title;
    }

    Component.onCompleted: {
        qmlDataAccess.gotNoteData.connect(gotNoteData);
        qmlDataAccess.gotNoteTitle.connect(gotNoteTitle);
        qmlDataAccess.errorFetchingNote.connect(errorFetchingNote);
        qmlDataAccess.addedAttachment.connect(updateViewWithAppendedAttachment);
        qmlDataAccess.aboutToQuit.connect(saveUnlessWeHaveToCreateAnEmptyNote);
    }

    Component.onDestruction: {
        qmlDataAccess.gotNoteData.disconnect(gotNoteData);
        qmlDataAccess.gotNoteTitle.disconnect(gotNoteTitle);
        qmlDataAccess.errorFetchingNote.disconnect(errorFetchingNote);
        qmlDataAccess.addedAttachment.disconnect(updateViewWithAppendedAttachment);
        qmlDataAccess.aboutToQuit.disconnect(saveUnlessWeHaveToCreateAnEmptyNote);
        if (internal.textAppendHelperSheetN9) {
            internal.textAppendHelperSheetN9.destroy();
        }
    }
}
