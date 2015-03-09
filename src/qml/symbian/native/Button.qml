import com.nokia.symbian 1.1

Button {
    property int colorScheme: Qt.black;
    property bool __dialogButton; // meego-components-specific private property
    platformInverted: (colorScheme != Qt.black);
}
