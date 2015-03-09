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

Page {
    id: root
    width: 100
    height: 62
    anchors.fill: parent
    property string title: ""
    property variant createdTime;
    property variant updatedTime;
    property string notebookName;
    property string tagNames;
    property variant noteAttributes;
    property variant window;

    UiStyle {
        id: _UI
    }

    Rectangle {
        anchors.fill: parent
        color: _UI.pageBgColor
    }

    TitleBar {
        id: titleBar
        anchors { left: parent.left; right: parent.right; top: parent.top; }
        text: root.title
    }

    ListView {
        id: listView
        anchors { fill: parent; topMargin: titleBar.height; }
        clip: true
        model: VisualItemModel {
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Timestamps"
            }
            GenericListItem {
                enabled: false
                width: listView.width
                height: lastUpdatedTime.height
                TimestampDetailItem { id: lastUpdatedTime; keyText: "Last updated:"; timestamp: root.updatedTime; is24hourTimeFormat: internal.is24hourTimeFormat; }
            }
            GenericListItem {
                enabled: false
                width: listView.width
                height: createdTime.height
                TimestampDetailItem { id: createdTime; keyText: "Created:"; timestamp: root.createdTime; is24hourTimeFormat: internal.is24hourTimeFormat; }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Source"
            }
            Column {
                id: sourceItemsColumn
                width: listView.width
                Repeater {
                    model: sourceItemsModel
                    delegate: Component {
                        SettingsItem {
                            width: sourceItemsColumn.width
                            topTextRole: "Subtitle"
                            topText: model.key
                            bottomTextRole: "Title"
                            bottomText: model.value
                            itemClickable: (model.isUrl? true : false)
                            onItemClicked: {
                                window.openUrlExternally(model.value);
                            }
                        }
                    }
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Notebook"
            }
            GenericListItem {
                enabled: false
                width: listView.width
                height: notebookNameText.height + (_UI.paddingLarge * 2)
                TextLabel {
                    id: notebookNameText
                    anchors { left: parent.left; leftMargin: _UI.paddingLarge; top: parent.top; topMargin: _UI.paddingLarge; }
                    width: parent.width - (_UI.paddingLarge * 2)
                    text: root.notebookName
                    wrapMode: Text.Wrap
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Tags"
            }
            GenericListItem {
                enabled: false
                width: listView.width
                height: tagNamesText.height + (_UI.paddingLarge * 2)
                TextLabel {
                    id: tagNamesText
                    anchors { left: parent.left; leftMargin: _UI.paddingLarge; top: parent.top; topMargin: _UI.paddingLarge; }
                    width: parent.width - (_UI.paddingLarge * 2)
                    text: (root.tagNames == ""? "No tags" : root.tagNames)
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: listView
    }

    tools: ToolBarLayout {
        id: toolbarLayout
        ToolIconButton {
            iconId: "toolbar-back";
            onClicked: root.pageStack.pop();
        }
    }

    ListModel {
        id: sourceItemsModel
        property string key;
        property string value;
        property bool isUrl;
    }

    QtObject {
        id: internal
        property bool is24hourTimeFormat: (root.window && root.window.is24hourTimeFormat)? true : false;
    }

    function load(attributes) {
        sourceItemsModel.clear();
        if (attributes.sourceUrl) {
            sourceItemsModel.append( { "key" : "Source URL", "value" : attributes.sourceUrl, "isUrl": true } );
        }
        if (attributes.sourceApplication) {
            sourceItemsModel.append( { "key" : "Source Application", "value" : attributes.sourceApplication } );
        }
        if (attributes.source) {
            sourceItemsModel.append( { "key" : "Source", "value" : attributes.source } );
        }
        if (attributes.author) {
            sourceItemsModel.append( { "key" : "Author", "value" : attributes.author } );
        }
        if (sourceItemsModel.count == 0) {
            sourceItemsModel.append( { "key" : "No attributes", "value" : "" } );
        }
    }
}
