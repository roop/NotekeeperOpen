import com.nokia.symbian 1.1
BusyIndicator {
    property int colorScheme: Qt.black;
    property string size; // ignored
    platformInverted: (colorScheme == Qt.white);
}
