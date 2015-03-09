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
    property string searchBoxPlaceholderText: "";
    property bool checkMarkShown: true;
    property bool canSelectExactlyOneItem: false;
    property string headerText: "";
    property string selectedItemsHeaderText: "Selected items";
    property string selectedItemsHeaderTextOnNullSelection: " ";
    property string allItemsHeaderText: "All items";
    property int itemWidth: (width * 0.8)
    property bool isSearchable: true // if false, can't search / create

    signal createObject(string name)
    signal searchModeEntered()
    signal searchModeExited()

    UiStyle {
        id: _UI
    }

    QtObject {
        id: internal
        property variant allItemsListModel;
        property bool showSearchResults: false;
        property int animationDuration: 200; // ms
        property int roundedRectRadius: 20;
        property bool selectedItemsListInitialized: false;
        property string matchingItemsSearchString;
        property bool isExactMatchFound: false;

        property bool keyboardShown: _UI.isVirtualKeyboardOpen;
        onKeyboardShownChanged: {
            if (!keyboardShown && showSearchResults) { // user dismissed the keyboard
                if (searchbox.searchText == "") {
                    hideSearchResults();
                }
            }
        }

        function hideSearchResults() {
            internal.showSearchResults = false;
            searchbox.searchText = "";
            searchboxFocusManager.closeVirtualKeyboard();
            root.searchModeExited();
        }

        function deselectItemByObjectId(id) {
            var selectedItemsIndex = -1;
            var allItemsIndex = -1;
            var listModel = selectedItemsModel;
            for (var i = 0; i < listModel.count; i++) {
                var item = listModel.get(i);
                if (item.objectId == id) {
                    selectedItemsIndex = i;
                    allItemsIndex = item.allItemsListIndex;
                    break;
                }
            }
            if ((allItemsIndex >= 0 && allItemsIndex < internal.allItemsListModel.count) &&
                (selectedItemsIndex >= 0 && selectedItemsIndex < selectedItemsModel.count)) {
                var ok = selectedItemsModelDelayedRemover.delayedRemove(selectedItemsIndex);
                if (ok) {
                    selectedItemsRepeater.itemAt(selectedItemsIndex).hide();
                    internal.allItemsListModel.setProperty(allItemsIndex, "checked", false);
                }
            }
        }

        function selectItemByAllItemsIndex(allItemsIndex) {
            if (allItemsIndex >= 0 && allItemsIndex < internal.allItemsListModel.count) {
                var item = internal.allItemsListModel.get(allItemsIndex);
                internal.allItemsListModel.setProperty(allItemsIndex, "checked", true);
                var listModel = selectedItemsModel;
                var insertPosition = listModel.count;
                for (var i = 0; i < listModel.count; i++) {
                    var name = listModel.get(i).name;
                    if (item.name < name) {
                        insertPosition = i;
                        break;
                    }
                }
                selectedItemsModel.insert(insertPosition, { objectId: item.objectId, name: item.name, allItemsListIndex: allItemsIndex } );
            }
        }

        function setExclusiveSelectedItemByAllItemsIndex(allItemsIndex) {
            var listModel = internal.allItemsListModel;
            if (allItemsIndex >= 0 && allItemsIndex < listModel.count) {
                // update selected items list
                var item = listModel.get(allItemsIndex);
                selectedItemsModel.set(0, { objectId: item.objectId, name: item.name, allItemsListIndex: allItemsIndex } );
                // update all items list
                for (var i = 0; i < listModel.count; i++) {
                    listModel.setProperty(i, "checked", (i == allItemsIndex));
                }
                // update search results
                for (var im = 0; im < matchingItemsModel.count; im++) {
                    matchingItemsModel.setProperty(im, "checked", (matchingItemsModel.get(im).allItemsListIndex == allItemsIndex));
                }
            }
        }

        function updateMatchingItems(searchString) {

            internal.isExactMatchFound = false;

            if (searchString.length == 0) {
                matchingItemsModel.clear();
                internal.matchingItemsSearchString = searchString;
                return;
            }

            var searchWithinCurrentResult = (internal.matchingItemsSearchString.length > 0 &&
                                             searchString.indexOf(internal.matchingItemsSearchString) >= 0);
            var listToSearch;
            if (searchWithinCurrentResult) {
                listToSearch = matchingItemsModel;
                // we will be removing unmatching items from the current matchingItemsModel
            } else {
                listToSearch = internal.allItemsListModel;
                // clear all items from matchingItemsModel - we'll populate it afresh
                matchingItemsModel.clear();
            }

            var i = 0;
            while (i < listToSearch.count) {
                var item = listToSearch.get(i);
                var name = item.name;
                var foundPos = name.toLowerCase().indexOf(searchString.toLowerCase());
                var matched = (foundPos >= 0);
                var richTextName;
                if (matched) {
                    richTextName = name.substr(0, foundPos) +
                                   "<u>" + name.substr(foundPos, searchString.length) + "</u>" +
                                   name.substring(foundPos + searchString.length, name.length);
                    if (foundPos == 0 && name.length == searchString.length) {
                        internal.isExactMatchFound = true;
                    }
                }
                if (searchWithinCurrentResult) {
                    if (matched) { // update richTextName
                        listToSearch.setProperty(i, "richTextName", richTextName);
                        ++i;
                    } else { // remove item
                        listToSearch.remove(i);
                    }
                } else {
                    if (matched) {
                        matchingItemsModel.append({ objectId: item.objectId, name: item.name, richTextName: richTextName, checked: item.checked, allItemsListIndex: i });
                    }
                    ++i;
                }
            }
            internal.matchingItemsSearchString = searchString;
        }

        function insertItemInList(listModel, dict) {
            var name = dict.name;
            if (!name) {
                return -1;
            }
            var insertPosition = listModel.count;
            for (var i = 0; i < listModel.count; i++) {
                var nameAtIndex = listModel.get(i).name;
                if (name < nameAtIndex) {
                    insertPosition = i;
                    break;
                }
            }
            listModel.insert(insertPosition, dict);
            return insertPosition;
        }

        function updateAllItemsIndexInList(listModel, newlyInsertedAllItemsIndex) {
            for (var i = 0; i < listModel.count; i++) {
                var allItemsIndex = listModel.get(i).allItemsListIndex;
                if (allItemsIndex >= newlyInsertedAllItemsIndex) {
                    listModel.setProperty(i, "allItemsListIndex", allItemsIndex + 1);
                }
            }
        }
    }

    ListModel {
        id: selectedItemsModel
        property string objectId;
        property string name;
        property int allItemsListIndex;
    }

    ListModel {
        id: matchingItemsModel
        property string objectId;
        property string name;
        property string richTextName;
        property bool checked;
        property int allItemsListIndex;
    }

    Timer {
        // Since we animate the removal of items from the selected items list,
        // we need to delay the removal of the item till the animation completes
        id: selectedItemsModelDelayedRemover;
        property int indexToRemove: -1;
        interval: internal.animationDuration;
        running: false;
        onTriggered: { if (indexToRemove >= 0) { selectedItemsModel.remove(indexToRemove); } }
        function delayedRemove(index) {
            if (selectedItemsModelDelayedRemover.running) {
                return false;
            }
            selectedItemsModelDelayedRemover.indexToRemove = index;
            selectedItemsModelDelayedRemover.start();
            return true;
        }
    }

    Flickable {
        id: topLevelFlickable
        width: root.width
        height: root.height
        contentWidth: column.width
        contentHeight: column.height
        interactive: (!internal.showSearchResults) // when showing search results, only the results scroll, not the searchbox
        flickableDirection: Flickable.VerticalFlick

        Column {
            id: column
            width: root.width

            Item {
                width: parent.width
                height: (root.isSearchable? searchboxFocusManager.height : 0)
                opacity: (root.isSearchable? 1 : 0)
                FocusScope {
                    id: searchboxFocusManager
                    anchors { top: parent.top; left: parent.left; right: parent.right; }
                    height: searchbox.height
                    GenericSearchBar {
                        id: searchbox
                        anchors.fill: parent
                        backButton: internal.showSearchResults
                        placeHolderText: root.searchBoxPlaceholderText
                        onSearchTextChanged: { internal.updateMatchingItems(searchbox.searchText); }
                        focus: false
                        rightButton: (!_UI.isPortrait && internal.showSearchResults)
                        rightButtonText: "Create"
                        rightButtonEnabled: (searchbox.searchText.length > 0 && !internal.isExactMatchFound)
                        onBackClicked: internal.hideSearchResults();
                        onClearClicked: internal.hideSearchResults();
                        onRightButtonClicked: {
                            root.createObject(searchbox.searchText); // emit signal
                        }
                    }
                    Item {
                        id: focusStealer
                        width: 0
                        height: 0
                        focus: true
                        onFocusChanged: {
                            var searchboxHasFocus = (!focusStealer.focus);
                            if (searchboxHasFocus) {
                                internal.showSearchResults = true;
                                root.searchModeEntered();
                            }
                        }
                    }
                    function closeVirtualKeyboard() {
                        focusStealer.forceActiveFocus();
                    }
                }
            }

            Item {
                id: itemsViewContainer
                width: parent.width
                height: (internal.showSearchResults? searchResultsView.height : defaultView.height)

                Item {
                    id: defaultView
                    width: parent.width
                    height: defaultViewColumn.height

                    opacity: (internal.showSearchResults? 0: 1)
                    Behavior on opacity { NumberAnimation { duration: 200; } }

                    Rectangle {
                        anchors { fill: defaultView; bottomMargin: -topLevelFlickable.height; }
                        color: (_UI.isDarkColorScheme? "#000" : "#cccccc");
                        Rectangle {
                            id: topGlow
                            anchors { top: parent.top; left: parent.left; right: parent.right; }
                            height: _UI.paddingLarge;
                            gradient: Gradient {
                                GradientStop { position: 0; color: (_UI.isDarkColorScheme? "#555" :"#ffffff"); }
                                GradientStop { position: 0.1; color: (_UI.isDarkColorScheme? "#444" : "#eeeeee"); }
                                GradientStop { position: 1; color: (_UI.isDarkColorScheme? "#000" : "#cccccc"); }
                            }
                            opacity: (root.isSearchable? 1 : 0)
                        }
                    }

                    Column {
                        id: defaultViewColumn
                        width: parent.width
                        spacing: _UI.paddingLarge

                        TextLabel {
                            text: root.headerText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignBottom
                            height: (root.headerText? implicitHeight + _UI.paddingLarge : 0)
                            font.pixelSize: _UI.fontSizeSmall
                            wrapMode: Text.Wrap
                            anchors { left: parent.left; leftMargin: _UI.paddingLarge;
                                      right: parent.right; rightMargin: _UI.paddingLarge; }
                        }

                        TextLabel {
                            text: ((selectedItemsColumn.height > 0)? root.selectedItemsHeaderText : root.selectedItemsHeaderTextOnNullSelection);
                            verticalAlignment: Text.AlignBottom
                            height: implicitHeight + _UI.paddingLarge
                            anchors { left: parent.left; leftMargin: _UI.paddingLarge; }
                        }

                        Item {
                            width: root.width
                            height: selectedItemsColumn.height
                            Rectangle { anchors.fill: selectedItemsColumn; radius: internal.roundedRectRadius; color: (_UI.isDarkColorScheme? "black" : "white"); }
                            Column {
                                id: selectedItemsColumn
                                width: root.itemWidth
                                anchors.centerIn: parent
                                clip: true
                                Repeater {
                                    id: selectedItemsRepeater
                                    model: selectedItemsModel
                                    delegate: Component {
                                        SelectableItem {
                                            id: selectableItem
                                            width: selectedItemsColumn.width
                                            text: model.name
                                            checkmarksShown: root.checkMarkShown
                                            checked: true
                                            clickable: (!root.canSelectExactlyOneItem)
                                            onClicked: {
                                                if (!root.canSelectExactlyOneItem) {
                                                    internal.deselectItemByObjectId(model.objectId);
                                                }
                                            }
                                            property bool animationsEnabled: true;
                                            Behavior on height {
                                                NumberAnimation { duration: internal.animationDuration; }
                                                enabled: animationsEnabled
                                            }
                                            Behavior on opacity {
                                                NumberAnimation { duration: internal.animationDuration; }
                                                enabled: animationsEnabled
                                            }
                                            function hide() {
                                                selectableItem.opacity = 0;
                                                selectableItem.height = 0;
                                            }
                                            Rectangle {
                                                id: topSeparatorLine
                                                anchors { top: parent.top; left: parent.left; right: parent.right; }
                                                height: 1
                                                color: "lightgray";
                                                opacity: (index? 1 : 0);
                                            }
                                        }
                                    }
                                    onItemAdded: {
                                        // animate addition of items
                                        if (internal.selectedItemsListInitialized) { // don't animate during list initialization
                                            var savedHeight = item.height;
                                            item.animationsEnabled = false; // don't animate setting of animation start value
                                            item.height = 0;
                                            item.opacity = 0;
                                            item.animationsEnabled = true;
                                            item.height = savedHeight;
                                            item.opacity = 1;
                                        }
                                    }
                                    onItemRemoved: {
                                        // The item is already removed when this is called, so we cannot animate the removal here.
                                        // So we start the animation in selectedItem.hide(), which we call just before removing any item
                                    }
                                }
                            }
                            Rectangle { anchors.fill: selectedItemsColumn; radius: internal.roundedRectRadius; color: "transparent"; border { color: "gray"; width: 2; } smooth: true; }
                        }

                        TextLabel {
                            text: ((allItemsColumn.height > 0)? root.allItemsHeaderText : " ");
                            verticalAlignment: Text.AlignBottom
                            height: implicitHeight + _UI.paddingLarge
                            anchors { left: parent.left; leftMargin: _UI.paddingLarge; }
                        }

                        Item {
                            width: root.width
                            height: allItemsColumn.height
                            Rectangle { anchors.fill: allItemsColumn; radius: internal.roundedRectRadius; color: (_UI.isDarkColorScheme? "black" : "white"); }
                            Column {
                                id: allItemsColumn
                                width: root.itemWidth
                                anchors.centerIn: parent
                                Repeater {
                                    id: allItemsRepeater
                                    model: internal.allItemsListModel
                                    delegate: Component {
                                        SelectableItem {
                                            width: allItemsColumn.width
                                            text: model.name
                                            checkmarksShown: root.checkMarkShown
                                            checked: model.checked
                                            onClicked: {
                                                if (root.canSelectExactlyOneItem) {
                                                     if (!model.checked) {
                                                         internal.setExclusiveSelectedItemByAllItemsIndex(index);
                                                     }
                                                } else {
                                                    if (model.checked) {
                                                        internal.deselectItemByObjectId(model.objectId);
                                                    } else {
                                                        internal.selectItemByAllItemsIndex(index);
                                                    }
                                                }
                                            }
                                            Rectangle {
                                                id: topSeparatorLine
                                                anchors { top: parent.top; left: parent.left; right: parent.right; }
                                                height: 1
                                                color: "lightgray";
                                                opacity: (index? 1 : 0);
                                            }
                                        }
                                    }
                                }
                            }
                            Rectangle { anchors.fill: allItemsColumn; radius: internal.roundedRectRadius; color: "transparent"; border { color: "gray"; width: 2; } smooth: true; }
                        }

                        Item { width: 1; height: 1; } // for space at the bottom

                    } // Column // defaultViewColumn
                } // Item // defaultView

		Flickable {
		    id: searchResultsView
		    width: parent.width
		    height: (topLevelFlickable.height - searchbox.height)
		    contentHeight: matchingItemsColumn.height
		    contentWidth: width
		    clip: true
		    interactive: internal.showSearchResults
		    flickableDirection: Flickable.VerticalFlick

		    opacity: (internal.showSearchResults? 1 : 0)
		    Behavior on opacity { NumberAnimation { duration: internal.animationDuration; } }

		    onMovementStarted: {
			searchboxFocusManager.closeVirtualKeyboard();
		    }

		    Column {
			id: matchingItemsColumn
			width: parent.width

			Item {
			    id: createObjectItem
			    width: parent.width
			    property int displayableHeight: (Math.max(createLabel.implicitHeight, createButton.height) + _UI.paddingMedium * 2)
			    property string objectNameToCreate: searchbox.searchText
			    property bool isHidden: (objectNameToCreate.length == 0 || internal.isExactMatchFound || !_UI.isPortrait)
			    height: (isHidden? 0 : displayableHeight)
			    opacity: (isHidden? 0 : 1)
			    TextLabel {
				id: createLabel;
				anchors {
				    left: parent.left; leftMargin: _UI.paddingLarge;
				    right: createButtonContainer.left; rightMargin: _UI.paddingLarge;
				    top: parent.top;
				}
				height: parent.height
				text: createObjectItem.objectNameToCreate
				verticalAlignment: Text.AlignVCenter
				wrapMode: Text.Wrap
				font.bold: true
			    }
			    Item {
				id: createButtonContainer;
				height: parent.height
				width: createButton.width
				anchors { right: parent.right; rightMargin: _UI.paddingLarge; top: parent.top; }
                                AutoWidthButton {
				    id: createButton;
				    anchors.centerIn: parent;
                                    colorScheme: Qt.white
				    text: "Create";
				    enabled: (createObjectItem.objectNameToCreate.length > 0 && !internal.isExactMatchFound);
				    onClicked: {
					root.createObject(createObjectItem.objectNameToCreate); // emit signal
				    }
				}
			    }
			}

			Repeater {
			    width: parent.width
			    model: matchingItemsModel
			    delegate: Component {
				SelectableItem {
				    width: root.width
				    text: model.richTextName
				    checkmarksShown: root.checkMarkShown
				    checked: model.checked
				    onClicked: {
					if (root.canSelectExactlyOneItem) {
					    if (!model.checked) {
						internal.setExclusiveSelectedItemByAllItemsIndex(model.allItemsListIndex);
					    }
					    internal.hideSearchResults();
					} else {
					    if (model.checked) {
						internal.deselectItemByObjectId(model.objectId);
					    } else {
						internal.selectItemByAllItemsIndex(model.allItemsListIndex);
					    }
					}
				    }
				    Rectangle {
					id: topSeparatorLine
					anchors { top: parent.top; left: parent.left; right: parent.right; }
					height: 1
					color: "lightgray";
				    }
				}
			    }
			}
		    }
		} // Flickable // searchResultsView
	    } // Item
	}
    }

    ScrollDecorator {
        flickableItem: topLevelFlickable
        opacity: (internal.showSearchResults? 0 : 1)
    }

    function setAllItemsListModel(listModel) {
        selectedItemsModel.clear();
        for (var i = 0; i < listModel.count; i++) {
            var item = listModel.get(i);
            if (item.checked) {
                selectedItemsModel.append({ objectId: item.objectId, name: item.name, allItemsListIndex: i });
            }
        }
        internal.allItemsListModel = listModel;
        internal.selectedItemsListInitialized = true;
    }

    function selectedItems() {
        return selectedItemsModel;
    }

    function objectCreated(objectId, name, checked) {
        if (name.length == 0) {
            return;
        }
        // insert in all items list
        var allItemsIndex = internal.insertItemInList(internal.allItemsListModel, { objectId: objectId, name: name, checked: checked });
        internal.updateAllItemsIndexInList(selectedItemsModel, allItemsIndex);
        internal.updateAllItemsIndexInList(matchingItemsModel, allItemsIndex);
        // insert in matching items list
        if (name == internal.matchingItemsSearchString) {
            internal.insertItemInList(matchingItemsModel, { objectId: objectId, name: name, checked: checked, allItemsListIndex: allItemsIndex,
                                                            richTextName: "<u>" + name + "</u>" });
            if (!internal.isExactMatchFound) {
                internal.isExactMatchFound = true;
            }
        }
        // insert in selected items list
        if (checked) {
            if (root.canSelectExactlyOneItem) {
                internal.setExclusiveSelectedItemByAllItemsIndex(allItemsIndex);
                internal.hideSearchResults();
            } else {
                internal.insertItemInList(selectedItemsModel, { objectId: objectId, name: name, allItemsListIndex: allItemsIndex });
            }
        }
    }
}
