import QtQuick 1.1
import com.nokia.meego 1.1

Item {
    id: root
    property string iconPath;
    property int colorScheme; // ignored
    property url iconSource: (iconPath? ("../notekeeper/" + iconPath) : "");

    // following code copied from imports/com/nokia/meego/ToolIcon.qml

    property string platformIconId

    // TODO: deprecated
    property alias iconId: root.platformIconId
    width: 80; height: 64
    signal clicked

    // Styling for the ToolItem
    property Style platformStyle: ToolItemStyle{}

    // TODO: deprecated
    property Style style: root.platformStyle

    Image {
        source: mouseArea.pressed ? platformStyle.pressedBackground : ""
        anchors.centerIn: parent

        Image {
            function handleIconSource(iconId) {
                if (iconSource != "")
                    return iconSource;

                if (iconId == "")
                    return "";

                var prefix = "icon-m-"
                // check if id starts with prefix and use it as is
                // otherwise append prefix and use the inverted version if required
                if (iconId.indexOf(prefix) !== 0)
                    iconId =  prefix.concat(iconId).concat(theme.inverted ? "-white" : "");
                return "image://theme/" + iconId;
            }

            source: handleIconSource(iconId)
            anchors.centerIn: parent
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
