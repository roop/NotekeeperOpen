import com.nokia.meego 1.1
BusyIndicator {
    id: busyIndicatorRoot
    property int colorScheme: Qt.black;
    property string size: "large";
    platformStyle: BusyIndicatorStyle{ inverted: (colorScheme == Qt.white); size: busyIndicatorRoot.size; }
}
