import QtQuick 1.1
import "textedit"
 
TextArea {
    id: textEdit
    property bool textHasBeenChangedByUser: false;
    property string fontFamily: font.family;
    property int fontPixelSize: font.pixelSize;
    property color color: "#000";

    signal enterPressed();

    // Unused properties for compatibility with the UITextEdit for Symbian
    property int flickableContentY: 0;
    property int flickableHeight: 0;
    property int flickableContentHeaderHeight: 0;
    property bool flickableIsMoving: false;
    property bool virtualKeyboardVisible: false;

    // Unused signals for compatibility with the UITextEdit for Symbian
    signal scrollToYRequested(int y);
    signal disableUserFlicking();
    signal enableUserFlicking();

    // Nullify styling
    style: TextAreaStyle {
        property color textColor: textEdit.color
        background: ""
        backgroundSelected: ""
        backgroundDisabled: ""
        backgroundError: ""
        backgroundCornerMargin: 0
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_Enter) {
            textEdit.enterPressed(); // emit signal
        }
        textHasBeenChangedByUser = true;
    }
    Keys.onReleased: {
        textHasBeenChangedByUser = true;
    }
    onTextChanged: {
        if (textEdit.activeFocus) {
            textHasBeenChangedByUser = true;
        }
    }

    function openKeyboardWithAllTextSelected() {
        textEdit.selectAll();
        textEdit.forceActiveFocus();
        textEdit.platformOpenSoftwareInputPanel();
    }

    function beginAppendText() {
        textEdit.cursorPosition = textEdit.text.length;
        textEdit.forceActiveFocus();
        textEdit.platformOpenSoftwareInputPanel();
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
        }
        textEdit.forceActiveFocus();
        textEdit.platformOpenSoftwareInputPanel();
    }

    function clickedAtEnd() {
        textEdit.cursorPosition = textEdit.text.length;
        textEdit.forceActiveFocus();
        textEdit.platformOpenSoftwareInputPanel();
    }

    function clickedOutside() {
        // Do nothing for now
    }
}
