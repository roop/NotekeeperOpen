import com.nokia.meego 1.1

Button {
    property string iconPath;
    iconSource: (iconPath? ("../notekeeper/" + iconPath) : "");
    platformStyle: ButtonStyle {
        background: ""
        pressedBackground: ((screen.currentOrientation == Screen.Portrait || screen.currentOrientation == Screen.PortraitInverted)?
                                "image://theme/meegotouch-button-positive-background-pressed" :
                                "")
        disabledBackground: ""
    }
}
