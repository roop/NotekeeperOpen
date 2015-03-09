import com.nokia.symbian 1.1

ToolButton {
    property string iconId;
    property string iconPath;
    iconSource: (iconPath? ("../notekeeper/" + iconPath) : iconId);
    property int colorScheme: Qt.black;
    platformInverted: (colorScheme != Qt.black);
}
