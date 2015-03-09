import QtQuick 1.1

import "qt_internal" 1.1  // for TextMagnifier
import com.nokia.symbian 1.1 // for TextMagnifier

// UITextEdit.qml
// A text edit with
//     - selection grips that can be dragged to modify the selection
//     - context menu
//     - ruled paper background that adjusts to the font size

Item {
    id: root
    property alias readOnly: textEdit.readOnly
    property alias text: textEdit.text
    property alias textHeight: textEdit.implicitHeight
    height: textHeight
    property alias fontFamily: textEdit.font.family
    property alias fontPixelSize: textEdit.font.pixelSize
    property alias color: textEdit.color
    property alias flickableContentY: textEdit.flickableContentY
    property alias flickableHeight: textEdit.flickableHeight
    property alias flickableContentHeaderHeight: textEdit.flickableContentHeaderHeight
    property alias flickableIsMoving: textEdit.flickableIsMoving
    property bool textHasBeenChangedByUser: false
    property bool virtualKeyboardVisible: false

    signal scrollToYRequested(int y)
    signal disableUserFlicking()
    signal enableUserFlicking()
    signal enterPressed()

    TextEdit {
        id: textEdit
        anchors { left: parent.left; top: parent.top; }
        width: root.width
        height: root.height
        focus: false

        property bool isEditing: activeFocus
        property int flickableContentY: 0                    // viewport is that part of the textedit that's visible;
        property int flickableHeight: textEdit.height // UITextEdit tries to keep current selection grip and context menu within the viewport
        property int flickableContentHeaderHeight: 0
        property bool flickableIsMoving: false

        text: "This is some example text\nThis will get replaced when actually using this component.\n"
        wrapMode: Text.Wrap
        activeFocusOnPress: false
        inputMethodHints: ((qmlDataAccess.qtVersionRuntimeMinor <= 7) ? Qt.ImhNoPredictiveText : Qt.ImhNone)
                          // In Qt 4.7.x (I think), when predictive text is enabled, and if user tapped on the
                          // actual-text-popup, it crashes the app
        font.pixelSize: platformStyle.fontSizeLarge
        color: "#333333"
        selectionColor: "#c2d3ec"
        selectedTextColor: "#000080"
        cursorDelegate: {
            if (privateData.textEditHasSelection) {
                return invisibleCursorDelegate;
            } else if (privateData.isMagnifierShown) {
                return unblinkingTextCursorDelegate;
            } else if (isEditing || contextMenu.isOpen) {
                return blinkingTextCursorDelegate;
            }
            return invisibleCursorDelegate;
        }

        Component.onCompleted: {
            contextMenu.clicked.connect(contextMenuItemClicked);
            privateData.textLength = text.length;
        }

        onSelectionChanged: { // this fires only when selection changes from present to absent, or vice versa
            privateData.textEditHasSelection = (textEdit.selectionStart >= 0 && textEdit.selectionStart != textEdit.selectionEnd);
            if (!textEdit.hasSelection()) { // if selection was removed by the user, probably by typing over a selection
                textEdit.hideSelectionGrips();
            }
            textEdit.hideContextMenu();
        }

        onWidthChanged: {
            if (textEdit.hasSelection()) {
                textEdit.showSelectionGrips();
            }
            textEdit.reshowOpenContextMenu();
        }

        onIsEditingChanged: {
            if (textEdit.isEditing) {
                contextMenu.changeMenuItemTexts([]); // we'll populate this before showing the menu
            } else {
                if (textEdit.hasSelection()) {
                    if (textEdit.selectionStart == 0 && textEdit.selectionEnd == textEdit.text.length) { // selected all
                        contextMenu.changeMenuItemTexts(["Copy", "Clear selection"]);
                    } else {
                        contextMenu.changeMenuItemTexts(["Copy", "Select all"]);
                    }
                } else {
                    textEdit.closeContextMenu();
                }
            }
        }

        onFlickableIsMovingChanged: {
            if (flickableIsMoving) {
                if (contextMenu.isOpen) {
                    textEdit.hideContextMenu();
                }
            } else {
                textEdit.reshowOpenContextMenu();
            }
        }

        Keys.onPressed: {
            onUserEditedTheText();
            if (event.key == Qt.Key_Enter) {
                root.enterPressed();
            }
        }
        Keys.onReleased: onUserEditedTheText();
        onTextChanged: onUserEditedTheText();
        // On the Nokia E6 / E7 (with physical keyboard), Keys.onPressed/onReleased
        // doesn't get called when using the physical keyboard.
        // So we try to work around that by looking for a change in text length. (Yuck. And flaky.)

        function onUserEditedTheText() {
            if (isEditing /* ensure it's really the user that made this edit */ &&
                privateData.textLength != text.length /* ensure we haven't handled this before */) {
                textEdit.ensureCursorIsInViewport();
                textEdit.hideContextMenu();
                root.textHasBeenChangedByUser = true;
                privateData.textLength = text.length;
            }
        }

        Timer {
            id: ensureCursorVisibleAfterKeyboardShownTimer
            running: false
            repeat: false
            interval: 300 // a little after keyboard show animation completes
            onTriggered: {
                if (root.virtualKeyboardVisible) {
                    textEdit.ensureCursorIsInViewport();
                }
            }
        }

        QtObject {
            id: privateData
            property bool textEditHasSelection: false;
            property variant hitTestPoint: Qt.point(0, 0);
            property int magnifierYOffset: 0
            property bool isMagnifierShown: false
            property int tapCounter: 0
            property int textLength: 0
        }

        MouseArea {
            id: textEditMouseArea
            anchors.fill: parent
            onClicked: {
                var contextMenuShouldBeOpen = false;
                var tappedPos = textEdit.positionAt(mouseX, mouseY);
                qmlDataAccess.resetPreeditText();
                if (textEdit.hasSelection()) {
                    if (((mouseY >= leftTextSelectionEdge.y) && (mouseY < rightTextSelectionEdge.y + textEdit.font.pixelSize)) &&
                        ((textEdit.selectionStart < tappedPos && textEdit.selectionEnd > tappedPos))) {
                        // clicked within selection
                        if (privateData.tapCounter == 2) { // this is the third tap
                            textEdit.selectAll();
                        } else {
                            textEdit.reshowOpenContextMenu();
                        }
                    } else {
                        textEdit.clearSelection();
                        textEdit.closeContextMenu();
                    }
                } else {
                    var eow = textEdit.nearestEndOfWord(tappedPos);
                    if (textEdit.isEditing) {
                        if (textEdit.cursorPosition == eow) {
                            if (privateData.tapCounter == 0) {
                                if (!contextMenu.isOpen) {
                                    contextMenu.changeMenuItemTexts(clipboard.canPaste()? ["Select", "Select all", "Paste"] : ["Select", "Select all"]);
                                }
                                contextMenuShouldBeOpen = (!contextMenu.isOpen); // toggle context menu
                            } else if (privateData.tapCounter == 1) { // this is the second tap
                                textEdit.selectWordIntelligently();
                            }
                        } else {
                            textEdit.cursorPosition = eow;
                            if (!virtualKeyboardVisible) {
                                textEdit.openSoftwareInputPanel();
                            }
                        }
                    } else {
                        textEdit.cursorPosition = eow;
                        textEdit.forceActiveFocus();
                        textEdit.openSoftwareInputPanel();
                    }
                }
                if (textEdit.hasSelection()) {
                    textEdit.showSelectionGrips();
                    contextMenu.changeMenuItemTexts(clipboard.canPaste()? ["Copy", "Cut", "Paste"] : ["Copy", "Cut"]);
                    contextMenuShouldBeOpen = true;
                }
                if (contextMenuShouldBeOpen != contextMenu.isOpen) {
                    if (contextMenuShouldBeOpen) {
                        textEdit.openContextMenu();
                    } else {
                        textEdit.closeContextMenu();
                    }
                }
                ++privateData.tapCounter;
                clickTimer.start();
            }
            onPressAndHold: {
                qmlDataAccess.resetPreeditText();
                // show cursor
                var pos = textEdit.positionAt(mouse.x, mouse.y);
                textEdit.cursorPosition = pos;
                // show magnifier
                privateData.hitTestPoint = { x: mouse.x, y: mouse.y };
                privateData.magnifierYOffset = 0;
                textMagnifier.show();
                privateData.isMagnifierShown = true;
                // other stuff
                textEdit.closeContextMenu();
                textEdit.forceActiveFocus();
                root.disableUserFlicking();
            }
            onReleased: {
                if (privateData.isMagnifierShown) {
                    textMagnifier.hide();
                    privateData.isMagnifierShown = false;
                    root.enableUserFlicking();
                    if (textEdit.isEditing) {
                        textEdit.cursorPosition = textEdit.positionAt(mouseX, mouseY);
                        textEdit.ensureCursorIsInViewport();
                        contextMenu.changeMenuItemTexts(clipboard.canPaste()? ["Select", "Select all", "Paste"] : ["Select", "Select all"]);
                    } else {
                        contextMenu.changeMenuItemTexts(["Select", "Select all"]);
                    }
                    textEdit.openContextMenu();
                }
            }
            onPositionChanged: {
                if (privateData.isMagnifierShown) {
                    var pos = textEdit.positionAt(mouse.x, mouse.y);
                    textEdit.select(pos, pos);
                    privateData.hitTestPoint = { x: mouse.x, y: mouse.y };
                    textEdit.ensureCursorIsInViewport();
                }
            }
        }

        UITextSelectionEdge {
            id: rightTextSelectionEdge
            x: -1000
            y: -1000
            edgeAlignment: Qt.AlignRight
            gripAlignment: Qt.AlignBottom
            textEdit: textEdit
            verticalLineColor: "#2a2aff"
            onPressed: {
                privateData.hitTestPoint = point; // point is passed down from the signal
                privateData.magnifierYOffset = (textEdit.font.pixelSize * (gripAlignment == Qt.AlignTop? -1 : 0));
                textMagnifier.show();
                privateData.isMagnifierShown = true;
                root.disableUserFlicking(); // emit signal
                textEdit.hideContextMenu();
            }
            onReleased: {
                textMagnifier.hide();
                privateData.isMagnifierShown = false;
                root.enableUserFlicking(); // emit signal
                textEdit.reshowOpenContextMenu();
                leftTextSelectionEdge.limitingY = y;
            }
            onMoved: {
                textEdit.ensureYIsInViewport(point.y, textEdit.font.pixelSize, textEdit.font.pixelSize);
            }
            onFingerMoved: {
                privateData.hitTestPoint = point; // point is passed down from the signal
            }
        }

        UITextSelectionEdge {
            id: leftTextSelectionEdge
            x: -1000
            y: -1000
            edgeAlignment: Qt.AlignLeft
            gripAlignment: Qt.AlignTop
            textEdit: textEdit
            verticalLineColor: "#2a2aff"
            onPressed: {
                privateData.hitTestPoint = point; // point is passed down from the signal
                privateData.magnifierYOffset = (textEdit.font.pixelSize * (gripAlignment == Qt.AlignTop? -1 : 0));
                textMagnifier.show();
                privateData.isMagnifierShown = true;
                root.disableUserFlicking(); // emit signal
                textEdit.hideContextMenu();
            }
            onReleased: {
                textMagnifier.hide();
                privateData.isMagnifierShown = false;
                root.enableUserFlicking(); // emit signal
                textEdit.reshowOpenContextMenu();
                rightTextSelectionEdge.limitingY = y;
            }
            onMoved: {
                textEdit.ensureYIsInViewport(point.y, textEdit.font.pixelSize, textEdit.font.pixelSize);
            }
            onFingerMoved: {
                privateData.hitTestPoint = point; // point is passed down from the signal
            }
        }

        UIContextMenu {
            id: contextMenu
            height: 50
            anchors { horizontalCenter: parent.horizontalCenter; }
            menuItemTexts: [ "Copy", "Select all" ]
            opacity: 0
            Behavior on opacity { NumberAnimation { duration: 200 }}
            onPressed: { root.disableUserFlicking(); privateStyle.play(Symbian.BasicButton); }
            onReleased: { root.enableUserFlicking(); privateStyle.play(Symbian.BasicButton); }
            property bool isOpen: false;
            function changeMenuItemTexts(list) {
                if (!isOpen) {
                    menuItemTexts = list;
                } else {
                    restoreHelper.savedMenuItemTexts = list;
                    opacity = 0; // hide context menu
                    animationEndCountdownTimer.triggered.connect(restoreHelper.restore); // will show it later
                    animationEndCountdownTimer.start();
                }
            }
            QtObject {
                id: restoreHelper
                property variant savedMenuItemTexts
                function restore() {
                    contextMenu.menuItemTexts = savedMenuItemTexts;
                    contextMenu.opacity = 1; // show context menu
                    animationEndCountdownTimer.triggered.disconnect(restoreHelper.restore);
                }
            }
            Timer {
                id: animationEndCountdownTimer
                repeat: false
                interval: 200; // same as the duration for the opacity bahaviour animation
            }
        }

        Component {
            id: blinkingTextCursorDelegate
            Item {
                width: 2;
                height: textEdit.font.pixelSize
                Rectangle {
                    id: cursor
                    width: parent.width;
                    height: parent.height
                    anchors { left: parent.left; leftMargin: -2; top: parent.top; }
                    color: "#2a2aff"
                    opacity: (isVisible? 0.8 : 0.0)
                    property bool isVisible: false
                }
                Timer {
                    id: cursorBlinkTimer
                    interval: 500
                    repeat: true
                    running: true
                    onTriggered: cursor.isVisible = (!cursor.isVisible);
                }
            }
        }

        Component {
            id: unblinkingTextCursorDelegate
            Item {
                width: 2;
                height: textEdit.font.pixelSize
                Rectangle {
                    id: cursor
                    width: parent.width;
                    height: parent.height
                    anchors { left: parent.left; leftMargin: -2; top: parent.top; }
                    color: "#2a2aff"
                    opacity: 0.8
                }
            }
        }

        Component {
            id: invisibleCursorDelegate // because "TextEdit.cursorVisible = false" doesn't work
            Item {
            }
        }

        TextMagnifier {
            id: textMagnifier
            editor: textEdit
            contentCenter: privateData.hitTestPoint
            yAdditionalOffset: privateData.magnifierYOffset
            platformInverted: false
        }

        Timer {
            id: clickTimer
            interval: 400;
            repeat: false
            onTriggered: {
                running = false;
                privateData.tapCounter = 0;
            }
        }

        function hasSelection() {
            return privateData.textEditHasSelection;
        }

        function selectWordAt(x, y) {
            var position = textEdit.positionAt(x, y);
            textEdit.cursorPosition = position;
            textEdit.selectWord();
            showSelectionGrips();
        }

        function showSelectionGrips() {
            if (textEdit.hasSelection()) {
                leftTextSelectionEdge.setRectangle(textEdit.positionToRectangle(textEdit.selectionStart)); // show
                rightTextSelectionEdge.setRectangle(textEdit.positionToRectangle(textEdit.selectionEnd)); // show
                leftTextSelectionEdge.limitingY = rightTextSelectionEdge.y;
                rightTextSelectionEdge.limitingY = leftTextSelectionEdge.y;
            }
        }

        function clearSelection() {
            textEdit.select(textEdit.selectionEnd, textEdit.selectionEnd);
            hideSelectionGrips();
        }

        function hideSelectionGrips() {
            leftTextSelectionEdge.hide();
            rightTextSelectionEdge.hide();
        }

        function openContextMenu() { // open menu
            contextMenu.isOpen = true;
            reshowOpenContextMenu();
        }

        function closeContextMenu() { // close menu
            hideContextMenu();
            contextMenu.isOpen = false;
        }

        function reshowOpenContextMenu() { // re-position menu / re-show after temporarily hiding it
            if (!contextMenu.isOpen)
                return;
            var yIfPlacedAboveSelection = 1000;
            if (textEdit.hasSelection()) {
                yIfPlacedAboveSelection = leftTextSelectionEdge.y - (textEdit.font.pixelSize * 1.5) - contextMenu.height;
            } else {
                yIfPlacedAboveSelection = textEdit.cursorRectangle.y - (textEdit.font.pixelSize * 1.5) - contextMenu.height;
            }
            if (yIfPlacedAboveSelection  > (textEdit.flickableContentY - textEdit.flickableContentHeaderHeight)) {
                // if placed above selection, menu wouldn't disappear above
                contextMenu.y = yIfPlacedAboveSelection;
            } else {
                // if placed above selection, it would disappear above
                var yIfPlacedBelowSelection = -1000;
                if (textEdit.hasSelection()) {
                    yIfPlacedBelowSelection = rightTextSelectionEdge.y + (textEdit.font.pixelSize * 1.5) + rightTextSelectionEdge.height;
                } else {
                    yIfPlacedBelowSelection = textEdit.cursorRectangle.y + (textEdit.font.pixelSize * 1.5) + textEdit.cursorRectangle.height * 2;
                }
                if (yIfPlacedBelowSelection + contextMenu.height < (textEdit.flickableContentY - textEdit.flickableContentHeaderHeight + textEdit.flickableHeight)) {
                    // if placed below selection, menu wouldn't disappear below
                    contextMenu.y = yIfPlacedBelowSelection;
                } else {
                    // if placed below selection, it would disappear below
                    // just place it in the middle then
                    contextMenu.y = textEdit.flickableContentY - textEdit.flickableContentHeaderHeight + (textEdit.flickableHeight / 2) - (contextMenu.height / 2);
                }
            }
            contextMenu.opacity = 1.0;
        }

        function hideContextMenu() { // temporarily hide menu
            if (!contextMenu.isOpen)
                return;
            contextMenu.opacity = 0.0;
        }

        function contextMenuItemClicked(itemText) {
            if (itemText == "Copy") {
                textEdit.copy();
                textEdit.clearSelection();
            } else if (itemText == "Select all") {
                textEdit.selectAll();
                if (textEdit.isEditing) {
                    contextMenu.changeMenuItemTexts(clipboard.canPaste()? ["Copy", "Cut", "Paste"] : ["Copy", "Cut"]);
                } else {
                    contextMenu.changeMenuItemTexts(["Copy", "Clear selection"]);
                }
            } else if (itemText == "Select") {
                textEdit.selectWordIntelligently();
                if (textEdit.isEditing) {
                    contextMenu.changeMenuItemTexts(clipboard.canPaste()? ["Copy", "Cut", "Paste"] : ["Copy", "Cut"]);
                } else {
                    contextMenu.changeMenuItemTexts(["Copy", "Select all"]);
                }
            } else if (itemText == "Cut") {
                textEdit.cut();
                if (textEdit.hasSelection()) {
                    root.textHasBeenChangedByUser = true;
                }
            } else if (itemText == "Paste") {
                textEdit.paste();
                root.textHasBeenChangedByUser = true;
            } else if (itemText == "Clear selection") {
                textEdit.clearSelection();
            }

            if (textEdit.hasSelection()) {
                showSelectionGrips();
            } else {
                leftTextSelectionEdge.hide();
                rightTextSelectionEdge.hide();
                closeContextMenu();
            }
        }

        function ensureYIsInViewport(y, topPadding, bottomPadding) {
            var contentY = textEdit.flickableContentY;
            var contentHeight = textEdit.flickableHeight;
            var yOffset = textEdit.flickableContentHeaderHeight;
            if (contentY > (y + yOffset - topPadding)) { // if y is above current viewport's top edge
                root.scrollToYRequested(y + yOffset - topPadding);
            } else if ((contentY + contentHeight) < (y + yOffset + bottomPadding)) { // if y is below current viewport's bottom edge
                root.scrollToYRequested(y + yOffset + bottomPadding - contentHeight);
            }
        }

        function ensureCursorIsInViewport() {
            var cursorRect = textEdit.cursorRectangle;
            var bottomPadding = cursorRect.height * 1.5
            ensureYIsInViewport(cursorRect.y, 2, bottomPadding);
        }

        function nearestEndOfWord(pos) {
            var str = textEdit.text;
            var spaceChars = /[ \f\r\t\v]/;
            var strlen = str.length;
            var charAtPos = str.charAt(pos);
            if (charAtPos == '\n') {
                return pos;
            } else if (charAtPos.search(spaceChars) >= 0) { // if space
                // look backward for a non-space char
                return (findChar(str, strlen, pos, false, false) + 1);
            } else {
                // look forward for a space char
                return findChar(str, strlen, pos, true, true);
            }
        }

        function selectWordIntelligently() {
            var str = textEdit.text;
            var spaceChars = /[ \f\n\r\t\v]/;
            var strlen = str.length;
            var pos = textEdit.cursorPosition;
            var startOfWord, endOfWord;
            if (pos > 0 && str.charAt(pos).search(spaceChars) >= 0) { // if space
                endOfWord = findChar(str, strlen, pos, false, false); // look backward for non-space char
                startOfWord = findChar(str, strlen, endOfWord, false, true) + 1; // look backward for space char
                textEdit.select(startOfWord, pos);
            } else {
                endOfWord = findChar(str, strlen, pos, true, true); // look forward for space char
                startOfWord = findChar(str, strlen, endOfWord - 1, false, true) + 1; // look backward for space char
                textEdit.select(startOfWord, endOfWord);
            }
        }

        function findChar(str, strlen, pos, forward, findSpace) {
            var spaceChars = /[ \f\n\r\t\v]/;
            if (forward && pos == strlen) {
                return pos;
            } else if (!forward && pos == 0) {
                return pos;
            }
            while (true) {
                var spaceCharFound = (str.charAt(pos).search(spaceChars) >= 0);
                if (findSpace == spaceCharFound) {
                    break;
                }
                if (forward) {
                    if (++pos == strlen) {
                        break
                    }
                } else {
                    if (--pos == 0) {
                        break;
                    }
                }
            }
            return pos;
        }
    } /* TextEdit */

    Item {
        id: focusStealer
        width: 0
        height: 0
        focus: true
    }

    onVirtualKeyboardVisibleChanged: {
        if (virtualKeyboardVisible) {
            ensureCursorVisibleAfterKeyboardShownTimer.restart();
        } else {
            // take focus away from the TextEdit
            focusStealer.forceActiveFocus();
        }
    }

    function openKeyboard() {
        textEdit.cursorPosition = 0;
        textEdit.forceActiveFocus();
        textEdit.openSoftwareInputPanel();
    }

    function openKeyboardWithAllTextSelected() {
        textEdit.selectAll();
        textEdit.showSelectionGrips();
        textEdit.forceActiveFocus();
        textEdit.openSoftwareInputPanel();
    }

    function clickedOutside() {
        if (textEdit.hasSelection()) {
            textEdit.clearSelection();
        }
        if (contextMenu.isOpen) {
            textEdit.closeContextMenu();
        }
    }

    function clickedAtEnd() {
        if (contextMenu.isOpen) {
            textEdit.closeContextMenu();
        } else {
            textEdit.cursorPosition = textEdit.text.length;
            textEdit.forceActiveFocus();
            textEdit.openSoftwareInputPanel();
        }
    }

    function beginAppendText() {
        clickedAtEnd();
        textEdit.ensureCursorIsInViewport();
    }

    function beginEditFirstLine() {
        if (textEdit.text.length == 0) {
            textEdit.cursorPosition = 0;
        } else {
            var endOfLine = textEdit.text.indexOf('\n');
            if (endOfLine < 0) {
                endOfLine = textEdit.text.length;
            }
            textEdit.select(0, endOfLine);
            if (endOfLine > 0) {
                textEdit.showSelectionGrips();
            }
        }
        textEdit.forceActiveFocus();
        textEdit.openSoftwareInputPanel();
    }

    function longPressedAtEnd() {
        if (privateData.isMagnifierShown) {
            textMagnifier.hide();
            privateData.isMagnifierShown = false;
        }
        if (!textEdit.isEditing) {
            textEdit.forceActiveFocus();
        }
        textEdit.cursorPosition = textEdit.text.length;
        textEdit.ensureCursorIsInViewport();
        contextMenu.changeMenuItemTexts(clipboard.canPaste()? ["Select", "Select all", "Paste"] : ["Select", "Select all"]);
        textEdit.openContextMenu();
    }
}
