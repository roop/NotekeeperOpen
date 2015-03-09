import com.nokia.meego 1.1

Button {
    property int colorScheme: Qt.black;
    platformStyle: ButtonStyle { inverted: (colorScheme == Qt.black); }
}
