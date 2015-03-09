import com.nokia.meego 1.1

Button {
    property int colorScheme: Qt.black;
    property string iconPath;
    property string iconId;
    platformStyle: ButtonStyle { inverted: (colorScheme == Qt.black); }
    iconSource: (iconPath? ("../notekeeper/" + iconPath) :
                           (iconId? ("image://theme/icon-m-" + iconId) : "")
                )
}
