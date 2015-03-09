import QtQuick 1.1
import com.nokia.meego 1.1
BorderImage {
    id: bgImage
    property int colorScheme: Qt.black;
    source: "image://theme/meegotouch-toolbar-" +
            ((screen.currentOrientation == Screen.Portrait || screen.currentOrientation == Screen.PortraitInverted) ? "portrait" : "landscape") +
            ((theme.inverted || colorScheme == Qt.black)? "-inverted" : "") +
            "-background";
    border.left: 10
    border.right: 10
    border.top: 10
    border.bottom: 10

    // Mousearea that eats clicks so they don't go through the toolbar to content
    // that may exist below it in z-order, such as unclipped listview items.
    MouseArea { anchors.fill: parent }
}
