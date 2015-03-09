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
        text: "Advanced Settings"
        iconPath: "images/titlebar/settings_white.svg"
    }

    ListView {
        id: listView
        anchors { fill: parent; topMargin: titleBar.height; }
        clip: true
        model: VisualItemModel {
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Disk cache"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Disk cache maximum size"
                bottomTextRole: "Title"
                bottomText: ((typeof internal.diskCacheSize !== undefined)? Humanize.filesizeformat(internal.diskCacheSize, decimal_point_char) : "Unknown")
                controlItemType: "Button"
                buttonText: "Change"
                onButtonClicked: {
                    cacheSizePickerDialog.cacheSizeMBs = Math.floor(internal.diskCacheSize / 1048576);
                    cacheSizePickerDialog.open();
                }
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Current space used"
                bottomTextRole: "Title"
                bottomText: ((typeof internal.diskCacheUsed !== undefined)? Humanize.filesizeformat(internal.diskCacheUsed, decimal_point_char) : "Unknown")
                controlItemType: "Button"
                buttonText: "Clear cache"
                buttonEnabled: (internal.diskCacheUsed)
                onButtonClicked: {
                    qmlDataAccess.clearDiskCache();
                    root.loadDiskCacheInfo();
                }
            }
            GenericListHeading {
                width: listView.width
                horizontalAlignment: Qt.AlignLeft
                text: "Logging"
            }
            SettingsItem {
                width: listView.width
                topTextRole: "Subtitle"
                topText: "Log messages for debug"
                controlItemType: "SubItemIndicator"
                itemClickable: true
                onItemClicked: {
                    var page = root.pageStack.push(Qt.resolvedUrl("LoggingPage.qml"), { window: root.window });
                    page.load();
                }
            }
            Column {
                property bool shown: (is_symbian? true : false)
                height: (shown? implicitHeight : 0)
                opacity: (shown? 1 : 0)
                GenericListHeading {
                    id: settingWarnOnUnusualIAPOrderingHeader
                    width: listView.width
                    horizontalAlignment: Qt.AlignLeft
                    text: "Show warnings on"
                }
                SettingsItem {
                    id: settingWarnOnUnusualIAPOrdering
                    width: listView.width
                    topTextRole: "Title"
                    topText: "Unusual access points order"
                    bottomTextRole: "Subtitle"
                    bottomText: "in phone settings"
                    controlItemType: "Switch"
                }
            }
        }
    }

    ScrollDecorator {
        flickableItem: listView
    }

    tools: ToolBarLayout {
        ToolIconButton { iconId: "toolbar-back"; onClicked: root.saveAndGoBack(); }
    }

    GenericDialog {
        id: cacheSizePickerDialog
        property bool isValueChanged: false;
        property int cacheSizeMBs: 0;
        titleText: "Change disk cache max size"
        buttonTexts: [ "Ok", "Cancel" ]
        content: Item {
            width: parent.width
            height: 150
            Item {
                anchors { fill: parent; margins: _UI.paddingLarge; }
                Column {
                    id: cacheSizePickerColumn
                    anchors.centerIn: parent
                    spacing: _UI.paddingSmall
                    width: 300
                    Row {
                        id: cacheSizePickerRow
                        anchors.horizontalCenter: parent.horizontalCenter
                        property int count: (cacheSizePickerSlider.maximumValue / 10)
                        width: cacheSizePickerColumn.width
                        Repeater {
                            model: cacheSizePickerRow.count
                            TextLabel {
                                width: cacheSizePickerRow.width / cacheSizePickerRow.count
                                horizontalAlignment: Text.AlignHCenter
                                text: ((index + 1) * 10)
                                color: "#fff"
                            }
                        }
                    }
                    Slider {
                        id: cacheSizePickerSlider
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 300
                        minimumValue: 10
                        maximumValue: 50
                        stepSize: 10
                        value: cacheSizePickerDialog.cacheSizeMBs
                        onValueChanged: {
                            if ((value % stepSize) === 0) {
                                if (value != cacheSizePickerDialog.cacheSizeMBs) {
                                    cacheSizePickerDialog.cacheSizeMBs = value;
                                    cacheSizePickerDialog.isValueChanged = true;
                                }
                            }
                        }
                    }
                    TextLabel {
                        id: cacheSizePickerText
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: cacheSizePickerDialog.cacheSizeMBs + " MB"
                        color: "#fff"
                    }
                }
            }
        }
        onButtonClicked: {
            if (index == 0) { // Ok clicked
                if (cacheSizePickerDialog.isValueChanged) {
                    qmlDataAccess.setDiskCacheMaximumSize(cacheSizePickerDialog.cacheSizeMBs * 1048576);
                    root.loadDiskCacheInfo();
                }
            }
        }
    }

    QtObject {
        id: internal
        property int diskCacheSize;
        property int diskCacheUsed;
    }

    function load() {
        settingWarnOnUnusualIAPOrdering.switchChecked = (!qmlDataAccess.retrieveSetting("Warn/dontWarnOnUnusualIAPOrdering"));
        loadDiskCacheInfo();
    }

    function loadDiskCacheInfo() {
        var diskCacheInfo = qmlDataAccess.diskCacheInfo();
        if (diskCacheInfo && 'Max' in diskCacheInfo) {
            internal.diskCacheSize = diskCacheInfo['Max'];
        }
        if (diskCacheInfo && 'Used' in diskCacheInfo) {
            internal.diskCacheUsed = diskCacheInfo['Used'];
        }
    }

    function saveAndGoBack() {
        save();
        pageStack.pop();
    }

    function save() {
        qmlDataAccess.saveSetting("Warn/dontWarnOnUnusualIAPOrdering", (!settingWarnOnUnusualIAPOrdering.switchChecked));
    }
}
