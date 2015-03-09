import com.nokia.meego 1.1
ToolBar {
    property int colorScheme: Qt.black;
    property string __invertedString: ((theme.inverted || colorScheme == Qt.black)? "-inverted" : "");
    platformStyle: ToolBarStyle {
       background: "image://theme/meegotouch-toolbar-" +
            ((screen.currentOrientation == Screen.Portrait || screen.currentOrientation == Screen.PortraitInverted) ? "portrait" : "landscape") +
            ((theme.inverted || colorScheme == Qt.black)? "-inverted" : "") +
            "-background";
    }
}
