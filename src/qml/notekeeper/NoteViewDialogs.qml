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
    property bool isTrashed: false;
    property bool isEditingDisabled: false;
    property string notebookName;
    property string tagNames;
    property int tagCount;
    property int attachmentsCount;
    property string title; // note's title
    property string guid; // note's guid

    signal notebookClicked()
    signal tagsClicked()
    signal noteDetailsClicked()
    signal attachmentsClicked()

    signal toggleFavouriteClicked()
    signal restoreNoteClicked()
    signal trashMenuClicked()
    signal trashNoteConfirmed()
    signal expungeNoteConfirmed()
    signal appendTextMenuClicked()
    signal editTitleMenuClicked()

    UiStyle {
        id: _UI
    }

    QueryDialog {
        id: trashConfirmDialog
        titleText: "Move note to Trash?"
        message: root.title
        acceptButtonText: "Trash"
        rejectButtonText: "Keep"
        onAccepted: root.trashNoteConfirmed();
        height: 150
    }

    QueryDialog {
        id: expungeConfirmDialog
        titleText: "Permanently delete note?"
        message: root.title
        acceptButtonText: "Delete"
        rejectButtonText: "Keep"
        onAccepted: root.expungeNoteConfirmed();
        height: 150
    }

    QueryDialog {
        id: messageBox
        acceptButtonText: "Ok"
        height: 150
    }

    Menu {
        id: mainMenuForNormalNote
        content: MenuLayout {
            MenuItem { enabled: (!root.isEditingDisabled); text: "Append text"; onClicked: root.appendTextMenuClicked(); }
            MenuItem { enabled: (!root.isEditingDisabled); text: "Edit title"; onClicked: root.editTitleMenuClicked(); }
            MenuItem { enabled: (!root.isEditingDisabled); text: "Move to Trash"; onClicked: root.trashMenuClicked(); }
        }
    }

    Menu {
        id: mainMenuForTrashedNote
        content: MenuLayout {
            MenuItem { text: "Restore note"; onClicked: root.restoreNoteClicked(); }
            MenuItem { text: "Permanently delete note";
                       onClicked: expungeConfirmDialog.open();
                       enabled: (root.guid == "") // can expunge only those notes that haven't been synced
                     }
        }
    }

    Menu {
        id: noteInfoMenu
        content: Item {
            id: infoMenuContent
            property bool useTwoRows: (_UI.isPortrait || is_harmattan)
            width: parent.width;
            height: (_UI.isPortrait? _UI.screenHeight / 2.5 : (is_harmattan? noteInfoMenu.height - 36 : _UI.screenHeight / 4))
            signal clicked;
            onClicked: {
                if (is_harmattan) {
                    parent.closeMenu();
                }
            }
            Flow {
                anchors.fill: parent
                CustomMenuItem {
                    width: (infoMenuContent.useTwoRows? parent.width / 2 : parent.width / 4)
                    height: (infoMenuContent.useTwoRows? parent.height / 2 : parent.height)
                    menuPosition: (infoMenuContent.useTwoRows? "top-left" : "horizontal-left")
                    imageSource: (is_harmattan? "images/menu/notebook_n9" + (_UI.isDarkColorScheme? "_white" : "") + ".svg" : "images/menu/notebook.svg");
                    heading: "Notebook";
                    subheading: root.notebookName;
                    maximumTextWidth: (width - (_UI.paddingLarge * 2))
                    onClicked: {
                        infoMenuContent.clicked();
                        root.notebookClicked();
                    }
                }
                CustomMenuItem {
                    width: (infoMenuContent.useTwoRows? parent.width / 2 : parent.width / 4)
                    height: (infoMenuContent.useTwoRows? parent.height / 2 : parent.height)
                    menuPosition: (infoMenuContent.useTwoRows? "top-right" : "horizontal-center")
                    imageSource: (is_harmattan? "images/menu/tag_n9" + (_UI.isDarkColorScheme? "_white" : "") + ".svg" : "images/menu/tag.svg");
                    heading: {
                        if (root.tagCount == 0) {
                            return "No Tags";
                        } else if (root.tagCount == 1) {
                            return "1 Tag";
                        } else {
                            return (root.tagCount + " Tags");
                        }
                    }
                    subheading: root.tagNames;
                    maximumTextWidth: (width - (_UI.paddingLarge * 2))
                    onClicked: {
                        infoMenuContent.clicked();
                        root.tagsClicked();
                    }
                }
                CustomMenuItem {
                    width: (infoMenuContent.useTwoRows? parent.width / 2 : parent.width / 4)
                    height: (infoMenuContent.useTwoRows? parent.height / 2 : parent.height)
                    menuPosition: (infoMenuContent.useTwoRows? "bottom-left" : "horizontal-center")
                    imageSource: (is_harmattan? "images/menu/note_n9" + (_UI.isDarkColorScheme? "_white" : "") + ".svg" : "images/menu/note.svg");
                    heading: "Note details";
                    subheading: "";
                    maximumTextWidth: (width - (_UI.paddingLarge * 2))
                    onClicked: {
                        infoMenuContent.clicked();
                        root.noteDetailsClicked();
                    }
                }
                CustomMenuItem {
                    width: (infoMenuContent.useTwoRows? parent.width / 2 : parent.width / 4)
                    height: (infoMenuContent.useTwoRows? parent.height / 2 : parent.height)
                    menuPosition: (infoMenuContent.useTwoRows? "bottom-right" : "horizontal-right")
                    imageSource: (is_harmattan? "images/menu/attachment_n9" + (_UI.isDarkColorScheme? "_white" : "") + ".svg" : "images/menu/attachment.svg");
                    heading: "Attachments";
                    subheading: (root.attachmentsCount? root.attachmentsCount : "None");
                    maximumTextWidth: (width - (_UI.paddingLarge * 2))
                    onClicked: {
                        infoMenuContent.clicked();
                        root.attachmentsClicked();
                    }
                }
            }
            // separator lines
            Rectangle {
                anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; }
                width: 1;
                height: parent.height;
                color: (is_symbian? "#4d4d4d" : "transparent");
            }
            Rectangle { // horizontal line when using two rows, vertical line otherwise
                width: (infoMenuContent.useTwoRows? parent.width / 2 : 1);
                height: (infoMenuContent.useTwoRows? 1 : parent.height);
                x: (infoMenuContent.useTwoRows? 0 : parent.width * 0.25);
                y: (infoMenuContent.useTwoRows? parent.height / 2 : 0);
                color: (is_symbian? "#4d4d4d" : "transparent");
            }
            Rectangle { // horizontal line when using two rows, vertical line otherwise
                width: (infoMenuContent.useTwoRows? parent.width / 2 : 1);
                height: (infoMenuContent.useTwoRows? 1 : parent.height);
                x: (infoMenuContent.useTwoRows? parent.width / 2 : parent.width * 0.75);
                y: (infoMenuContent.useTwoRows? parent.height / 2 : 0);
                color: (is_symbian? "#4d4d4d" : "transparent");
            }
        }
    }

    function openTrashConfirmDialog() {
        trashConfirmDialog.open();
    }

    function openExpungeConfirmDialog() {
        expungeConfirmDialog.open();
    }

    function openInfoDialog(title, message) {
        if (messageBox.status == _UI.dialogStatus_Closed) {
            messageBox.titleText = title;
            messageBox.message = message;
            messageBox.open();
        }
    }

    function openMenu() {
        if (root.isTrashed) {
            mainMenuForTrashedNote.open();
        } else {
            mainMenuForNormalNote.open();
        }
    }

    function openNoteInfo() {
        noteInfoMenu.open();
    }
}
