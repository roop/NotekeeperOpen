import QtQuick 1.1

Item {
    id: root
    property string iconPath;
    signal clicked();

    UiStyle {
        id: _UI
    }

    BorderImage {
        id: highlight
        border { left: 20; top: 20; right: 20; bottom: 20 }
        source: "images/qt_components_graphics/qtg_fr_pushbutton_pressed.svg"
        opacity: (mouseArea.pressed? 1 : 0)
        Behavior on opacity { NumberAnimation { duration: 100; } }
        anchors.fill: parent
    }

    Image {
        id: icon
        source: ("../notekeeper/" + iconPath)
        sourceSize.width: _UI.graphicSizeSmall
        sourceSize.height: _UI.graphicSizeSmall
        smooth: true
        anchors.centerIn: parent
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.clicked();
    }
}
