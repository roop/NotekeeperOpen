import QtQuick 1.0

// UITextSelectionEdge

Item {
    id: uiTextSelectionEdge
    x: -1000
    y: -1000
    width: 10
    height: 30
    property int gripAlignment: Qt.AlignTop
    property int edgeAlignment: Qt.AlignLeft
    property variant textEdit;
    property int limitingY; // this selection edge's movement is limited by the position of the other selection edge
    property alias verticalLineColor: verticalLine.color
    signal pressed(variant point)
    signal released()
    signal moved(variant point)
    signal fingerMoved(variant point)
    Rectangle {
        id: verticalLine
        width: 2;
        height: parent.height
        anchors { right: (uiTextSelectionEdge.edgeAlignment == Qt.AlignLeft? uiTextSelectionEdge.left : undefined);
                  left: (uiTextSelectionEdge.edgeAlignment == Qt.AlignRight? uiTextSelectionEdge.left : undefined);
                  top: parent.top; }
        color: "#2a2aff";
    }
    Image {
        id: selectionGripImage
        width: uiTextSelectionEdge.textEdit.font.pixelSize
        height: uiTextSelectionEdge.textEdit.font.pixelSize
        anchors { horizontalCenter: verticalLine.horizontalCenter;
                  verticalCenter: (uiTextSelectionEdge.gripAlignment == Qt.AlignTop? verticalLine.top : verticalLine.bottom);
                  verticalCenterOffset: height * (uiTextSelectionEdge.gripAlignment == Qt.AlignTop? -0.15 : 0.25); }
        source: "images/selection_grip.png"
        smooth: true
    }
    MouseArea {
        id: selectionGripMouseArea
        anchors { fill: selectionGripImage; margins: -10; }
        onMousePositionChanged: {
            var textEdit = uiTextSelectionEdge.textEdit;
            var yOffsetFromText = uiTextSelectionEdge.height * (uiTextSelectionEdge.gripAlignment == Qt.AlignTop? 0.5 : -0.5)
            var point = textEdit.mapFromItem(selectionGripMouseArea, mouseX, mouseY + yOffsetFromText);
            uiTextSelectionEdge.fingerMoved(point); // emit signal
            if (uiTextSelectionEdge.edgeAlignment == Qt.AlignLeft && point.y > uiTextSelectionEdge.limitingY + uiTextSelectionEdge.height) {
                point.y = uiTextSelectionEdge.limitingY + uiTextSelectionEdge.height * 0.5;
            }
            if (uiTextSelectionEdge.edgeAlignment == Qt.AlignRight && point.y < uiTextSelectionEdge.limitingY - uiTextSelectionEdge.height) {
                point.y = uiTextSelectionEdge.limitingY + uiTextSelectionEdge.height * 0.5;
            }
            var movedToPos = moveToPos(textEdit.positionAt(point.x, point.y));
            if (movedToPos) {
                uiTextSelectionEdge.moved(point);
            }
        }
        onPressed: {
            var textEdit = uiTextSelectionEdge.textEdit;
            var yOffsetFromText = uiTextSelectionEdge.height * (uiTextSelectionEdge.gripAlignment == Qt.AlignTop? 0.5 : -0.5)
            var point = textEdit.mapFromItem(selectionGripMouseArea, mouseX, mouseY + yOffsetFromText);
            uiTextSelectionEdge.pressed(point);
        }
        onReleased: uiTextSelectionEdge.released();
    }

    function moveToPos(pos) {
        if (uiTextSelectionEdge.edgeAlignment == Qt.AlignLeft &&
                pos != textEdit.selectionStart &&
                pos < textEdit.selectionEnd) {
            textEdit.select(pos, textEdit.selectionEnd);
            uiTextSelectionEdge.setRectangle(textEdit.positionToRectangle(pos));
            return true;
        }
        if (uiTextSelectionEdge.edgeAlignment == Qt.AlignRight &&
                pos != textEdit.selectionEnd &&
                pos > textEdit.selectionStart) {
            textEdit.select(textEdit.selectionStart, pos);
            uiTextSelectionEdge.setRectangle(textEdit.positionToRectangle(pos));
            return true;
        }
        return false;
    }

    function setRectangle(rect) {
        uiTextSelectionEdge.x = rect.x;
        uiTextSelectionEdge.y = rect.y;
        uiTextSelectionEdge.height = rect.height;
    }

    function hide() {
        uiTextSelectionEdge.x = -1000;
    }
}
